/**
 * @file odbc.hpp
 * @brief ODBC接続をC++のクラスでラップし、RAIIパターンで安全にリソース管理を行うためのヘッダーファイル。
 * @details OdbcException, OdbcConnection, OdbcStatement, OdbcSessionのクラス群を提供します。
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
#include <memory>	// std::unique_ptr, std::make_unique
#include <utility>	// std::move
#include <sqlext.h>	// ODBCのコアヘッダー

namespace odbc{

//============================================
// 例外クラス
//============================================

/**
 * @class OdbcException
 * @brief ODBC API呼び出し中にエラーが発生した際にスローされる例外クラス。
 */
class OdbcException : public std::runtime_error {
public:
	/**
	 * @brief OdbcExceptionを構築します。
	 * @param msg エラーの状況を示すメッセージ。
	 * @param handle エラーが発生したODBCハンドル (HENV, HDBC, HSTMT)。
	 * @param handleType エラーが発生したハンドルの種別。
	 */
	OdbcException(const std::string& msg, SQLHANDLE handle, SQLSMALLINT handleType);
private:
	/**
	 * @brief ODBCドライバから詳細なエラー情報を取得する静的ヘルパー関数。
	 * @param handle エラーが発生したODBCハンドル。
	 * @param handleType ハンドルの種別。
	 * @return SQLSTATE, Native Error, Messageを含む整形されたエラー文字列。
	 */
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
	/**
	 * @brief データベースへの接続を確立するコンストラクタ。
	 * @param dsn データソース名。
	 * @param user ユーザー名。
	 * @param pass パスワード。
	 * @throw OdbcException 接続に失敗した場合。
	 */
	OdbcConnection(const std::string& dsn, const std::string& user, const std::string& pass);

	/**
	 * @brief データベースから切断し、リソースを解放するデストラクタ。
	 */
	~OdbcConnection();

	OdbcConnection(const OdbcConnection&) = delete;
	OdbcConnection& operator=(const OdbcConnection&) = delete;

	/**
	 * @brief ODBC接続ハンドル(HDBC)を取得します。
	 * @return OdbcStatementの生成などに必要な接続ハンドル。
	 */
	SQLHDBC getHandle() const;

	/**
	 * @brief トランザクションを開始します。
	 * @details 内部でオートコミットを無効にします。
	 * @throw OdbcException トランザクションの開始に失敗した場合。
	 */
	void beginTransaction();

	/**
	 * @brief 現在のトランザクションをコミットします。
	 * @throw OdbcException コミットに失敗した場合。
	 */
	void commit();

	/**
	 * @brief 現在のトランザクションをロールバックします。
	 * @throw OdbcException ロールバックに失敗した場合。
	 */
	void rollback();

private:
	/// @brief ODBC環境ハンドル。
	SQLHENV henv = SQL_NULL_HENV;
	/// @brief ODBC接続ハンドル。
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
	/**
	 * @brief OdbcConnectionからステートメントハンドルを生成するコンストラクタ。
	 * @param conn 親となるOdbcConnectionオブジェクト。
	 */
	explicit OdbcStatement(const OdbcConnection& conn);

	/**
	 * @brief ステートメントハンドルを解放するデストラクタ。
	 */
	~OdbcStatement();

	OdbcStatement(const OdbcStatement&) = delete;
	OdbcStatement& operator=(const OdbcStatement&) = delete;

	/**
	 * @brief SQL文を直接実行します。
	 * @details 結果セットを返さないINSERT/UPDATE/DELETE文や、パラメータを含まないSELECT文に使用します。
	 * @param sql 実行するSQL文字列。
	 * @return 影響を受けた行数。
	 * @throw OdbcException クエリの実行に失敗した場合。
	 */
	int execute(const std::string& sql);

	/**
	 * @brief SELECT文の結果セットから次の行にカーソルを移動させます。
	 * @return 次の行が存在すればtrue、存在しなければfalse。
	 * @throw OdbcException フェッチ処理中にエラーが発生した場合。
	 */
	bool fetch();

	/**
	 * @brief 現在の行から指定列のデータを取得します。(基本テンプレート宣言)
	 * @details 各データ型に対応する特殊化は odbc.cpp に実装されています。
	 * @tparam T 取得したいC++のデータ型 (int, std::string, etc.)。
	 * @param col 取得する列番号 (1から始まる)。
	 * @return データを格納したoptional。データがNULLまたは取得失敗の場合は空。
	 */
	template<typename T> std::optional<T> getData(int col);

	/**
	 * @brief 現在開いているカーソルを閉じ、結果セットを破棄します。
	 * @details SELECT文の結果を最後まで読まずに次のクエリを実行する場合などに呼び出します。
	 * カーソルが既に閉じていてもエラーになりません（べき等性）。
	 * @throw OdbcException カーソルを閉じる際に予期せぬエラーが発生した場合。
	 */
	void closeCursor();

	/**
	 * @brief SELECT文の結果セット全行を、辞書型の配列として一括取得します。
	 * @details 内部で最後までfetchし、自動でカーソルを閉じるため、呼び出し後にcloseCursorは不要です。
	 * @return 全ての行データ。
	 * @throw OdbcException データ取得中にエラーが発生した場合。
	 */
	std::vector<std::map<std::string, std::any>> fetchAll();

	/**
	 * @brief プリペアドステートメント用のSQL文を準備（コンパイル）します。
	 * @param sql プレースホルダ(?)を含むSQL文。
	 * @throw OdbcException SQL文の準備に失敗した場合。
	 */
	void prepare(const std::string& sql);

	/**
	 * @brief パラメータをバインドします。
	 * @tparam T バインドするパラメータの型。
	 * @param paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param value バインドするパラメータの値。
	 */
	template<typename T>
	void bind(int paramIndex, const T& value) { bound_params[paramIndex] = value; }

	/**
	 * @brief パラメータをバインドします。
	 * @param paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param value バインドするパラメータの値。
	 */
	void bind(int paramIndex, const std::string& value);

	/**
	 * @brief パラメータをバインドします。
	 * @param paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param value バインドするパラメータの値。
	 */
	void bind(int paramIndex, const std::vector<char>& value);

	/**
	 * @brief パラメータをバインドします (日時型専用のオーバーロード)。
	 * @details time_pointをODBC用のTIMESTAMP構造体に変換する特別な処理を行ってからバインドします。
	 * @param paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param value バインドするパラメータの値 (time_point)。
	 */
	void bind(int paramIndex, const std::chrono::system_clock::time_point& value);

	/**
	 * @brief 一括挿入(バルクインサート)用に、パラメータの配列を列単位でバインドします。
	 * @details int, double, long long などの固定長で連続メモリに配置される型を扱うための汎用テンプレートです。
	 * @tparam T バインドするデータの型 (例: int, std::string)
	 * @param paramIndex パラメータのインデックス (1から始まる)。
	 * @param values バインドするデータのvector。
	 */
	template<typename T>
	void bindColumn(int paramIndex, const std::vector<T>& values) { bulk_param_buffers[paramIndex] = values; }

	/**
	 * @brief 一括挿入用に文字列の配列をバインドします (string専用オーバーロード)。
	 * @details 可変長の文字列をODBCが要求する固定長の連続メモリにコピーする特別な処理を行います。
	 * @param paramIndex パラメータのインデックス (1から始まる)。
	 * @param values バインドするデータのvector。
	 */
	void bindColumn(int paramIndex, const std::vector<std::string>& values);

	/**
	 * @brief 一括挿入用にBLOBの配列をバインドします (vector<char>専用オーバーロード)。
	 * @details 可変長のバイナリデータをODBCが要求する固定長の連続メモリにコピーする特別な処理を行います。
	 * @param paramIndex パラメータのインデックス (1から始まる)。
	 * @param values バインドするデータのvector。
	 */
	void bindColumn(int paramIndex, const std::vector<std::vector<char>>& values);

	/**
	 * @brief 一括挿入用に日時の配列をバインドします (time_point専用オーバーロード)。
	 * @details time_pointをTIMESTAMP構造体に変換する特別な処理を行います。
	 * @param paramIndex パラメータのインデックス (1から始まる)。
	 * @param values バインドするデータのvector。
	 */
	void bindColumn(int paramIndex, const std::vector<std::chrono::system_clock::time_point>& values);

	/**
	 * @brief 一括挿入(バルクインサート)で一度に処理する行数を設定します。
	 * @param size 挿入する行数。
	 */
	void setParamsetSize(size_t size);

	/**
	 * @brief 準備・バインドされたステートメントを実行します。
	 * @details set_paramset_sizeが呼び出されている場合は一括実行、そうでなければ単一行実行を行います。
	 * @return 影響を受けた行数。
	 * @throw OdbcException ステートメントの実行に失敗した場合。
	 */
	int executePrepared();

	/**
	 * @brief 直前のINSERT/UPDATE/DELETEで影響を受けた行数を取得します。
	 * @return 影響を受けた行数。
	 */
	int getAffectedRowCount();

private:
	/// @brief ODBCステートメントハンドル。
	SQLHSTMT hstmt = SQL_NULL_HSTMT;
	/// @brief (単一行用) バインドされたパラメータの値を、実行時まで保持するためのコンテナ。
	std::map<int, std::any> bound_params;
	/// @brief (単一行用) バインドされた文字列やBLOBの長さを保持するためのコンテナ。
	std::map<int, SQLLEN> param_indicators;

	/// @brief (一括実行用) バインドされたパラメータ配列のデータを保持するバッファ。
	std::map<int, std::any> bulk_param_buffers;
	/// @brief (一括実行用) バインドされた可変長データ(文字列など)の長さの配列を保持するバッファ。
	std::map<int, std::vector<SQLLEN>> bulk_param_indicators;
	/// @brief (一括実行用) BLOBのストライド（1行あたりの固定長）を保持するバッファ。
	std::map<int, size_t> bulk_param_strides;
	/// @brief (一括実行用) 各パラメータのSQL型を保持するマップ。
	std::map<int, SQLSMALLINT> bulk_param_sql_types;
	/// @brief 一括実行で処理する行数。
	size_t paramset_size = 0;
};

//============================================
// セッション管理クラス
//============================================

/**
 * @struct OdbcSession
 * @brief データベース接続と複数のステートメントを安全に一元管理する高レベルクラス。
 * @details このクラスのunique_ptrを一つ保持して利用するのが基本的な使い方です。
 * RAIIにより、このクラスのオブジェクトが破棄される際に、全てのステートメントが破棄された後で
 * 接続が安全に破棄されることを保証します。
 */
struct OdbcSession {
	/// @brief このセッションが使用する唯一のデータベース接続。
	std::unique_ptr<OdbcConnection> connection;
	/// @brief 名前付きで複数のステートメントを保持するコンテナ。
	std::map<std::string, std::unique_ptr<OdbcStatement>> statements;

	/**
	 * @brief コンストラクタ。与えられた接続の所有権を移動します。
	 * @param conn OdbcConnectionへのunique_ptr。
	 */
	OdbcSession(std::unique_ptr<OdbcConnection> conn)
		: connection(std::move(conn)) {}

	/**
	 * @brief このセッションに新しい名前付きステートメントを追加または取得します。
	 * @param name このステートメントを識別するためのユニークな名前。
	 * @param overwrite (オプション) 同名のステートメントが既に存在する場合に上書きするかどうか。デフォルトはfalse（上書きしない）。
	 * @return 作成された、または既存のステートメントへの参照。
	 */
	OdbcStatement& add_statement(const std::string& name, bool overwrite = false) {
		if (!connection) {
			throw std::runtime_error("Cannot add statement: OdbcConnection is null.");
		}

		// 1. 指定された名前のステートメントが既に存在するか探す
		auto it = statements.find(name);

		// 2. ステートメントが存在する場合
		if (it != statements.end()) {
			// 2a. 上書きが要求されていれば、古いものを破棄して新しいものに置き換える
			if (overwrite) {
				it->second = std::make_unique<OdbcStatement>(*connection);
			}
			// 2b. (上書きしたかどうかにかかわらず) 存在するステートメントへの参照を返す
			return *it->second;
		}
		// 3. ステートメントが存在しない場合
		else {
			// 3a. 新しく作成し、マップに追加する
			auto& new_stmt_ptr = statements[name] = std::make_unique<OdbcStatement>(*connection);
			// 3b. 新しく作成したステートメントへの参照を返す
			return *new_stmt_ptr;
		}
	}
};
} // namespace odbc