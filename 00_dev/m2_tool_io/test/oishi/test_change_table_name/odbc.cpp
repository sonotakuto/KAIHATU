/**
 * @file odbc.cpp
 * @brief odbc.hppで宣言されたクラスのメンバ関数の実装ファイル。
 */
#include "odbc.hpp"
#include <cstring>
#include <iostream>

using namespace odbc;

//============================================
// ヘルパー関数
//============================================

std::optional<SQL_TIMESTAMP_STRUCT> odbc::to_sql_timestamp(const std::chrono::system_clock::time_point& tp) {
	std::time_t timet = std::chrono::system_clock::to_time_t(tp);
	std::tm timeinfo;

	if (localtime_r(&timet, &timeinfo) == nullptr) {
		// 失敗することは稀ですが、エラーハンドリング
		std::cerr << "localtime_r failed" << std::endl;
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
	SQLCHAR sqlState[6];
	SQLINTEGER nativeError;
	SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
	SQLSMALLINT textLength;
	std::string errorMsg = "No specific error message available.";

	// 診断レコードの1番目を取得する
	if (SQLGetDiagRec(handleType, handle, 1, sqlState, &nativeError, message, sizeof(message), &textLength) == SQL_SUCCESS) {
		errorMsg =	"SQLSTATE: " + std::string(reinterpret_cast<char*>(sqlState)) +
					", Native Error: " + std::to_string(nativeError) +
					", Message: " + std::string(reinterpret_cast<char*>(message));
	}
	return errorMsg;
}

OdbcException::OdbcException(const std::string& msg, SQLHANDLE handle, SQLSMALLINT handleType)
	: std::runtime_error(msg + " - " + getOdbcError(handle, handleType)) {}

//============================================
// OdbcConnection
//============================================

OdbcConnection::OdbcConnection(const std::string& dsn, const std::string& user, const std::string& pass) {
	// 1. 環境ハンドルの割り当て
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	// 2. 使用するODBCのバージョンを3.xに設定
	SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
	// 3. 接続ハンドルの割り当て
	SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
	// 4. データベースへの接続実行
	SQLRETURN ret = SQLConnect(hdbc, (SQLCHAR*)dsn.c_str(), SQL_NTS, (SQLCHAR*)user.c_str(), SQL_NTS, (SQLCHAR*)pass.c_str(), SQL_NTS);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to connect to database", hdbc, SQL_HANDLE_DBC);
	}
}

OdbcConnection::~OdbcConnection() {
	if (hdbc != SQL_NULL_HDBC) {
		SQLDisconnect(hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	}
	if (henv != SQL_NULL_HENV) {
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
}

SQLHDBC OdbcConnection::getHandle() const {
	return hdbc;
}

void OdbcConnection::beginTransaction() {
	SQLRETURN ret = SQLSetConnectAttr(hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to begin transaction (disable autocommit)", hdbc, SQL_HANDLE_DBC);
	}
}

void OdbcConnection::commit() {
	SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT);
	// オートコミットを有効に戻す
	SQLSetConnectAttr(hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to commit transaction", hdbc, SQL_HANDLE_DBC);
	}
}

void OdbcConnection::rollback() {
	SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_ROLLBACK);
	// オートコミットを有効に戻す
	SQLSetConnectAttr(hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to rollback transaction", hdbc, SQL_HANDLE_DBC);
	}
}

//============================================
// OdbcStatement
//============================================

OdbcStatement::OdbcStatement(const OdbcConnection& conn) {
	SQLAllocHandle(SQL_HANDLE_STMT, conn.getHandle(), &hstmt);
}

OdbcStatement::~OdbcStatement() {
	if (hstmt != SQL_NULL_HSTMT) {
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	}
}

int OdbcStatement::execute(const std::string& sql) {
	SQLRETURN ret = SQLExecDirect(hstmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to execute query", hstmt, SQL_HANDLE_STMT);
	}
	return getAffectedRowCount();
}

bool OdbcStatement::fetch() {
	SQLRETURN ret = SQLFetch(hstmt);

	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
		return true;	// 次の行の取得に成功
	}
	if (ret == SQL_NO_DATA) {
		return false;	// 結果セットの終端に到達
	}
	// 上記以外は予期せぬエラー
	throw OdbcException("Failed to fetch row", hstmt, SQL_HANDLE_STMT);
}

// --- getData<T>のテンプレート特殊化実装群 ---

template<> std::optional<std::string> OdbcStatement::getData<std::string>(int col) {
	const int BUFFER_SIZE = 1024;
	char buffer[BUFFER_SIZE];
	SQLLEN indicator;
	// 固定長バッファに文字列を読み込む
	SQLRETURN ret = SQLGetData(hstmt, col, SQL_C_CHAR, buffer, BUFFER_SIZE, &indicator);
	if (ret == SQL_NO_DATA || indicator == SQL_NULL_DATA) {
		return std::nullopt;
	}
	// バッファサイズを超える巨大な文字列は切り捨てられる可能性がある
	return std::string(buffer);
}

template<> std::optional<std::vector<char>> OdbcStatement::getData<std::vector<char>>(int col) {
	std::vector<char> buffer;
	SQLLEN indicator;
	SQLRETURN ret;
	const int CHUNK_SIZE = 4096;	// 一度に読み込むサイズ
	char chunk[CHUNK_SIZE];

	// まずデータがNULLでないかを確認するために、バッファ長0で呼び出す
	ret = SQLGetData(hstmt, col, SQL_C_BINARY, chunk, 0, &indicator);
	if (indicator == SQL_NULL_DATA) {
		return std::nullopt;
	}

	// データがNULLでなければ、SQL_NO_DATAが返されるまでチャンク単位で全て読み出す
	while ((ret = SQLGetData(hstmt, col, SQL_C_BINARY, chunk, CHUNK_SIZE, &indicator)) != SQL_NO_DATA) {
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			return std::nullopt;	// 読み取り中にエラー
		}
		// indicatorには実際に読み取ったバイト数が入る
		long bytesInChunk = (indicator > CHUNK_SIZE || indicator < 0) ? (CHUNK_SIZE) : (indicator);
		buffer.insert(buffer.end(), chunk, chunk + bytesInChunk);
	}
	return buffer;
}

template<> std::optional<std::chrono::system_clock::time_point> OdbcStatement::getData<std::chrono::system_clock::time_point>(int col) {
	SQL_TIMESTAMP_STRUCT ts;
	SQLLEN indicator;
	SQLGetData(hstmt, col, SQL_C_TYPE_TIMESTAMP, &ts, sizeof(ts), &indicator);	// ODBCのTIMESTAMP構造体で取得
	if (indicator == SQL_NULL_DATA) {
		return std::nullopt;
	}
	return to_time_point(ts);
}

void OdbcStatement::closeCursor() {
	SQLRETURN ret = SQLCloseCursor(hstmt);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		// 失敗した場合、それが「既に閉じている」エラー(24000)か確認する
		SQLCHAR sqlState[6];
		SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlState, NULL, NULL, 0, NULL);
		// '24000'でなければ、それは予期せぬエラーなので例外をスロー
		if (std::string(reinterpret_cast<char*>(sqlState)) != "24000") {
			throw OdbcException("Failed to close cursor with an unexpected error", hstmt, SQL_HANDLE_STMT);
		}
		// '24000'の場合は何もしない（べき等性を保証）
	}
}

std::vector<std::map<std::string, std::any>> OdbcStatement::fetchAll() {
	std::vector<std::map<std::string, std::any>> allRows;
	// 1. 結果セットの列情報を取得 (列数、名前、型)
	SQLSMALLINT columnCount = 0;
	SQLNumResultCols(hstmt, &columnCount);
	std::vector<std::string> columnNames;
	std::vector<SQLSMALLINT> columnTypes;
	for (SQLUSMALLINT i = 1; i <= columnCount; i++) {
		SQLCHAR columnName[256];
		SQLSMALLINT dataType;
		SQLDescribeCol(hstmt, i, columnName, sizeof(columnName), NULL, &dataType, NULL, NULL, NULL);
		columnNames.push_back(std::string(reinterpret_cast<char*>(columnName)));
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
				auto data = this->getData<std::vector<char>>(i);
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

// --- プリペアドステートメント関連 ---

void OdbcStatement::prepare(const std::string& sql) {
	// 1. ODBCドライバにSQL文をコンパイルさせる
	SQLRETURN ret = SQLPrepare(hstmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to prepare statement", hstmt, SQL_HANDLE_STMT);
	}
	// 2. prepareが成功した場合のみ、古いバインド情報をクリアする
	bound_params.clear();
	param_indicators.clear();
	bulk_param_buffers.clear();
	bulk_param_indicators.clear();
	bulk_param_strides.clear();
	bulk_param_sql_types.clear();
	paramset_size = 0;
}

void OdbcStatement::bind(int paramIndex, const std::optional<std::string>& opt_val) {
	param_indicators.insert_or_assign(paramIndex, ((opt_val.has_value()) ? (opt_val.value().length()) : (SQL_NULL_DATA)));
	bound_params.insert_or_assign(paramIndex, opt_val);
}

void OdbcStatement::bind(int paramIndex, const std::optional<std::vector<char>>& opt_val) {
	param_indicators.insert_or_assign(paramIndex, ((opt_val.has_value()) ? (opt_val.value().size()) : (SQL_NULL_DATA)));
	bound_params.insert_or_assign(paramIndex, opt_val);
}

bool OdbcStatement::bind(int paramIndex, const std::optional<std::chrono::system_clock::time_point>& opt_val) {
	if (opt_val.has_value()) {
		// 値がある場合のみ、C++のtime_pointをODBC用のTIMESTAMP構造体に変換して、変換後の構造体の実体とサイズをマップに格納
		if (std::optional<SQL_TIMESTAMP_STRUCT> ts_opt = to_sql_timestamp(opt_val.value())) {
			param_indicators.insert_or_assign(paramIndex, sizeof(SQL_TIMESTAMP_STRUCT));	// 長さ情報をここで保存
			bound_params.insert_or_assign(paramIndex, std::move(ts_opt));
		} else {
			return false;
		}
	} else {
		param_indicators.insert_or_assign(paramIndex, SQL_NULL_DATA);	// 長さ情報をここで保存
		// 空の optional<SQL_TIMESTAMP_STRUCT> を格納 (型情報を保持するため)
		bound_params.insert_or_assign(paramIndex, std::optional<SQL_TIMESTAMP_STRUCT>());
	}
	return true;
}

void OdbcStatement::bind(int paramIndex, const std::optional<long double>& opt_val) {
	std::cerr << "Warning: 'long double' is truncated to 'double' for ODBC binding." << std::endl;
	if (opt_val.has_value()) {
		bound_params.insert_or_assign(paramIndex, std::make_optional(static_cast<double>(opt_val.value())));
	} else {
		param_indicators.insert_or_assign(paramIndex, SQL_NULL_DATA);
		bound_params.insert_or_assign(paramIndex, std::optional<double>());
	}
}

void OdbcStatement::setParamsetSize(size_t size) {
	paramset_size = size;
	// SQL_ATTR_PARAMSET_SIZEを設定し、一括実行モードをドライバに通知する
	SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)paramset_size, 0);
}

void OdbcStatement::bindColumn(int paramIndex, const std::vector<std::string>& values) {
	// 1. 全ての文字列を格納できる最大の長さを特定（+1は終端文字用）
	size_t max_len = 0;
	for (const auto& value : values) {
		if (value.length() > max_len) {
			max_len = value.length();
		}
	}
	max_len += 1;	// 終端文字('\0')のためのスペースを確保

	// 2. 連続したメモリブロックを確保
	std::vector<char> flat_buffer(values.size() * max_len, 0);
	bulk_param_indicators[paramIndex].resize(values.size());

	// 3. 各文字列をメモリブロックの正しい位置にコピーする
	for (size_t i = 0; i < values.size(); i++) {
		// strncpyは安全だが、ここではmemcpyの方がパフォーマンスが良い
		memcpy(flat_buffer.data() + (i * max_len), values[i].c_str(), values[i].length());
		bulk_param_indicators[paramIndex][i] = values[i].length();
	}

	// 4. 構築した連続メモリブロックと、各部屋の大きさ（ストライド）を保存
	bulk_param_buffers.insert_or_assign(paramIndex, std::move(flat_buffer));
	bulk_param_strides.insert_or_assign(paramIndex, std::move(max_len));
	bulk_param_sql_types.insert_or_assign(paramIndex, SQL_VARCHAR);
}

void OdbcStatement::bindColumn(int paramIndex, const std::vector<std::optional<std::string>>& opt_vals) {
	// 1. 全ての文字列を格納できる最大の長さを特定（+1は終端文字用）
	size_t max_len = 0;
	for (const auto& opt_val : opt_vals) {
		if (opt_val.has_value() && opt_val.value().length() > max_len) {
			max_len = opt_val.value().length();
		}
	}
	max_len += 1;	// 終端文字('\0')のためのスペースを確保

	// 2. 連続したメモリブロックを確保
	std::vector<char> flat_buffer(opt_vals.size() * max_len, 0);
	bulk_param_indicators[paramIndex].resize(opt_vals.size());

	// 3. 各文字列をメモリブロックの正しい位置にコピーする
	for (size_t i = 0; i < opt_vals.size(); i++) {
		if (opt_vals[i].has_value()) {
			memcpy(flat_buffer.data() + (i * max_len), opt_vals[i].value().c_str(), opt_vals[i].value().length());
			bulk_param_indicators[paramIndex][i] = opt_vals[i].value().length();
		} else {
			bulk_param_indicators[paramIndex][i] = SQL_NULL_DATA;
		}
	}

	// 4. 構築した連続メモリブロックと、各部屋の大きさ（ストライド）を保存
	bulk_param_buffers.insert_or_assign(paramIndex, std::move(flat_buffer));
	bulk_param_strides.insert_or_assign(paramIndex, std::move(max_len));
	bulk_param_sql_types.insert_or_assign(paramIndex, SQL_VARCHAR);
}

void OdbcStatement::bindColumn(int paramIndex, const std::vector<std::vector<char>>& values) {
	// 1. 全てのBLOBを格納できる最大の長さを特定
	size_t max_len = 0;
	for (const auto& value : values) {
		if (value.size() > max_len) {
			max_len = value.size();
		}
	}
	if (max_len == 0) max_len = 1;

	// 2. 連続したメモリブロックを確保
	std::vector<char> flat_buffer(values.size() * max_len, 0);
	bulk_param_indicators[paramIndex].resize(values.size());

	// 3. 各文字列をメモリブロックの正しい位置にコピーする
	for (size_t i = 0; i < values.size(); i++) {
		if (!values[i].empty()) {
			memcpy(flat_buffer.data() + (i * max_len), values[i].data(), values[i].size());
		}
		bulk_param_indicators[paramIndex][i] = values[i].size();
	}

	// 4. 構築した連続メモリブロックと、各部屋の大きさ（ストライド）を保存
	bulk_param_buffers.insert_or_assign(paramIndex, std::move(flat_buffer));
	bulk_param_strides.insert_or_assign(paramIndex, std::move(max_len));
	bulk_param_sql_types.insert_or_assign(paramIndex, SQL_LONGVARBINARY);
}

void OdbcStatement::bindColumn(int paramIndex, const std::vector<std::optional<std::vector<char>>>& opt_vals) {
	// 1. 全てのBLOBを格納できる最大の長さを特定
	size_t max_len = 0;
	for (const auto& opt_val : opt_vals) {
		if (opt_val.has_value() && opt_val.value().size() > max_len) {
			max_len = opt_val.value().size();
		}
	}
	if (max_len == 0) max_len = 1;

	// 2. 連続したメモリブロックを確保
	std::vector<char> flat_buffer(opt_vals.size() * max_len, 0);
	bulk_param_indicators[paramIndex].resize(opt_vals.size());

	// 3. 各文字列をメモリブロックの正しい位置にコピーする
	for (size_t i = 0; i < opt_vals.size(); i++) {
		if (opt_vals[i].has_value()) {
			if (!opt_vals[i].value().empty()) {
				memcpy(flat_buffer.data() + (i * max_len), opt_vals[i].value().data(), opt_vals[i].value().size());
			}
			bulk_param_indicators[paramIndex][i] = opt_vals[i].value().size();
		} else {
			bulk_param_indicators[paramIndex][i] = SQL_NULL_DATA;
		}
	}

	// 4. 構築した連続メモリブロックと、各部屋の大きさ（ストライド）を保存
	bulk_param_buffers.insert_or_assign(paramIndex, std::move(flat_buffer));
	bulk_param_strides.insert_or_assign(paramIndex, std::move(max_len));
	bulk_param_sql_types.insert_or_assign(paramIndex, SQL_LONGVARBINARY);
}

bool OdbcStatement::bindColumn(int paramIndex, const std::vector<std::chrono::system_clock::time_point>& values) {
	// time_pointのvectorを、TIMESTAMP_STRUCTのvectorに変換する
	std::vector<SQL_TIMESTAMP_STRUCT> buffer(values.size());
	bulk_param_indicators[paramIndex].resize(values.size());

	for (size_t i = 0; i < values.size(); i++) {
		if (auto ts_opt = to_sql_timestamp(values[i])) {
			buffer[i] = ts_opt.value();
			bulk_param_indicators[paramIndex][i] = sizeof(SQL_TIMESTAMP_STRUCT);
		} else {
			return false;
		}
	}

	// 変換後のバッファを保存
	bulk_param_buffers.insert_or_assign(paramIndex, std::move(buffer));
	return true;
}

bool OdbcStatement::bindColumn(int paramIndex, const std::vector<std::optional<std::chrono::system_clock::time_point>>& opt_vals) {
	std::vector<SQL_TIMESTAMP_STRUCT> buffer(opt_vals.size());
	bulk_param_indicators[paramIndex].resize(opt_vals.size());

	for (size_t i = 0; i < opt_vals.size(); i++) {
		if (opt_vals[i].has_value()) {
			if (auto ts_opt = to_sql_timestamp(opt_vals[i].value())) {
				buffer[i] = ts_opt.value();
				bulk_param_indicators[paramIndex][i] = sizeof(SQL_TIMESTAMP_STRUCT);
			} else {
				return false;
			}
		} else {
			// buffer[i] はデフォルト値のまま
			bulk_param_indicators[paramIndex][i] = SQL_NULL_DATA;
		}
	}

	// 変換後のバッファを保存
	bulk_param_buffers.insert_or_assign(paramIndex, std::move(buffer));
	return true;
}

void OdbcStatement::bindColumn(int paramIndex, const std::vector<long double>& values) {
	bulk_param_indicators.insert_or_assign(paramIndex, std::vector<SQLLEN>(values.size(), 0));
	bulk_param_buffers.insert_or_assign(paramIndex, std::vector<double>(values.begin(), values.end()));
}

void OdbcStatement::bindColumn(int paramIndex, const std::vector<std::optional<long double>>& opt_vals) {
	// 1. データ配列とインジケータ配列を作成
	std::vector<double> data_buffer(opt_vals.size());
	bulk_param_indicators[paramIndex].resize(opt_vals.size());
	for (size_t i = 0; i < opt_vals.size(); i++) {
		if (opt_vals[i].has_value()) {
			data_buffer[i] = static_cast<double>(opt_vals[i].value());
			// 固定長データの場合、インジケータは0で良い
			bulk_param_indicators[paramIndex][i] = 0;
		} else {
			// data_buffer[i] はデフォルト値(0など)のまま
			bulk_param_indicators[paramIndex][i] = SQL_NULL_DATA;
		}
	}
	// 2. 作成したデータ配列をバッファに保存
	bulk_param_buffers.insert_or_assign(paramIndex, data_buffer);
}

/**
 * @brief (Helper) 単一行用の固定長パラメータをバインドします。
 */
template<typename T>
void OdbcStatement::bindFixedSizeParam(int index, const std::any& any_val, SQLLEN* indicator_ptr) {
	// any_val は const std::any& なので、const std::optional<T>& でキャスト
	const std::optional<T>& opt_val = std::any_cast<const std::optional<T>&>(any_val);

	// SQLBindParameter は void* (SQLPOINTER) を要求するため、const_cast が必要です。
	// このポインタ経由で値が変更されることはありません。
	SQLPOINTER data_ptr = (opt_val.has_value()) ? (static_cast<SQLPOINTER>(const_cast<T*>(&opt_val.value()))) : (NULL);

	// インジケータポインタは、値がNULLの場合(data_ptrがNULL)のみ使用されます。
	SQLLEN* final_indicator_ptr = (opt_val.has_value()) ? NULL : indicator_ptr;

	// ★ 型特性を使って C型 と SQL型 を取得
	constexpr SQLSMALLINT c_type = OdbcTypeTraits<T>::c_type;
	constexpr SQLSMALLINT sql_type = OdbcTypeTraits<T>::sql_type;
	SQLULEN col_size = 0;
	SQLSMALLINT decimal_digits = 0;

	// 日時型の場合のみ、インジケータからサイズを取得
	if constexpr (std::is_same_v<T, SQL_TIMESTAMP_STRUCT>) {
		col_size = (opt_val.has_value()) ? *indicator_ptr : 0;
	}

	SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, c_type, sql_type, col_size, decimal_digits, data_ptr, 0, final_indicator_ptr);
}


/**
 * @brief (Helper) 一括実行用の固定長パラメータをバインドします。
 */
template<typename T>
void OdbcStatement::bindFixedSizeColumn(int index, std::any& any_val, std::vector<SQLLEN>& indicators) {
	// 元のコードのロジックに基づき、非constの std::vector<T>& でキャストします。
	// (この関数内で vec の中身が変更されることはありません)
	std::vector<T>& vec = std::any_cast<std::vector<T>&>(any_val);

	SQLPOINTER data_ptr = (SQLPOINTER)vec.data();	// T* が返る

	// ★ 型特性を使って C型 と SQL型 を取得
	constexpr SQLSMALLINT c_type = OdbcTypeTraits<T>::c_type;
	constexpr SQLSMALLINT sql_type = OdbcTypeTraits<T>::sql_type;
	SQLULEN col_size = 0;


	// 日時型の場合のみ、構造体のサイズを設定
	if constexpr (std::is_same_v<T, SQL_TIMESTAMP_STRUCT>) {
		col_size = sizeof(SQL_TIMESTAMP_STRUCT);
	}

	SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, c_type, sql_type, col_size, 0, data_ptr, 0, indicators.data());
}

int OdbcStatement::executePrepared() {
	if (paramset_size > 0) {
		// 一括実行モード: バインドはbind_columnで完了済み
		// 実行直前に、保持しておいたバッファを使って全ての列をバインドする
		for (auto const& [index, _] : bulk_param_buffers) {
			// at() で std::any& (非const) を取得
			std::any& any_val = bulk_param_buffers.at(index);

			// --- 固定長データ (ヘルパー関数) ---
			if (any_val.type() == typeid(std::vector<signed char>)) {
				bindFixedSizeColumn<signed char>(index, any_val, bulk_param_indicators.at(index));
			} else if (any_val.type() == typeid(std::vector<unsigned char>)) {
				bindFixedSizeColumn<unsigned char>(index, any_val, bulk_param_indicators.at(index));
			} else if (any_val.type() == typeid(std::vector<short>)) {
				bindFixedSizeColumn<short>(index, any_val, bulk_param_indicators.at(index));
			} else if (any_val.type() == typeid(std::vector<unsigned short>)) {
				bindFixedSizeColumn<unsigned short>(index, any_val, bulk_param_indicators.at(index));
			} else if (any_val.type() == typeid(std::vector<int>)) {
				bindFixedSizeColumn<int>(index, any_val, bulk_param_indicators.at(index));
			} else if (any_val.type() == typeid(std::vector<unsigned int>)) {
				bindFixedSizeColumn<unsigned int>(index, any_val, bulk_param_indicators.at(index));
			} else if (any_val.type() == typeid(std::vector<long>)) {
				bindFixedSizeColumn<long>(index, any_val, bulk_param_indicators.at(index));
			} else if (any_val.type() == typeid(std::vector<unsigned long>)) {
				bindFixedSizeColumn<unsigned long>(index, any_val, bulk_param_indicators.at(index));
			} else if (any_val.type() == typeid(std::vector<long long>)) {
				bindFixedSizeColumn<long long>(index, any_val, bulk_param_indicators.at(index));
			} else if (any_val.type() == typeid(std::vector<unsigned long long>)) {
				bindFixedSizeColumn<unsigned long long>(index, any_val, bulk_param_indicators.at(index));
			} else if (any_val.type() == typeid(std::vector<float>)) {
				bindFixedSizeColumn<float>(index, any_val, bulk_param_indicators.at(index));
			} else if (any_val.type() == typeid(std::vector<double>)) {
				bindFixedSizeColumn<double>(index, any_val, bulk_param_indicators.at(index));
			} else if (any_val.type() == typeid(std::vector<SQL_TIMESTAMP_STRUCT>)) {
				bindFixedSizeColumn<SQL_TIMESTAMP_STRUCT>(index, any_val, bulk_param_indicators.at(index));

			// --- 可変長データ (特別扱い) ---
			} else if (any_val.type() == typeid(std::vector<char>)) { // string または BLOB
				auto& vec = std::any_cast<std::vector<char>&>(any_val);
				size_t stride = bulk_param_strides.at(index);
				SQLSMALLINT sql_type = bulk_param_sql_types.at(index);
				SQLSMALLINT c_type = (sql_type == SQL_VARCHAR) ? SQL_C_CHAR : SQL_C_BINARY;
				SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, c_type, sql_type, stride, 0, (SQLPOINTER)vec.data(), stride, bulk_param_indicators.at(index).data());
			}
		}
	} else {
		// 単一行実行モード: 保持しているパラメータを一つずつ取り出し、SQLBindParameterで紐付ける
		for (auto const& [index, any_val] : bound_params) {
			// --- 固定長データ (ヘルパー関数) ---
			if (any_val.type() == typeid(std::optional<signed char>)) {
				bindFixedSizeParam<signed char>(index, any_val, &param_indicators.at(index));
			} else if (any_val.type() == typeid(std::optional<unsigned char>)) {
				bindFixedSizeParam<unsigned char>(index, any_val, &param_indicators.at(index));
			} else if (any_val.type() == typeid(std::optional<short>)) {
				bindFixedSizeParam<short>(index, any_val, &param_indicators.at(index));
			} else if (any_val.type() == typeid(std::optional<unsigned short>)) {
				bindFixedSizeParam<unsigned short>(index, any_val, &param_indicators.at(index));
			} else if (any_val.type() == typeid(std::optional<int>)) {
				bindFixedSizeParam<int>(index, any_val, &param_indicators.at(index));
			} else if (any_val.type() == typeid(std::optional<unsigned int>)) {
				bindFixedSizeParam<unsigned int>(index, any_val, &param_indicators.at(index));
			} else if (any_val.type() == typeid(std::optional<long>)) {
				bindFixedSizeParam<long>(index, any_val, &param_indicators.at(index));
			} else if (any_val.type() == typeid(std::optional<unsigned long>)) {
				bindFixedSizeParam<unsigned long>(index, any_val, &param_indicators.at(index));
			} else if (any_val.type() == typeid(std::optional<long long>)) {
				bindFixedSizeParam<long long>(index, any_val, &param_indicators.at(index));
			} else if (any_val.type() == typeid(std::optional<unsigned long long>)) {
				bindFixedSizeParam<unsigned long long>(index, any_val, &param_indicators.at(index));
			} else if (any_val.type() == typeid(std::optional<float>)) {
				bindFixedSizeParam<float>(index, any_val, &param_indicators.at(index));
			} else if (any_val.type() == typeid(std::optional<double>)) {
				bindFixedSizeParam<double>(index, any_val, &param_indicators.at(index));
			} else if (any_val.type() == typeid(std::optional<SQL_TIMESTAMP_STRUCT>)) {
				bindFixedSizeParam<SQL_TIMESTAMP_STRUCT>(index, any_val, &param_indicators.at(index));

			// --- 可変長データ (特別扱い) ---
			} else if (any_val.type() == typeid(std::optional<std::string>)) {
				const std::optional<std::string>& opt_val = std::any_cast<const std::optional<std::string>&>(any_val);
				SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, ((opt_val.has_value()) ? (param_indicators.at(index)) : (0)), 0, ((opt_val.has_value()) ? ((SQLPOINTER)opt_val.value().c_str()) : (NULL)), 0, &param_indicators.at(index));
			} else if (any_val.type() == typeid(std::optional<std::vector<char>>)) {
				const std::optional<std::vector<char>>& opt_val = std::any_cast<const std::optional<std::vector<char>>&>(any_val);
				SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, ((opt_val.has_value()) ? (param_indicators.at(index)) : (0)), 0, ((opt_val.has_value() && !opt_val.value().empty()) ? ((SQLPOINTER)opt_val.value().data()) : (NULL)), 0, &param_indicators.at(index));
			}
		}
	}

	// SQLを実行
	SQLRETURN ret = SQLExecute(hstmt);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA) {
		throw OdbcException("Failed to execute prepared statement", hstmt, SQL_HANDLE_STMT);
	}
	return getAffectedRowCount();
}

int OdbcStatement::getAffectedRowCount() {
	SQLLEN rowCount = 0;
	SQLRowCount(hstmt, &rowCount);
	return static_cast<int>(rowCount);
}
