/**
 * @file odbc.cpp
 * @brief odbc.hppで宣言されたクラスのメンバ関数の実装ファイル。
 */
#include "odbc.hpp"

using namespace odbc;

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
		return true; // 次の行の取得に成功
	}
	if (ret == SQL_NO_DATA) {
		return false; // 結果セットの終端に到達
	}
	// 上記以外は予期せぬエラー
	throw OdbcException("Failed to fetch row", hstmt, SQL_HANDLE_STMT);
}

// --- getData<T>のテンプレート特殊化実装群 ---

template<> std::optional<int> OdbcStatement::getData<int>(int col) {
	int value;
	SQLLEN indicator;
	SQLGetData(hstmt, col, SQL_C_LONG, &value, sizeof(value), &indicator); // 32bit符号付き整数
	if (indicator == SQL_NULL_DATA) return std::nullopt;
	return value;
}

template<> std::optional<unsigned int> OdbcStatement::getData<unsigned int>(int col) {
	unsigned int value;
	SQLLEN indicator;
	SQLGetData(hstmt, col, SQL_C_ULONG, &value, sizeof(value), &indicator); // 32bit符号なし整数
	if (indicator == SQL_NULL_DATA) return std::nullopt;
	return value;
}

template<> std::optional<long long> OdbcStatement::getData<long long>(int col) {
	long long value;
	SQLLEN indicator;
	SQLGetData(hstmt, col, SQL_C_SBIGINT, &value, sizeof(value), &indicator); // 64bit符号付き整数
	if (indicator == SQL_NULL_DATA) return std::nullopt;
	return value;
}

template<> std::optional<unsigned long long> OdbcStatement::getData<unsigned long long>(int col) {
	unsigned long long value;
	SQLLEN indicator;
	SQLGetData(hstmt, col, SQL_C_UBIGINT, &value, sizeof(value), &indicator); // 64bit符号なし整数
	if (indicator == SQL_NULL_DATA) return std::nullopt;
	return value;
}

template<> std::optional<double> OdbcStatement::getData<double>(int col) {
	double value;
	SQLLEN indicator;
	SQLGetData(hstmt, col, SQL_C_DOUBLE, &value, sizeof(value), &indicator); // 倍精度浮動小数点数
	if (indicator == SQL_NULL_DATA) return std::nullopt;
	return value;
}

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
	const int CHUNK_SIZE = 4096; // 一度に読み込むサイズ
	char chunk[CHUNK_SIZE];

	// まずデータがNULLでないかを確認するために、バッファ長0で呼び出す
	ret = SQLGetData(hstmt, col, SQL_C_BINARY, chunk, 0, &indicator);
	if (indicator == SQL_NULL_DATA) {
		return std::nullopt;
	}

	// データがNULLでなければ、SQL_NO_DATAが返されるまでチャンク単位で全て読み出す
	while ((ret = SQLGetData(hstmt, col, SQL_C_BINARY, chunk, CHUNK_SIZE, &indicator)) != SQL_NO_DATA) {
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			return std::nullopt; // 読み取り中にエラー
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
	SQLGetData(hstmt, col, SQL_C_TYPE_TIMESTAMP, &ts, sizeof(ts), &indicator); // ODBCのTIMESTAMP構造体で取得
	if (indicator == SQL_NULL_DATA) {
		return std::nullopt;
	}
	// ODBC構造体をC++のtime_pointに変換
	std::tm timeinfo = {};
	timeinfo.tm_year = ts.year - 1900;
	timeinfo.tm_mon = ts.month - 1;
	timeinfo.tm_mday = ts.day;
	timeinfo.tm_hour = ts.hour;
	timeinfo.tm_min = ts.minute;
	timeinfo.tm_sec = ts.second;
	timeinfo.tm_isdst = -1;

	std::time_t timet = std::mktime(&timeinfo);
	if (timet == -1) {
		return std::nullopt;
	}
	auto timepoint = std::chrono::system_clock::from_time_t(timet);
	timepoint += std::chrono::nanoseconds(ts.fraction); // ナノ秒の精度も加える
	return timepoint;
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
	for (SQLUSMALLINT i = 1; i <= columnCount; ++i) {
		SQLCHAR columnName[256];
		SQLSMALLINT dataType;
		SQLDescribeCol(hstmt, i, columnName, sizeof(columnName), NULL, &dataType, NULL, NULL, NULL);
		columnNames.push_back(std::string(reinterpret_cast<char*>(columnName)));
		columnTypes.push_back(dataType);
	}

	// 2. 全ての行をループし、列の型に応じて適切なgetData<T>を呼び出す
	while (this->fetch()) {
		std::map<std::string, std::any> singleRow;
		for (SQLUSMALLINT i = 1; i <= columnCount; ++i) {
			std::any value;
			switch (columnTypes[i - 1]) {
				case SQL_INTEGER: case SQL_SMALLINT: case SQL_TINYINT: {
					auto data = this->getData<int>(i);
					if (data.has_value()) value = data.value();
					break;
				}
				case SQL_BIGINT: {
					auto data = this->getData<long long>(i);
					if (data.has_value()) value = data.value();
					break;
				}
				case SQL_DOUBLE: case SQL_FLOAT: case SQL_REAL: case SQL_DECIMAL: case SQL_NUMERIC: {
					auto data = this->getData<double>(i);
					if (data.has_value()) value = data.value();
					break;
				}
				case SQL_TYPE_DATE: case SQL_TYPE_TIME: case SQL_TYPE_TIMESTAMP: {
					auto data = this->getData<std::chrono::system_clock::time_point>(i);
					if (data.has_value()) value = data.value();
					break;
				}
				case SQL_BINARY: case SQL_VARBINARY: case SQL_LONGVARBINARY: {
					auto data = this->getData<std::vector<char>>(i);
					if (data.has_value()) value = data.value();
					break;
				}
				default: { // 不明な型は文字列として取得を試みる
					auto data = this->getData<std::string>(i);
					if (data.has_value()) value = data.value();
					break;
				}
			}
			singleRow[columnNames[i - 1]] = value;
		}
		allRows.push_back(singleRow);
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
	timestamp_buffers.clear();
}

void OdbcStatement::bind(int paramIndex, const std::chrono::system_clock::time_point& value) {
	// 1. C++のtime_pointをODBC用のTIMESTAMP構造体に変換する
	std::time_t timet = std::chrono::system_clock::to_time_t(value);
	std::tm* timeinfo = std::localtime(&timet);
	// 2. 変換後のデータを、寿命が保証されるtimestamp_buffersに格納する
	timestamp_buffers.emplace_back();
	SQL_TIMESTAMP_STRUCT& ts = timestamp_buffers.back();
	ts.year = timeinfo->tm_year + 1900;
	ts.month = timeinfo->tm_mon + 1;
	ts.day = timeinfo->tm_mday;
	ts.hour = timeinfo->tm_hour;
	ts.minute = timeinfo->tm_min;
	ts.second = timeinfo->tm_sec;
	ts.fraction = std::chrono::duration_cast<std::chrono::nanoseconds>(value.time_since_epoch()).count() % 1000000000;
	// 3. 実行時まで有効なポインタをマップに保存する
	bound_params[paramIndex] = &ts;
}

int OdbcStatement::executePrepared() {
	// 1. 保持しているパラメータを一つずつ取り出し、SQLBindParameterで紐付ける
	for (auto const& [index, any_val] : bound_params) {
		if (any_val.type() == typeid(int)) {
			const int& value = std::any_cast<const int&>(any_val);
			SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&value, 0, NULL);
		} else if (any_val.type() == typeid(unsigned int)) {
			const unsigned int& value = std::any_cast<const unsigned int&>(any_val);
			SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&value, 0, NULL);
		} else if (any_val.type() == typeid(double)) {
			const double& value = std::any_cast<const double&>(any_val);
			SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE, 0, 0, (SQLPOINTER)&value, 0, NULL);
		} else if (any_val.type() == typeid(long long)) {
			const long long& value = std::any_cast<const long long&>(any_val);
			SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, (SQLPOINTER)&value, 0, NULL);
		} else if (any_val.type() == typeid(unsigned long long)) {
			const unsigned long long& value = std::any_cast<const unsigned long long&>(any_val);
			SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_UBIGINT, SQL_BIGINT, 0, 0, (SQLPOINTER)&value, 0, NULL);
		} else if (any_val.type() == typeid(std::string)) {
			const std::string& value = std::any_cast<const std::string&>(any_val);
			param_indicators[index] = value.length();
			SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, value.length(), 0, (SQLPOINTER)value.c_str(), 0, &param_indicators.at(index));
		} else if (any_val.type() == typeid(std::vector<char>)) {
			const std::vector<char>& value = std::any_cast<const std::vector<char>&>(any_val);
			param_indicators[index] = value.size();
			SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, value.size(), 0, (SQLPOINTER)value.data(), 0, &param_indicators.at(index));
		} else if (any_val.type() == typeid(SQL_TIMESTAMP_STRUCT*)) {
			SQL_TIMESTAMP_STRUCT* ts_ptr = std::any_cast<SQL_TIMESTAMP_STRUCT*>(any_val);
			param_indicators[index] = sizeof(SQL_TIMESTAMP_STRUCT);
			SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, sizeof(SQL_TIMESTAMP_STRUCT), 0, ts_ptr, 0, &param_indicators.at(index));
		}
	}
	// 2. 紐付け完了後、SQLを実行
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