/**
 * @file odbc.cpp
 * @brief odbc.hppで宣言されたクラスのメンバ関数の実装ファイル。
 */
#include <cstring>
#include <iostream>
#include "odbc.hpp"

using namespace odbc;

//============================================
// ヘルパー関数
//============================================

std::optional<SQL_TIMESTAMP_STRUCT> odbc::to_sql_timestamp(const std::chrono::system_clock::time_point& tp) {
	std::time_t timet = std::chrono::system_clock::to_time_t(tp);
	std::tm timeinfo;

	if (localtime_r(&timet, &timeinfo) == nullptr) {
		// 失敗することは稀ですが、エラーハンドリング
		LogManager::logError("localtime_r failed");
		return std::nullopt;
	} else {
		return SQL_TIMESTAMP_STRUCT{
			.year		= static_cast<SQLSMALLINT>(timeinfo.tm_year + 1900),
			.month		= static_cast<SQLUSMALLINT>(timeinfo.tm_mon + 1),
			.day		= static_cast<SQLUSMALLINT>(timeinfo.tm_mday),
			.hour		= static_cast<SQLUSMALLINT>(timeinfo.tm_hour),
			.minute		= static_cast<SQLUSMALLINT>(timeinfo.tm_min),
			.second		= static_cast<SQLUSMALLINT>(timeinfo.tm_sec),
			.fraction	= static_cast<SQLUINTEGER>(std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count() % 1000000000)
		};
	}
}

std::optional<std::chrono::system_clock::time_point> odbc::to_time_point(const SQL_TIMESTAMP_STRUCT& ts) {
	// ODBC構造体をC++のtime_pointに変換
	std::tm timeinfo = {
		.tm_sec = ts.second,
		.tm_min = ts.minute,
		.tm_hour = ts.hour,
		.tm_mday = ts.day,
		.tm_mon = ts.month - 1,
		.tm_year = ts.year - 1900,
		.tm_isdst = -1,
	};

	std::time_t timet = std::mktime(&timeinfo);
	if (timet == -1) {
		return std::nullopt;
	}
	return (std::chrono::system_clock::from_time_t(timet) + std::chrono::nanoseconds(ts.fraction));	// ナノ秒の精度も加える
}

//============================================
// OdbcException
//============================================

// ODBCドライバから詳細なエラー情報を取得する
std::string OdbcException::getOdbcError(SQLHANDLE handle, SQLSMALLINT handleType) {
	SQLCHAR sqlState[6]{};
	SQLINTEGER nativeError{};
	SQLCHAR message[SQL_MAX_MESSAGE_LENGTH]{};
	SQLSMALLINT textLength{};
	std::string errorMsg{"No specific error message available."};

	// 診断レコードの1番目を取得する
	if (SQLGetDiagRec(handleType, handle, 1, sqlState, &nativeError, message, sizeof(message), &textLength) == SQL_SUCCESS) {
		utils::assign_concat(errorMsg,
			"SQLSTATE: ", utils::make_strn_view(reinterpret_cast<char*>(sqlState), sizeof(sqlState)),
			", Native Error: ", std::to_string(nativeError),
			", Message: ", utils::make_strn_view(reinterpret_cast<char*>(message), static_cast<size_t>(textLength))
		);
	}
	return errorMsg;
}

OdbcException::OdbcException(const std::string& msg, SQLHANDLE handle, SQLSMALLINT handleType) :
	std::runtime_error(msg + " - " + getOdbcError(handle, handleType))
{}

//============================================
// OdbcConnection
//============================================

OdbcConnection::OdbcConnection(std::string dsn, std::string user, std::string pass) {
	SQLRETURN ret;

	// 1. 環境ハンドルの割り当て
	SQLHENV tmp_henv = SQL_NULL_HENV;
	ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &tmp_henv);
	// 確保できた時点で管理を unique_ptr に移譲する (失敗時は nullptr なので安全)
	henv.reset(tmp_henv);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to alloc env handle", nullptr, 0);
	}

	// 2. ODBCバージョンの設定
	// unique_ptr.get() で生ポインタを取り出して渡す
	ret = SQLSetEnvAttr(
		getEnvHandle(),
		SQL_ATTR_ODBC_VERSION,
		reinterpret_cast<SQLPOINTER>(static_cast<std::intptr_t>(SQL_OV_ODBC3)),
		0
	);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to set ODBC version", getEnvHandle(), SQL_HANDLE_ENV);
	}

	// 3. 接続ハンドルの割り当て
	SQLHDBC tmp_hdbc = SQL_NULL_HDBC;
	ret = SQLAllocHandle(SQL_HANDLE_DBC, getEnvHandle(), &tmp_hdbc);
	hdbc.reset(tmp_hdbc); // 管理移譲

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to alloc dbc handle", getEnvHandle(), SQL_HANDLE_ENV);
	}

	// 4. 接続実行
	ret = SQLConnect(
		getConnHandle(),
		reinterpret_cast<SQLCHAR*>(dsn.data()), SQL_NTS,
		reinterpret_cast<SQLCHAR*>(user.data()), SQL_NTS,
		reinterpret_cast<SQLCHAR*>(pass.data()), SQL_NTS
	);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		// 接続失敗時、hdbcのデストラクタが走るが、
		// まだ接続されていないので SQLDisconnect はエラーになる(or 何もしない)。
		// その後 SQLFreeHandle が呼ばれてメモリは解放される。安全。
		throw OdbcException("Failed to connect to database", getConnHandle(), SQL_HANDLE_DBC);
	}
}

void OdbcConnection::beginTransaction() {
	SQLRETURN ret = SQLSetConnectAttr(getConnHandle(), SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to begin transaction (disable autocommit)", getConnHandle(), SQL_HANDLE_DBC);
	}
}

void OdbcConnection::commit() {
	SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, getConnHandle(), SQL_COMMIT);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to commit transaction", getConnHandle(), SQL_HANDLE_DBC);
	}

	// オートコミットを有効に戻す
	ret = SQLSetConnectAttr(getConnHandle(), SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to re-enable autocommit after commit", getConnHandle(), SQL_HANDLE_DBC);
	}
}

void OdbcConnection::rollback() {
	SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, getConnHandle(), SQL_ROLLBACK);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to rollback transaction", getConnHandle(), SQL_HANDLE_DBC);
	}

	// オートコミットを有効に戻す
	ret = SQLSetConnectAttr(getConnHandle(), SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to re-enable autocommit after commit", getConnHandle(), SQL_HANDLE_DBC);
	}
}

//============================================
// OdbcStatement
//============================================

OdbcStatement::OdbcStatement(const OdbcConnection& conn) {
	SQLHSTMT tmp_hstmt = SQL_NULL_HSTMT;
	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, conn.getConnHandle(), &tmp_hstmt);
	hstmt.reset(tmp_hstmt); // 管理移譲

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to alloc statement handle", conn.getConnHandle(), SQL_HANDLE_DBC);
	}
}

int OdbcStatement::execute(std::string sql) {
	SQLRETURN ret = SQLExecDirect(getStmtHandle(), reinterpret_cast<SQLCHAR*>(sql.data()), SQL_NTS);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to execute query", getStmtHandle(), SQL_HANDLE_STMT);
	}
	column_map_initialized = false;	// カラムマップは再初期化が必要
	return getAffectedRowCount();
}

bool OdbcStatement::fetch() {
	SQLRETURN ret = SQLFetch(getStmtHandle());

	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) { return true; }	// 次の行の取得に成功
	if (ret == SQL_NO_DATA) { return false; }	// 結果セットの終端に到達

	// 上記以外は予期せぬエラー
	throw OdbcException("Failed to fetch row", getStmtHandle(), SQL_HANDLE_STMT);
}

void OdbcStatement::closeCursor() {
	SQLRETURN ret = SQLCloseCursor(getStmtHandle());
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		// 失敗した場合、それが「既に閉じている」エラー(24000)か確認する
		SQLCHAR sqlState[6];
		SQLGetDiagRec(SQL_HANDLE_STMT, getStmtHandle(), 1, sqlState, NULL, NULL, 0, NULL);
		// '24000'でなければ、それは予期せぬエラーなので例外をスロー
		if (std::string(reinterpret_cast<char*>(sqlState)) != "24000") {
			throw OdbcException("Failed to close cursor with an unexpected error", getStmtHandle(), SQL_HANDLE_STMT);
		}
		// '24000'の場合は何もしない（べき等性を保証）
	}
}

std::vector<std::map<std::string, std::any>> OdbcStatement::fetchDynamic() {
	SQLRETURN ret;
	std::vector<std::map<std::string, std::any>> allRows;
	// 1. 結果セットの列情報を取得 (列数、名前、型)
	SQLSMALLINT columnCount = 0;
	ret = SQLNumResultCols(getStmtHandle(), &columnCount);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to get number of result columns", getStmtHandle(), SQL_HANDLE_STMT);
	}
	std::vector<std::string> columnNames;
	std::vector<SQLSMALLINT> columnTypes;
	for (SQLUSMALLINT i = 1; i <= columnCount; i++) {
		SQLSMALLINT nameLen = 0;

		// まず名前の長さだけを取得する (バッファを 0 にして呼び出す)
		ret = SQLDescribeCol(getStmtHandle(), i, NULL, 0, &nameLen, NULL, NULL, NULL, NULL);
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			throw OdbcException("Failed to describe column for column map initialization", getStmtHandle(), SQL_HANDLE_STMT);
		}
		if (nameLen == 0) {
			nameLen = 255;
		}

		// 必要なサイズでバッファを確保 (+1 は終端文字用)
		std::vector<SQLCHAR> nameBuffer(nameLen + 1);
		SQLSMALLINT dataType;
		ret = SQLDescribeCol(getStmtHandle(), i, nameBuffer.data(), nameBuffer.size(), &nameLen, &dataType, NULL, NULL, NULL);
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			throw OdbcException("Failed to describe column", getStmtHandle(), SQL_HANDLE_STMT);
		}
		columnNames.push_back(std::string(reinterpret_cast<char*>(nameBuffer.data())));
		columnTypes.push_back(dataType);
	}

	// 2. 全ての行をループし、列の型に応じて適切なgetData<T>を呼び出す
	while (this->fetch()) {
		std::map<std::string, std::any> singleRow;
		for (SQLUSMALLINT i = 1; i <= columnCount; i++) {
			std::any value;	// 空のstd::anyとしてデフォルト構築
			switch (columnTypes[i - 1]) {
			case SQL_TINYINT: {
				auto data = this->getData<SQLSCHAR>(i);
				if (data.has_value()) { value = data.value(); }
				break;
			}
			case SQL_SMALLINT: {
				auto data = this->getData<SQLSMALLINT>(i);
				if (data.has_value()) { value = data.value(); }
				break;
			}
			case SQL_INTEGER: {
				auto data = this->getData<SQLINTEGER>(i);
				if (data.has_value()) { value = data.value(); }
				break;
			}
			case SQL_BIGINT: {
				auto data = this->getData<SQLBIGINT>(i);
				if (data.has_value()) { value = data.value(); }
				break;
			}
			case SQL_REAL: {
				auto data = this->getData<SQLREAL>(i);
				if (data.has_value()) { value = data.value(); }
				break;
			}
			case SQL_FLOAT: {
				auto data = this->getData<SQLFLOAT>(i);
				if (data.has_value()) { value = data.value(); }
				break;
			}
			case SQL_DOUBLE: {
				auto data = this->getData<SQLDOUBLE>(i);
				if (data.has_value()) { value = data.value(); }
				break;
			}
			case SQL_DECIMAL: case SQL_NUMERIC: {
				auto data = this->getData<double>(i);
				if (data.has_value()) { value = data.value(); }
				break;
			}
			case SQL_TYPE_DATE: case SQL_TYPE_TIME: case SQL_TYPE_TIMESTAMP: {
				auto data = this->getData<std::chrono::system_clock::time_point>(i);
				if (data.has_value()) { value = data.value(); }
				break;
			}
			case SQL_BINARY: case SQL_VARBINARY: case SQL_LONGVARBINARY: {
				auto data = this->getData<std::vector<std::uint8_t>>(i);
				if (data.has_value()) { value = data.value(); }
				break;
			}
			default: { // 不明な型は文字列として取得を試みる
				auto data = this->getData<std::string>(i);
				if (data.has_value()) { value = data.value(); }
				break;
			}
			}
			singleRow.try_emplace(columnNames[i - 1], std::move(value));
		}
		allRows.push_back(std::move(singleRow));
	}
	// 3. 処理完了後、カーソルを閉じてステートメントをクリーンな状態にする
	this->closeCursor();
	return allRows;
}

// === プリペアドステートメント関連 ===

void OdbcStatement::prepare(std::string sql) {
	// 1. まず古いSQLのバインド情報とC++のバッファを完全にクリアし、クリーンな状態にする
	clearParams();

	// 2. ODBCドライバにSQL文をコンパイルさせる
	SQLRETURN ret = SQLPrepare(getStmtHandle(), reinterpret_cast<SQLCHAR*>(sql.data()), SQL_NTS);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to prepare statement", getStmtHandle(), SQL_HANDLE_STMT);
	}
	column_map_initialized = false;	// カラムマップは再初期化が必要
}

void OdbcStatement::setBatchSize(size_t size) {
	SQLRETURN ret;

	if (size > ATTR_PARAMSET_SIZE_MAX_VALUE) {
		throw std::invalid_argument("Batch size exceeds ODBC maximum limit of " + std::to_string(ATTR_PARAMSET_SIZE_MAX_VALUE));
	}

	// 【安全対策】配列のサイズを変更（再配置）する前に、
	// ODBCドライバが古いポインタを参照しないよう確実に切断しておく
	ret = SQLSetStmtAttr(getStmtHandle(), SQL_ATTR_PARAM_STATUS_PTR, NULL, 0);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to disconnect SQL_ATTR_PARAM_STATUS_PTR", getStmtHandle(), SQL_HANDLE_STMT);
	}
	ret = SQLSetStmtAttr(getStmtHandle(), SQL_ATTR_PARAMS_PROCESSED_PTR, NULL, 0);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to disconnect SQL_ATTR_PARAMS_PROCESSED_PTR", getStmtHandle(), SQL_HANDLE_STMT);
	}

	if (size > 1) {
		// 1. メモリ確保しつつ以前のデータをクリア
		param_status_array.assign(size, 0);
		*params_processed = 0;

		// 2. ODBCドライバに配列サイズと、内部変数のアドレスをまとめて通知する
		ret = SQLSetStmtAttr(getStmtHandle(), SQL_ATTR_PARAMSET_SIZE, reinterpret_cast<SQLPOINTER>(static_cast<std::intptr_t>(size)), 0);
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			throw OdbcException("Failed to set SQL_ATTR_PARAMSET_SIZE", getStmtHandle(), SQL_HANDLE_STMT);
		}
		paramset_size = size;

		ret = SQLSetStmtAttr(getStmtHandle(), SQL_ATTR_PARAM_STATUS_PTR, param_status_array.data(), 0);
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			throw OdbcException("Failed to set SQL_ATTR_PARAM_STATUS_PTR", getStmtHandle(), SQL_HANDLE_STMT);
		}

		ret = SQLSetStmtAttr(getStmtHandle(), SQL_ATTR_PARAMS_PROCESSED_PTR, params_processed.get(), 0);
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			throw OdbcException("Failed to set SQL_ATTR_PARAMS_PROCESSED_PTR", getStmtHandle(), SQL_HANDLE_STMT);
		}
	} else {
		// 1以下（単一行処理）に戻す場合は、安全のためにポインタを解除する
		ret = SQLSetStmtAttr(getStmtHandle(), SQL_ATTR_PARAMSET_SIZE, reinterpret_cast<SQLPOINTER>(static_cast<std::intptr_t>(1)), 0);
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			throw OdbcException("Failed to reset SQL_ATTR_PARAMSET_SIZE", getStmtHandle(), SQL_HANDLE_STMT);
		}
		paramset_size = 1;
		// ポインタ類は冒頭で既に NULL に設定されているため、ODBCドライバには何もしない
		param_status_array.clear();
		*params_processed = 0;
	}
}

// --- bindParamAsType (未定義型エスケープハッチ) ---

void OdbcStatement::bindParamAsType(int paramIndex, const std::optional<std::string>& opt_val, SQLSMALLINT target_sql_type, SQLULEN column_size, SQLSMALLINT decimal_digits) {
	checkSingleRowMode();
	size_t length = ((opt_val.has_value()) ? (opt_val.value().length()) : (0));
	if (length > static_cast<size_t>(std::numeric_limits<SQLLEN>::max())) {	// SQLLEN (符号付き) で表現できる最大値を超えていないかチェック
		throw std::out_of_range("String length exceeds maximum allowed size for ODBC parameters");
	}
	if (column_size <= 0) {
		column_size = std::max(1UL, length);
	}
	std::optional<std::vector<char>> opt_buffer;
	if (opt_val.has_value()) {
		const std::string& str = opt_val.value();
		opt_buffer.emplace(str.begin(), str.end());
		opt_buffer.value().push_back('\0');
	}
	bindSingleCore(
		paramIndex,
		opt_buffer,
		static_cast<SQLLEN>((opt_val.has_value()) ? (length) : (SQL_NULL_DATA)),
		ParamMetadata{SQL_PARAM_INPUT, SQL_C_CHAR, target_sql_type, column_size, std::max(static_cast<SQLSMALLINT>(0), decimal_digits), static_cast<SQLLEN>((opt_val.has_value()) ? (length + 1) : (0))}
	);
}

void OdbcStatement::bindParamAsType(int paramIndex, const std::vector<std::string>& values, SQLSMALLINT target_sql_type, SQLULEN column_size, SQLSMALLINT decimal_digits) {
	checkBatchSize(values.size());
	if (values.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
		bindParamAsType(paramIndex, values[0], target_sql_type, column_size, decimal_digits);
		return;
	}

	auto [flat_buffer, indicators, max_len] = buildStringFlatBuffer(values);
	if (column_size <= 0) {
		column_size = std::max<SQLULEN>(1UL, max_len - 1);
	}
	bindBatchCore(
		paramIndex,
		std::move(flat_buffer),
		std::move(indicators),
		ParamMetadata{SQL_PARAM_INPUT, SQL_C_CHAR, target_sql_type, column_size, std::max(static_cast<SQLSMALLINT>(0), decimal_digits), static_cast<SQLLEN>(max_len)}
	);
}

void OdbcStatement::bindParamAsType(int paramIndex, const std::vector<std::optional<std::string>>& opt_vals, SQLSMALLINT target_sql_type, SQLULEN column_size, SQLSMALLINT decimal_digits) {
	checkBatchSize(opt_vals.size());
	if (opt_vals.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
		bindParamAsType(paramIndex, opt_vals[0], target_sql_type, column_size, decimal_digits);
		return;
	}

	auto [flat_buffer, indicators, max_len] = buildStringFlatBuffer(opt_vals);
	if (column_size <= 0) {
		column_size = std::max<SQLULEN>(1UL, max_len - 1);
	}
	bindBatchCore(
		paramIndex,
		std::move(flat_buffer),
		std::move(indicators),
		ParamMetadata{SQL_PARAM_INPUT, SQL_C_CHAR, target_sql_type, column_size, std::max(static_cast<SQLSMALLINT>(0), decimal_digits), static_cast<SQLLEN>(max_len)}
	);
}

// --- 文字列バインド ---

void OdbcStatement::bindParam(int paramIndex, const char* value) {
	checkSingleRowMode();
	if (value == nullptr) {
		throw std::invalid_argument("bindParam: value must not be nullptr. Use std::optional for database NULL.");
	}
	std::string str(value);
	if (str.length() > static_cast<size_t>(std::numeric_limits<SQLLEN>::max())) {
		throw std::out_of_range("String length exceeds maximum allowed size for ODBC parameters");
	}
	bindParam(paramIndex, str);
}

void OdbcStatement::bindParam(int paramIndex, const std::optional<std::string>& opt_val) {
	checkSingleRowMode();
	size_t length = ((opt_val.has_value()) ? (opt_val.value().length()) : (0));
	if (length > static_cast<size_t>(std::numeric_limits<SQLLEN>::max())) {	// SQLLEN (符号付き) で表現できる最大値を超えていないかチェック
		throw std::out_of_range("String length exceeds maximum allowed size for ODBC parameters");
	}
	std::optional<std::vector<char>> opt_buffer;
	if (opt_val.has_value()) {
		const std::string& str = opt_val.value();
		opt_buffer.emplace(str.begin(), str.end());
		opt_buffer.value().push_back('\0');
	}
	bindSingleCore(
		paramIndex,
		opt_buffer,
		static_cast<SQLLEN>((opt_val.has_value()) ? (length) : (SQL_NULL_DATA)),
		ParamMetadata{SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, std::max(1UL, length), 0, static_cast<SQLLEN>((opt_val.has_value()) ? (length + 1) : (0))}
	);
}

void OdbcStatement::bindParam(int paramIndex, const std::vector<std::string>& values) {
	checkBatchSize(values.size());
	if (values.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
		bindParam(paramIndex, values[0]);
		return;
	}

	auto [flat_buffer, indicators, max_len] = buildStringFlatBuffer(values);
	bindBatchCore(
		paramIndex,
		std::move(flat_buffer),
		std::move(indicators),
		ParamMetadata{SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, std::max<SQLULEN>(1UL, max_len - 1), 0, static_cast<SQLLEN>(max_len)}
	);
}

void OdbcStatement::bindParam(int paramIndex, const std::vector<std::optional<std::string>>& opt_vals) {
	checkBatchSize(opt_vals.size());
	if (opt_vals.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
		bindParam(paramIndex, opt_vals[0]);
		return;
	}

	auto [flat_buffer, indicators, max_len] = buildStringFlatBuffer(opt_vals);
	bindBatchCore(
		paramIndex,
		std::move(flat_buffer),
		std::move(indicators),
		ParamMetadata{SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, std::max<SQLULEN>(1UL, max_len - 1), 0, static_cast<SQLLEN>(max_len)}
	);
}

// --- 日時型バインド ---

void OdbcStatement::bindParam(int paramIndex, const std::optional<SQL_TIMESTAMP_STRUCT>& opt_val, SQLSMALLINT precision) {
	checkSingleRowMode();
	bindSingleCore(
		paramIndex,
		opt_val,
		static_cast<SQLLEN>((opt_val.has_value()) ? sizeof(SQL_TIMESTAMP_STRUCT) : SQL_NULL_DATA),
		ParamMetadata{SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, ((precision > 0) ? (20UL + static_cast<SQLULEN>(precision)) : (19UL)), std::max(static_cast<SQLSMALLINT>(0), precision), static_cast<SQLLEN>(sizeof(SQL_TIMESTAMP_STRUCT))}
	);
}

void OdbcStatement::bindParam(int paramIndex, const std::vector<SQL_TIMESTAMP_STRUCT>& values, SQLSMALLINT precision) {
	checkBatchSize(values.size());
	if (values.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
		bindParam(paramIndex, values[0], precision);
		return;
	}

	bindBatchCore(
		paramIndex,
		values,
		std::vector<SQLLEN>(values.size(), static_cast<SQLLEN>(sizeof(SQL_TIMESTAMP_STRUCT))),
		ParamMetadata{SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, ((precision > 0) ? (20UL + static_cast<SQLULEN>(precision)) : (19UL)), std::max(static_cast<SQLSMALLINT>(0), precision), static_cast<SQLLEN>(sizeof(SQL_TIMESTAMP_STRUCT))}
	);
}

void OdbcStatement::bindParam(int paramIndex, const std::vector<std::optional<SQL_TIMESTAMP_STRUCT>>& opt_vals, SQLSMALLINT precision) {
	checkBatchSize(opt_vals.size());
	if (opt_vals.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
		bindParam(paramIndex, opt_vals[0], precision);
		return;
	}

	// 1. データ配列とインジケータ配列を作成
	std::vector<SQL_TIMESTAMP_STRUCT> buffer(opt_vals.size());
	std::vector<SQLLEN> indicators(opt_vals.size());
	for (size_t i = 0; i < opt_vals.size(); i++) {
		if (opt_vals[i].has_value()) {
			buffer[i] = opt_vals[i].value();
			// 固定長データの場合、インジケータは0で良いが、一応サイズをセットしておく
			indicators[i] = static_cast<SQLLEN>(sizeof(SQL_TIMESTAMP_STRUCT));
		} else {
			// buffer[i] はデフォルト値(0など)のまま
			indicators[i] = SQL_NULL_DATA;
		}
	}
	bindBatchCore(
		paramIndex,
		std::move(buffer),
		std::move(indicators),
		ParamMetadata{SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, ((precision > 0) ? (20UL + static_cast<SQLULEN>(precision)) : (19UL)), std::max(static_cast<SQLSMALLINT>(0), precision), static_cast<SQLLEN>(sizeof(SQL_TIMESTAMP_STRUCT))}
	);
}

void OdbcStatement::bindParam(int paramIndex, const std::optional<std::chrono::system_clock::time_point>& opt_val, SQLSMALLINT precision) {
	checkSingleRowMode();
	std::optional<SQL_TIMESTAMP_STRUCT> ts_opt;
	if (opt_val.has_value()) {
		// 値がある場合のみ、C++のtime_pointをODBC用のTIMESTAMP構造体に変換して、変換後の構造体の実体とサイズをマップに格納
		if (!(ts_opt = to_sql_timestamp(opt_val.value()))) {
			throw std::invalid_argument("Failed to convert time_point to SQL_TIMESTAMP_STRUCT.");
		}
	}
	// SQL_TIMESTAMP_STRUCT 版へ委譲
	bindParam(paramIndex, ts_opt, precision);
}

void OdbcStatement::bindParam(int paramIndex, const std::vector<std::chrono::system_clock::time_point>& values, SQLSMALLINT precision) {
	checkBatchSize(values.size());
	if (values.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
		bindParam(paramIndex, values[0], precision);
		return;
	}

	// time_pointのvectorを、TIMESTAMP_STRUCTのvectorに変換する
	std::vector<SQL_TIMESTAMP_STRUCT> buffer(values.size());

	for (size_t i = 0; i < values.size(); i++) {
		if (auto ts_opt = to_sql_timestamp(values[i])) {
			buffer[i] = ts_opt.value();
		} else {
			throw std::invalid_argument("Failed to convert time_point to SQL_TIMESTAMP_STRUCT at index " + std::to_string(i) + ".");
		}
	}

	// 変換後のバッファを保存
	bindBatchCore(
		paramIndex,
		std::move(buffer),
		std::vector<SQLLEN>(values.size(), static_cast<SQLLEN>(sizeof(SQL_TIMESTAMP_STRUCT))),
		ParamMetadata{SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, ((precision > 0) ? (20UL + static_cast<SQLULEN>(precision)) : (19UL)), std::max(static_cast<SQLSMALLINT>(0), precision), static_cast<SQLLEN>(sizeof(SQL_TIMESTAMP_STRUCT))}
	);
}

void OdbcStatement::bindParam(int paramIndex, const std::vector<std::optional<std::chrono::system_clock::time_point>>& opt_vals, SQLSMALLINT precision) {
	checkBatchSize(opt_vals.size());
	if (opt_vals.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
		bindParam(paramIndex, opt_vals[0], precision);
		return;
	}

	// 1. データ配列とインジケータ配列を作成
	std::vector<SQL_TIMESTAMP_STRUCT> buffer(opt_vals.size());
	std::vector<SQLLEN> indicators(opt_vals.size());
	for (size_t i = 0; i < opt_vals.size(); i++) {
		if (opt_vals[i].has_value()) {
			if (auto ts_opt = to_sql_timestamp(opt_vals[i].value())) {
				buffer[i] = ts_opt.value();
				// 固定長データの場合、インジケータは0で良いが、一応サイズをセットしておく
				indicators[i] = sizeof(SQL_TIMESTAMP_STRUCT);
			} else {
				throw std::invalid_argument("Failed to convert time_point to SQL_TIMESTAMP_STRUCT at index " + std::to_string(i) + ".");
			}
		} else {
			// buffer[i] はデフォルト値のまま
			indicators[i] = SQL_NULL_DATA;
		}
	}

	// 変換後のバッファを保存
	bindBatchCore(
		paramIndex,
		std::move(buffer),
		std::move(indicators),
		ParamMetadata{SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, ((precision > 0) ? (20UL + static_cast<SQLULEN>(precision)) : (19UL)), std::max(static_cast<SQLSMALLINT>(0), precision), static_cast<SQLLEN>(sizeof(SQL_TIMESTAMP_STRUCT))}
	);
}

// --- long double (非推奨処理) ---

void OdbcStatement::bindParam(int paramIndex, const std::optional<long double>& opt_val) {
	LogManager::logError("Warning: 'long double' is truncated to 'double' for ODBC binding.");
	checkSingleRowMode();
	if (opt_val.has_value()) {
		bindParam(paramIndex, std::make_optional(static_cast<double>(opt_val.value())));
	} else {
		bindParam(paramIndex, std::optional<double>());
	}
}

void OdbcStatement::bindParam(int paramIndex, const std::vector<long double>& values) {
	LogManager::logError("Warning: 'long double' is truncated to 'double' for ODBC binding.");
	checkBatchSize(values.size());
	if (values.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
		// double に明示的にキャストして、非推奨関数を回避して委譲する
		bindParam(paramIndex, static_cast<double>(values[0]));
		return;
	}

	std::vector<double> d_vals(values.size());
	for (size_t i = 0; i < values.size(); i++) {
		d_vals[i] = static_cast<double>(values[i]);
	}
	bindParam(paramIndex, d_vals);
}

void OdbcStatement::bindParam(int paramIndex, const std::vector<std::optional<long double>>& opt_vals) {
	LogManager::logError("Warning: 'long double' is truncated to 'double' for ODBC binding.");
	checkBatchSize(opt_vals.size());
	if (opt_vals.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
		// double に明示的にキャストして、非推奨関数を回避して委譲する
		if (opt_vals[0].has_value()) {
			bindParam(paramIndex, std::make_optional(static_cast<double>(opt_vals[0].value())));
		} else {
			bindParam(paramIndex, std::optional<double>());
		}
		return;
	}

	std::vector<std::optional<double>> d_vals(opt_vals.size());
	for (size_t i = 0; i < opt_vals.size(); i++) {
		if (opt_vals[i].has_value()) {
			d_vals[i] = std::make_optional(static_cast<double>(opt_vals[i].value()));
		}
	}
	bindParam(paramIndex, d_vals);
}
// === パラメータ解除・実行・バインド補助 実装 ===

SQLRETURN OdbcStatement::resetParams() noexcept {
	return SQLFreeStmt(getStmtHandle(), SQL_RESET_PARAMS);
}

void OdbcStatement::clearParams() {
	// 1. まずODBCドライバへのパラメータバインドを解除する
	SQLRETURN ret = resetParams();
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to reset parameters", getStmtHandle(), SQL_HANDLE_STMT);
	}

	// 2. resetParams が成功した場合のみ、古いバインド情報をクリアする
	param_buffers.clear();
	param_metadata.clear();
	param_indicators.clear();
	bulk_param_indicators.clear();
	resetBatchSize();
}

int OdbcStatement::executePrepared() {
	SQLRETURN ret;

	// 実行直前のポインタ評価用 RAII（いかなる例外発生時も確実にバインドを解除する）
	struct ParamUnbinder {
		std::function<void()> f;
		~ParamUnbinder() { if(f) { f(); } }
	} unbinder{[this](){ this->resetParams(); }};

	// 実行直前に、保持しておいたバッファを使って全ての列をバインドする
	for (auto& [index, buffer_ptr] : param_buffers) {
		const auto& meta = param_metadata.at(index);

		// 実行直前に仮想関数経由でポインタを解決
		SQLPOINTER data_ptr = buffer_ptr->getPointer();

		// モードに応じてインジケータ配列のポインタを取得
		SQLLEN* ind_ptr = nullptr;
		if (paramset_size > 1) {
			ind_ptr = bulk_param_indicators.at(index).data();
		} else {
			ind_ptr = &param_indicators.at(index);
		}

		// バッファから得たポインタとメタ情報を使って、SQLBindParameter を呼び出す
		ret = SQLBindParameter(
			getStmtHandle(),
			index,
			meta.io_type,
			meta.c_type,
			meta.sql_type,
			meta.col_size,
			meta.decimal_digits,
			data_ptr,
			meta.buf_length,
			ind_ptr
		);
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			throw OdbcException("Failed to bind parameter at execution", getStmtHandle(), SQL_HANDLE_STMT);
		}
	}

	// SQLを実行
	ret = SQLExecute(getStmtHandle());

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA) {
		throw OdbcException("Failed to execute prepared statement", getStmtHandle(), SQL_HANDLE_STMT);
	}
	return getAffectedRowCount();
}

int OdbcStatement::getAffectedRowCount() {
	SQLLEN rowCount = 0;
	SQLRETURN ret = SQLRowCount(getStmtHandle(), &rowCount);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to get affected row count", getStmtHandle(), SQL_HANDLE_STMT);
	}
	return static_cast<int>(rowCount);
}

void OdbcStatement::initColumnMap() {
	if (column_map_initialized) { return; }
	SQLRETURN ret;

	column_map_cache.clear();

	// 1. まず結果セットの列数を取得する
	SQLSMALLINT columnCount = 0;
	ret = SQLNumResultCols(getStmtHandle(), &columnCount);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to get number of result columns", getStmtHandle(), SQL_HANDLE_STMT);
	}

	// 2. ループに入る前に、取得した列数分を予約する
	if (columnCount > 0) {
		column_map_cache.reserve(static_cast<size_t>(columnCount));
	}

	// 3. 列数分ループして、各列の名前を取得し、小文字に変換してキャッシュする
	for (SQLUSMALLINT i = 1; i <= columnCount; i++) {
		SQLSMALLINT nameLen = 0;
		// まず名前の長さだけを取得する (バッファを 0 にして呼び出す)
		ret = SQLDescribeCol(getStmtHandle(), i, NULL, 0, &nameLen, NULL, NULL, NULL, NULL);
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			throw OdbcException("Failed to describe column for column map initialization", getStmtHandle(), SQL_HANDLE_STMT);
		}
		if (nameLen == 0) {
			nameLen = 255;
		}

		// 必要なサイズでバッファを確保 (+1 は終端文字用)
		std::vector<SQLCHAR> nameBuffer(nameLen + 1);
		ret = SQLDescribeCol(getStmtHandle(), i, nameBuffer.data(), nameBuffer.size(), &nameLen, NULL, NULL, NULL, NULL);
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			throw OdbcException("Failed to describe column for column map initialization", getStmtHandle(), SQL_HANDLE_STMT);
		}

		// データベースの違い（大文字小文字の揺れ）を吸収するため、全て小文字に変換してキャッシュ
		std::string originalName = reinterpret_cast<char*>(nameBuffer.data());
		std::string lowerName(originalName.size(), '\0');
		std::transform(originalName.begin(), originalName.end(), lowerName.begin(),
			[](unsigned char c) { return std::tolower(c); }
		);

		// すでに小文字化した名前が登録されているかチェック
		if (column_map_cache.find(lowerName) != column_map_cache.end()) {
			// 【注意】同じ名前の列がすでに存在する場合は、ODBCドライバによっては列名の大文字小文字を区別せずに返すことがあるため、警告を出力します。
			LogManager::logError(utils::concat("[ODBC Warning] Duplicate column name (case-insensitive): ", originalName, " at index ", std::to_string(i)));
			// 必要ならここで例外を投げても良いですが、警告にとどめるのが実用的です
		}

		column_map_cache.emplace(lowerName, i);
	}
	column_map_initialized = true;
}
