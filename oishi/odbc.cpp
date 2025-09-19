/**
 * @file odbc.cpp
 * @brief odbc.hppで宣言されたクラスのメンバ関数の実装ファイル。
 */
#include "odbc.hpp" // 対応するヘッダーをインクルード

//============================================
// 例外クラスの実装
//============================================
std::string OdbcException::getOdbcError(SQLHANDLE handle, SQLSMALLINT handleType) {
	SQLCHAR sqlState[6];
	SQLINTEGER nativeError;
	SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
	SQLSMALLINT textLength;
	std::string errorMsg = "No specific error message available."; // デフォルトメッセージ

	// ODBC診断レコードを取得する。通常、最初のレコード(RecNumber=1)に最も重要な情報が含まれる。
	if (SQLGetDiagRec(handleType, handle, 1, sqlState, &nativeError, message, sizeof(message), &textLength) == SQL_SUCCESS) {
		// 取得した情報を人間が読みやすい形式の文字列にまとめる
		errorMsg =	"SQLSTATE: " + std::string(reinterpret_cast<char*>(sqlState)) +
					", Native Error: " + std::to_string(nativeError) +
					", Message: " + std::string(reinterpret_cast<char*>(message));
	}
	return errorMsg;
}

OdbcException::OdbcException(const std::string& msg, SQLHANDLE handle, SQLSMALLINT handleType)
	: std::runtime_error(msg + " - " + getOdbcError(handle, handleType)) {}


//============================================
// 接続クラスの実装
//============================================
OdbcConnection::OdbcConnection(const std::string& dsn, const std::string& user, const std::string& pass) {
	// 1. 環境ハンドルの割り当て
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	// 2. ODBCバージョンの設定 (ODBC 3.x を使用)
	SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
	// 3. 接続ハンドルの割り当て
	SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

	// 4. データベースへの接続
	SQLRETURN ret = SQLConnect(hdbc,
							(SQLCHAR*)dsn.c_str(), SQL_NTS,	// DSN
							(SQLCHAR*)user.c_str(), SQL_NTS,	// User
							(SQLCHAR*)pass.c_str(), SQL_NTS);	// Password

	// 接続に失敗した場合は、リソースを解放して例外をスロー
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to connect to database", hdbc, SQL_HANDLE_DBC);
	}
}

OdbcConnection::~OdbcConnection() {
	// 接続が確立されている場合のみ切断・解放処理を行う
	if (hdbc != SQL_NULL_HDBC) {
		SQLDisconnect(hdbc); // 1. 切断
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc); // 2. 接続ハンドルの解放
	}
	if (henv != SQL_NULL_HENV) {
		SQLFreeHandle(SQL_HANDLE_ENV, henv); // 3. 環境ハンドルの解放
	}
}

SQLHDBC OdbcConnection::getHandle() const {
	return hdbc;
}

//============================================
// ステートメントクラスの実装
//============================================
OdbcStatement::OdbcStatement(const OdbcConnection& conn) {
	// 接続ハンドルに関連付けられたステートメントハンドルを割り当てる
	SQLAllocHandle(SQL_HANDLE_STMT, conn.getHandle(), &hstmt);
}

OdbcStatement::~OdbcStatement() {
	// ハンドルが有効な場合のみ解放する
	if (hstmt != SQL_NULL_HSTMT) {
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	}
}

void OdbcStatement::execute(const std::string& sql) {
	// SQLステートメントを直接実行する
	SQLRETURN ret = SQLExecDirect(hstmt, (SQLCHAR*)sql.c_str(), SQL_NTS);

	// 実行に失敗した場合 (SQL_SUCCESS_WITH_INFO は警告付き成功なので許容)
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		throw OdbcException("Failed to execute query", hstmt, SQL_HANDLE_STMT);
	}
}

bool OdbcStatement::fetch() {
	// 結果セットから次の行を取得する
	SQLRETURN ret = SQLFetch(hstmt);

	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
		return true; // 次の行が存在した
	}
	if (ret == SQL_NO_DATA) {
		return false; // 全ての行を取得し終わった
	}

	// 上記以外はエラーとみなす
	throw OdbcException("Failed to fetch row", hstmt, SQL_HANDLE_STMT);
}