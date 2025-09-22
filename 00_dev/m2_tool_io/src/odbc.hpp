/**
 * @file odbc.hpp
 * @brief ODBC接続をC++のクラスでラップし、RAIIパターンで安全にリソース管理を行うためのヘッダーファイル。
 * @details OdbcConnection, OdbcStatement, OdbcSessionの3クラスを提供します。
 */
#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <list>
#include <any>
#include <chrono>
#include <ctime>
#include <memory>   // std::unique_ptr, std::make_unique
#include <utility>  // std::move
#include <sqlext.h> // ODBCのコアヘッダー

//============================================
// 例外クラス
//============================================

/**
 * @class OdbcException
 * @brief ODBC API呼び出し中にエラーが発生した際にスローされる例外クラス。
 */
class OdbcException : public std::runtime_error {
public:
	OdbcException(const std::string& msg, SQLHANDLE handle, SQLSMALLINT handleType);
private:
	static std::string getOdbcError(SQLHANDLE handle, SQLSMALLINT handleType);
};

//============================================
// 接続クラス
//============================================

/**
 * @class OdbcConnection
 * @brief データベースへの接続を管理する低レベルクラス。
 * @details RAIIパターンを利用し、オブジェクト生成時に接続、破棄時に自動切断します。
 * 通常はOdbcSession経由で使用します。
 */
class OdbcConnection {
public:
	OdbcConnection(const std::string& dsn, const std::string& user, const std::string& pass);
	~OdbcConnection();

	OdbcConnection(const OdbcConnection&) = delete;
	OdbcConnection& operator=(const OdbcConnection&) = delete;

	SQLHDBC getHandle() const;
private:
	SQLHENV henv = SQL_NULL_HENV;
	SQLHDBC hdbc = SQL_NULL_HDBC;
};

//============================================
// ステートメントクラス
//============================================

/**
 * @class OdbcStatement
 * @brief SQLステートメントの実行と結果の取得を管理する低レベルクラス。
 * @details RAIIパターンを利用し、オブジェクト破棄時にステートメントハンドルを自動解放します。
 * 通常はOdbcSession経由で使用します。
 */
class OdbcStatement {
public:
	explicit OdbcStatement(const OdbcConnection& conn);
	~OdbcStatement();

	OdbcStatement(const OdbcStatement&) = delete;
	OdbcStatement& operator=(const OdbcStatement&) = delete;

	int execute(const std::string& sql);
	bool fetch();
	template<typename T> std::optional<T> getData(int col);
	void closeCursor();
	std::vector<std::map<std::string, std::any>> fetchAll();
	void prepare(const std::string& sql);
	template<typename T> void bind(int paramIndex, const T& value) { bound_params[paramIndex] = value; }
	void bind(int paramIndex, const std::chrono::system_clock::time_point& value);
	int executePrepared();
	int getAffectedRowCount();

private:
	SQLHSTMT hstmt = SQL_NULL_HSTMT;
	std::map<int, std::any> bound_params;
	std::map<int, SQLLEN> param_indicators;
	std::list<SQL_TIMESTAMP_STRUCT> timestamp_buffers;
};

//============================================
// セッション管理クラス
//============================================

/**
 * @class OdbcSession
 * @brief データベース接続と複数のステートメントを安全に一元管理する高レベルクラス。
 * @details このクラスのunique_ptrを一つ保持して利用するのが基本的な使い方です。
 * RAIIにより、このクラスのオブジェクトが破棄される際に、全てのステートメントが破棄された後で
 * 接続が安全に破棄されることを保証します。
 */
struct OdbcSession {
	/**
	 * @brief このセッションが使用する唯一のデータベース接続。
	 */
	std::unique_ptr<OdbcConnection> connection;

	/**
	 * @brief 名前付きで複数のステートメントを保持するコンテナ。
	 */
	std::map<std::string, std::unique_ptr<OdbcStatement>> statements;

	/**
	 * @brief コンストラクタ。与えられた接続の所有権を移動します。
	 * @param conn OdbcConnectionへのunique_ptr。
	 */
	OdbcSession(std::unique_ptr<OdbcConnection> conn)
		: connection(std::move(conn)) {}

	/**
	 * @brief このセッションに新しい名前付きステートメントを追加します。
	 * @param name このステートメントを識別するためのユニークな名前。
	 */
	void add_statement(const std::string& name) {
		if (connection) {
			statements[name] = std::make_unique<OdbcStatement>(*connection);
		}
	}
};