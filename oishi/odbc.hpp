/**
 * @file odbc.hpp
 * @brief ODBC接続をC++のクラスでラップし、RAIIパターンで安全にリソース管理を行うためのヘッダーファイル。
 */
#ifndef ODBC_HPP
#define ODBC_HPP

#include <stdexcept>
#include <string>
#include <vector>
#include <optional>
#include <sqlext.h> // ODBCのコアヘッダー

//============================================
// 例外クラス
//============================================

/**
 * @class OdbcException
 * @brief ODBC API呼び出し中にエラーが発生した際にスローされる例外クラス。
 *
 * SQLSTATEやネイティブエラーコードなどの詳細なエラー情報を保持します。
 */
class OdbcException : public std::runtime_error {
public:
	/**
	 * @brief OdbcExceptionのコンストラクタ。
	 * @param msg エラーメッセージ。
	 * @param handle エラーが発生したODBCハンドル。
	 * @param handleType エラーが発生したODBCハンドルの種別 (SQL_HANDLE_ENV, SQL_HANDLE_DBC, SQL_HANDLE_STMT)。
	 */
	OdbcException(const std::string& msg, SQLHANDLE handle, SQLSMALLINT handleType);

private:
	/**
	 * @brief ODBCドライバから詳細なエラー情報を取得するヘルパー関数。
	 * @param handle エラーが発生したODBCハンドル。
	 * @param handleType ハンドルの種別。
	 * @return 整形されたエラー文字列。
	 */
	static std::string getOdbcError(SQLHANDLE handle, SQLSMALLINT handleType);
};


//============================================
// 接続クラス
//============================================

/**
 * @class OdbcConnection
 * @brief データベースへの接続を管理するクラス。
 *
 * RAII (Resource Acquisition Is Initialization) パターンを利用しており、
 * オブジェクトの生成時に接続を確立し、オブジェクトの破棄時に自動的に切断します。
 */
class OdbcConnection {
public:
	/**
	 * @brief データベースに接続するためのコンストラクタ。
	 * @param dsn データソース名。
	 * @param user ユーザー名。
	 * @param pass パスワード。
	 * @throw OdbcException 接続に失敗した場合にスローされます。
	 */
	OdbcConnection(const std::string& dsn, const std::string& user, const std::string& pass);

	/**
	 * @brief データベースから切断するためのデストラクタ。
	 */
	~OdbcConnection();

	// オブジェクトのコピーを禁止することで、リソースの二重解放を防ぐ
	OdbcConnection(const OdbcConnection&) = delete;
	OdbcConnection& operator=(const OdbcConnection&) = delete;

	/**
	 * @brief 内部で保持している接続ハンドル(HDBC)を取得します。
	 * @return SQLHDBC 接続ハンドル。
	 */
	SQLHDBC getHandle() const;

private:
	SQLHENV henv = SQL_NULL_HENV; // ODBC環境ハンドル
	SQLHDBC hdbc = SQL_NULL_HDBC; // データベース接続ハンドル
};


//============================================
// ステートメントクラス
//============================================

/**
 * @class OdbcStatement
 * @brief SQLステートメントの実行と結果の取得を管理するクラス。
 *
 * このクラスもRAIIパターンを利用しており、オブジェクトの破棄時に
 * ステートメントハンドルを自動的に解放します。
 */
class OdbcStatement {
public:
	/**
	 * @brief OdbcStatementのコンストラクタ。
	 * @param conn OdbcConnectionオブジェクトへの参照。
	 */
	explicit OdbcStatement(const OdbcConnection& conn);

	/**
	 * @brief ステートメントハンドルを解放するためのデストラクタ。
	 */
	~OdbcStatement();

	// オブジェクトのコピーを禁止
	OdbcStatement(const OdbcStatement&) = delete;
	OdbcStatement& operator=(const OdbcStatement&) = delete;

	/**
	 * @brief SQLクエリを実行します。SELECT文以外(INSERT, UPDATE, DELETEなど)にも使用できます。
	 * @param sql 実行するSQL文字列。
	 * @throw OdbcException クエリの実行に失敗した場合にスローされます。
	 */
	void execute(const std::string& sql);

	/**
	 * @brief 結果セットから次の行を取得(フェッチ)します。
	 * @return bool 次の行が存在すればtrue、存在しなければfalseを返します。
	 * @throw OdbcException フェッチ中にエラーが発生した場合にスローされます。
	 */
	bool fetch();

	/**
	 * @brief 現在の行から指定された列のデータを取得します。
	 *
	 * @tparam T 取得したいデータの型 (int, std::stringなど)。
	 * @param col 取得する列の番号 (1から始まるインデックス)。
	 * @return std::optional<T> データが取得できた場合はその値を、
	 * データがNULLまたは取得に失敗した場合は空のoptionalを返します。
	 * @note テンプレート関数はコンパイル時に実体化される必要があるため、実装もヘッダーファイルに記述します。
	 */
	template<typename T>
	std::optional<T> getData(int col) {
		// T型に応じて、ODBCのCデータ型を決定する
		SQLSMALLINT cType = SQL_C_DEFAULT;
		if constexpr (std::is_same_v<T, int>) cType = SQL_C_LONG;
		if constexpr (std::is_same_v<T, std::string>) cType = SQL_C_CHAR;
		// 他の型(doubleならSQL_C_DOUBLEなど)も必要に応じて追加する

		// データを格納するための変数
		T value;
		SQLLEN indicator; // データの長さやNULLかどうかを示すインジケータ

		// 文字列型の場合は固定長のバッファに一度読み込む
		char buffer[1024];
		void* dataPtr = &value; // データ格納先のポインタ
		SQLINTEGER bufferLength = sizeof(value);

		// Tがstd::stringの場合のみ、ポインタとバッファ長を上書きする
		if constexpr (std::is_same_v<T, std::string>) {
			dataPtr = buffer;
			bufferLength = sizeof(buffer);
		}

		// 実際にデータを取得する
		SQLRETURN ret = SQLGetData(hstmt, col, cType, dataPtr, bufferLength, &indicator);

		// 取得失敗、またはデータがNULLの場合
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO || indicator == SQL_NULL_DATA) {
			return std::nullopt; // 空のoptionalを返す
		}

		// 型に応じて値を返す
		if constexpr (std::is_same_v<T, std::string>) {
			return std::string(buffer); // バッファからstringオブジェクトを作成して返す
		} else {
			return value; // 値をそのまま返す
		}
	}

private:
	SQLHSTMT hstmt = SQL_NULL_HSTMT; // ステートメントハンドル
};

#endif // ODBC_HPP