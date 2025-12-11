/**
 * @file odbc.hpp
 * @brief ODBC接続をC++のクラスでラップし、RAIIパターンで安全にリソース管理を行うためのヘッダーファイル。
 * @details OdbcException, OdbcConnection, OdbcStatement, OdbcSessionのクラス群を提供します。
 */
#pragma once

#include <any>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <stdexcept>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <sqlext.h>	// ODBCのコアヘッダー

namespace odbc{

// 符号が曖昧な整数型(char, wchar_t)を除外する
template<typename T>
inline constexpr bool is_unambiguous_integral_v =
	std::is_integral_v<T> &&
	!std::is_same_v<std::remove_cv_t<T>, char> &&
	!std::is_same_v<std::remove_cv_t<T>, wchar_t>;

// 浮動小数点型 または 符号が明確な整数型
template<typename T>
inline constexpr bool is_explicit_arithmetic_v =
	std::is_floating_point_v<T> || is_unambiguous_integral_v<T>;


//============================================
// ヘルパー関数
//============================================

/**
 * @brief std::chrono::system_clock::time_point を SQL_TIMESTAMP_STRUCT に変換します。
 * @param[in] tp 変換元の time_point オブジェクト。
 * @return 成功した場合、SQL_TIMESTAMP_STRUCT へ変換後の構造体。失敗したら std::nullopt。
 */
std::optional<SQL_TIMESTAMP_STRUCT> to_sql_timestamp(const std::chrono::system_clock::time_point& tp);

/**
 * @brief SQL_TIMESTAMP_STRUCT を std::chrono::system_clock::time_point に変換します。
 * @param[in] tp 変換元の SQL_TIMESTAMP_STRUCT オブジェクト。
 * @return 成功した場合、std::chrono::system_clock::time_point へ変換後の構造体。失敗したら std::nullopt。
 */
std::optional<std::chrono::system_clock::time_point> to_time_point(const SQL_TIMESTAMP_STRUCT& ts);

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
	 * @param[in] msg エラーの状況を示すメッセージ。
	 * @param[in] handle エラーが発生したODBCハンドル (HENV, HDBC, HSTMT)。
	 * @param[in] handleType エラーが発生したハンドルの種別。
	 */
	OdbcException(const std::string& msg, SQLHANDLE handle, SQLSMALLINT handleType);
private:
	/**
	 * @brief ODBCドライバから詳細なエラー情報を取得する静的ヘルパー関数。
	 * @param[in] handle エラーが発生したODBCハンドル。
	 * @param[in] handleType ハンドルの種別。
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
	 * @param[in] dsn データソース名。
	 * @param[in] user ユーザー名。
	 * @param[in] pass パスワード。
	 * @throw OdbcException 接続に失敗した場合。
	 */
	explicit OdbcConnection(const std::string& dsn, const std::string& user, const std::string& pass);

	// コピーは禁止
	OdbcConnection(const OdbcConnection&) = delete;
	OdbcConnection& operator=(const OdbcConnection&) = delete;

	// ムーブは許可
	OdbcConnection(OdbcConnection&&) = default;
	OdbcConnection& operator=(OdbcConnection&&) = default;

	/**
	 * @brief ODBC接続ハンドル(HDBC)を取得します。
	 * @details インライン関数にしたいのでヘッダーファイル内で定義しています。
	 * @return OdbcStatementの生成などに必要な接続ハンドル。
	 */
	SQLHDBC getConnHandle() const {
		return static_cast<SQLHDBC>(hdbc.get());
	}

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
	// 環境ハンドル用デリータ
	struct EnvDeleter {
		void operator()(SQLHENV h) const {
			if (h != SQL_NULL_HENV) { SQLFreeHandle(SQL_HANDLE_ENV, h); }
		}
	};

	// 接続ハンドル用デリータ
	// ※重要: 接続ハンドルは「切断(Disconnect)」してから「解放(Free)」する必要がある
	struct DbcDeleter {
		void operator()(SQLHDBC h) const {
			if (h != SQL_NULL_HDBC) {
				SQLDisconnect(h); // 失敗してもデストラクタ内なので無視するしかない
				SQLFreeHandle(SQL_HANDLE_DBC, h);
			}
		}
	};

// ■ スマートポインタでの宣言 ■
	// ODBCハンドルは通常 void* なので unique_ptr<void, Deleter> を使う
	// 注意: 宣言順序が重要です (henv が先に宣言されていれば最後に破棄される…と言いたいところですが、
	//      DbcDeleter内で完結しているので、ここでは henv が hdbc より長生きしていれば安全です)
	//      したがって、メンバ変数は 1. henv, 2. hdbc の順で宣言します。

	/// @brief ODBC環境ハンドル。
	std::unique_ptr<void, EnvDeleter> henv;
	/// @brief ODBC接続ハンドル。
	std::unique_ptr<void, DbcDeleter> hdbc;

	/**
	 * @brief ODBC環境ハンドル(HENV)を取得します。
	 * @details インライン関数にしたいのでヘッダーファイル内で定義しています。
	 * @return ODBC環境ハンドル。
	 */
	SQLHENV getEnvHandle() const {
		return static_cast<SQLHENV>(henv.get());
	}
};

//============================================
// ステートメントクラス
//============================================

// 型 T から C型 と SQL型 を決定するための型特性
template<typename T>
struct OdbcTypeTraits; // まず基本テンプレートを宣言

// --- 以下、サポートする型ごとに「特殊化」を定義 ---

template<>
struct OdbcTypeTraits<signed char> {
	static constexpr SQLSMALLINT c_type		= SQL_C_STINYINT;
	static constexpr SQLSMALLINT sql_type	= SQL_TINYINT;
};

template<>
struct OdbcTypeTraits<unsigned char> {
	static constexpr SQLSMALLINT c_type		= SQL_C_UTINYINT;
	static constexpr SQLSMALLINT sql_type	= SQL_TINYINT;
};

template<>
struct OdbcTypeTraits<short> {
	static constexpr SQLSMALLINT c_type		= SQL_C_SSHORT;
	static constexpr SQLSMALLINT sql_type	= SQL_SMALLINT;
};

template<>
struct OdbcTypeTraits<unsigned short> {
	static constexpr SQLSMALLINT c_type		= SQL_C_USHORT;
	static constexpr SQLSMALLINT sql_type	= SQL_SMALLINT;
};

template<>
struct OdbcTypeTraits<int> {
	static constexpr SQLSMALLINT c_type		= SQL_C_SLONG;
	static constexpr SQLSMALLINT sql_type	= SQL_INTEGER;
};

template<>
struct OdbcTypeTraits<unsigned int> {
	static constexpr SQLSMALLINT c_type		= SQL_C_ULONG;
	static constexpr SQLSMALLINT sql_type	= SQL_INTEGER;
};

template<>
struct OdbcTypeTraits<long> {
	static constexpr SQLSMALLINT c_type		= ((sizeof(long) == 8) ? (SQL_C_SBIGINT)	: (SQL_C_SLONG));
	static constexpr SQLSMALLINT sql_type	= ((sizeof(long) == 8) ? (SQL_BIGINT)		: (SQL_INTEGER));
};

template<>
struct OdbcTypeTraits<unsigned long> {
	static constexpr SQLSMALLINT c_type		= ((sizeof(unsigned long) == 8) ? (SQL_C_UBIGINT)	: (SQL_C_ULONG));
	static constexpr SQLSMALLINT sql_type	= ((sizeof(unsigned long) == 8) ? (SQL_BIGINT)		: (SQL_INTEGER));
};

template<>
struct OdbcTypeTraits<long long> {
	static constexpr SQLSMALLINT c_type		= SQL_C_SBIGINT;
	static constexpr SQLSMALLINT sql_type	= SQL_BIGINT;
};

template<>
struct OdbcTypeTraits<unsigned long long> {
	static constexpr SQLSMALLINT c_type		= SQL_C_UBIGINT;
	static constexpr SQLSMALLINT sql_type	= SQL_BIGINT;
};

template<>
struct OdbcTypeTraits<float> {
	static constexpr SQLSMALLINT c_type = SQL_C_FLOAT;
	static constexpr SQLSMALLINT sql_type = SQL_REAL; // (bind用)
};

template<>
struct OdbcTypeTraits<double> {
	static constexpr SQLSMALLINT c_type		= SQL_C_DOUBLE;
	static constexpr SQLSMALLINT sql_type	= SQL_DOUBLE;
};

template<>
struct OdbcTypeTraits<SQL_TIMESTAMP_STRUCT> {
	static constexpr SQLSMALLINT c_type		= SQL_C_TYPE_TIMESTAMP;
	static constexpr SQLSMALLINT sql_type	= SQL_TIMESTAMP;
};

// これ以外の型が来た場合 (例: std::string) にコンパイルエラーにするための仕組み
template<typename T>
struct OdbcTypeTraits {
	// SFINAE や static_assert を使う
	static_assert(!std::is_same_v<T, T>, "OdbcTypeTraits for this type is not defined.");
};

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
	 * @param[in] conn 親となるOdbcConnectionオブジェクト。
	 */
	explicit OdbcStatement(const OdbcConnection& conn);

	// コピーは禁止
	OdbcStatement(const OdbcStatement&) = delete;
	OdbcStatement& operator=(const OdbcStatement&) = delete;

	// ムーブは許可
	OdbcStatement(OdbcStatement&&) = default;
	OdbcStatement& operator=(OdbcStatement&&) = default;

	/**
	 * @brief SQL文を直接実行します。
	 * @details 結果セットを返さないINSERT/UPDATE/DELETE文や、パラメータを含まないSELECT文に使用します。
	 * @param[in] sql 実行するSQL文字列。
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
	 * @brief 現在の行から指定列のデータを取得します (汎用テンプレート定義)。
	 * @details 数値型やTIMESTAMPのような固定長データ型のための実装です。
	 * std::string や std::vector<char> のような可変長型は、テンプレート特殊化によって処理されます。
	 * @tparam T 取得したいC++のデータ型 (int, double, etc.)
	 * @param[in] col 取得する列番号 (1から始まる)。
	 * @return データを格納したoptional。データがNULLまたは取得失敗の場合は空。
	 */
	template<typename T>
	std::optional<T> getData(int col) {
		T value;
		SQLLEN indicator;
		// OdbcTypeTraits から型情報を取得
		constexpr SQLSMALLINT c_type = OdbcTypeTraits<T>::c_type;
		SQLRETURN ret = SQLGetData(getStmtHandle(), col, c_type, &value, sizeof(value), &indicator);

		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			// (エラー処理: 例外をスローする)
			OdbcException("Failed to get data", getStmtHandle(), SQL_HANDLE_STMT);
		}
		if (indicator == SQL_NULL_DATA) {
			return std::nullopt;
		}
		return value;
	}

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
	 * @param[in] sql プレースホルダ(?)を含むSQL文。
	 * @throw OdbcException SQL文の準備に失敗した場合。
	 */
	void prepare(const std::string& sql);

	/**
	 * @brief パラメータをバインドします (NULL対応版)。
	 * @tparam T バインドするパラメータの型。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] opt_value バインドするパラメータの値。NULLをバインドしたければ nullopt。
	 */
	template<typename T,
			 // SFINAE: Tが数値型(int, double等)またはTIMESTAMP構造体の場合のみ、このテンプレートを有効にする
			std::enable_if_t<
				is_explicit_arithmetic_v<T> ||
				std::is_same_v<std::remove_cv_t<T>, SQL_TIMESTAMP_STRUCT>,
			int> = 0
	>
	void bind(int paramIndex, const std::optional<T>& opt_val) {
		param_indicators.insert_or_assign(paramIndex, ((opt_val.has_value())
			? (sizeof(T))
			: (SQL_NULL_DATA)
		));
		bound_params.insert_or_assign(paramIndex, opt_val);
	}

	/**
	 * @brief パラメータをバインドします (NULL対応版)。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] opt_val バインドするパラメータの値。NULLをバインドしたければ nullopt。
	 */
	void bind(int paramIndex, const std::optional<std::string>& opt_val);

	/**
	 * @brief パラメータをバインドします。(NULL対応版)
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] opt_val バインドするパラメータの値。NULLをバインドしたければ nullopt。
	 */
	void bind(int paramIndex, const std::optional<std::vector<char>>& opt_val);

	/**
	 * @brief パラメータをバインドします (NULL対応版、time_point 専用オーバーロード)。
	 * @details time_point を ODBC用の TIMESTAMP 構造体に変換する処理を行ってからバインドします。
	 * 変換処理に失敗した場合はバインドしません。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] opt_val バインドするパラメータの値 (time_point)。NULLをバインドしたければ nullopt。
	 * @return TIMESTAMP 構造体への変換に成功した場合はtrue、失敗した場合はfalse。
	 */
	bool bind(int paramIndex, const std::optional<std::chrono::system_clock::time_point>& opt_val);

	/**
	 * @brief パラメータをバインドします (NULL対応版、long double 専用オーバーロード)。
	 * @details long double を double に丸めてからバインドします。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] opt_val バインドするパラメータの値。NULLをバインドしたければ nullopt。
	 */
	[[deprecated("Warning: 'long double' is truncated to 'double' for ODBC binding.")]]
	void bind(int paramIndex, const std::optional<long double>& opt_val);

	/**
	 * @brief パラメータをバインドします (NULL非対応版)。
	 * @details value を std::optional で包んで、適切なNULL対応版のオーバーロードへ処理を委譲します。
	 * @tparam T バインドするパラメータの型。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] value バインドするパラメータの値。
	 */
	template<typename T>
	void bind(int paramIndex, const T& value) {
		bind(paramIndex, std::make_optional(value));
	}

	/**
	 * @brief パラメータをバインドします (NULL非対応版、time_point 専用オーバーロード)。
	 * @details time_point を ODBC用の TIMESTAMP 構造体に変換する処理を行ってからバインドします。
	 * 変換処理に失敗した場合はバインドしません。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] value バインドするパラメータの値。
	 * @return TIMESTAMP 構造体への変換に成功した場合はtrue、失敗した場合はfalse。
	 */
	bool bind(int paramIndex, const std::chrono::system_clock::time_point& value) {
		return bind(paramIndex, std::make_optional(value));
	}

	/**
	 * @brief 一括挿入(バルクインサート)用に、パラメータの配列を列単位でバインドします (NULL非対応版)。
	 * @details int, double, long long などの固定長で連続メモリに配置される型を扱うための汎用テンプレートです。
	 * @tparam T バインドするデータの型 (例: int, std::string)
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] values バインドするデータのvector。
	 */
	template<typename T,
			 // SFINAE: Tが数値型(int, double等)またはTIMESTAMP構造体の場合のみ、このテンプレートを有効にする
			std::enable_if_t<
				is_explicit_arithmetic_v<T> ||
				std::is_same_v<std::remove_cv_t<T>, SQL_TIMESTAMP_STRUCT>,
			int> = 0
	>
	void bindColumn(int paramIndex, const std::vector<T>& values) {
		bulk_param_indicators.insert_or_assign(paramIndex, std::vector<SQLLEN>(values.size(), sizeof(T)));
		bulk_param_buffers.insert_or_assign(paramIndex, values);
	}

	/**
	 * @brief 一括挿入(バルクインサート)用に、パラメータの配列を列単位でバインドします (NULL対応版)。
	 * @details int, double, long long などの固定長で連続メモリに配置される型を扱うための汎用テンプレートです。
	 * @tparam T バインドするデータの型 (例: int, std::string)
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 */
	template<typename T,
			 // SFINAE: Tが数値型(int, double等)またはTIMESTAMP構造体の場合のみ、このテンプレートを有効にする
			std::enable_if_t<
				std::is_arithmetic_v<T> ||
				std::is_same_v<T, SQL_TIMESTAMP_STRUCT>,
			int> = 0
	>
	void bindColumn(int paramIndex, const std::vector<std::optional<T>>& opt_vals) {
		// 1. データ配列とインジケータ配列を作成
		std::vector<T> data_buffer(opt_vals.size());
		bulk_param_indicators[paramIndex].resize(opt_vals.size());
		for (size_t i = 0; i < opt_vals.size(); i++) {
			if (opt_vals[i].has_value()) {
				data_buffer[i] = opt_vals[i].value();
				// 固定長データの場合、インジケータは0で良い
				bulk_param_indicators[paramIndex][i] = sizeof(T);
			} else {
				// data_buffer[i] はデフォルト値(0など)のまま
				bulk_param_indicators[paramIndex][i] = SQL_NULL_DATA;
			}
		}
		// 2. 作成したデータ配列をバッファに保存
		bulk_param_buffers.insert_or_assign(paramIndex, data_buffer);
	}

	/**
	 * @brief 一括挿入用に文字列の配列をバインドします (NULL非対応版、string 専用オーバーロード)。
	 * @details 可変長の文字列をODBCが要求する固定長の連続メモリにコピーする処理を行います。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] values バインドするデータのvector。
	 */
	void bindColumn(int paramIndex, const std::vector<std::string>& values);

	/**
	 * @brief 一括挿入用に文字列の配列をバインドします (NULL対応版、string 専用オーバーロード)。
	 * @details 可変長の文字列をODBCが要求する固定長の連続メモリにコピーする処理を行います。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 */
	void bindColumn(int paramIndex, const std::vector<std::optional<std::string>>& opt_vals);

	/**
	 * @brief 一括挿入用にBLOBの配列をバインドします (NULL非対応版、vector<char> 専用オーバーロード)。
	 * @details 可変長のバイナリデータをODBCが要求する固定長の連続メモリにコピーする処理を行います。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] values バインドするデータのvector。
	 */
	void bindColumn(int paramIndex, const std::vector<std::vector<char>>& values);

	/**
	 * @brief 一括挿入用にBLOBの配列をバインドします (NULL対応版、vector<char> 専用オーバーロード)。
	 * @details 可変長のバイナリデータをODBCが要求する固定長の連続メモリにコピーする処理を行います。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 */
	void bindColumn(int paramIndex, const std::vector<std::optional<std::vector<char>>>& opt_vals);

	/**
	 * @brief 一括挿入用に日時の配列をバインドします (NULL非対応版、time_point 専用オーバーロード)。
	 * @details time_point を TIMESTAMP 構造体に変換する処理を行ってからバインドします。
	 * 変換処理に失敗した場合はバインドしません。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] values バインドするデータのvector。
	 * @return TIMESTAMP 構造体への変換に全て成功した場合はtrue、一つでも失敗した場合はfalse。
	 */
	bool bindColumn(int paramIndex, const std::vector<std::chrono::system_clock::time_point>& values);

	/**
	 * @brief 一括挿入用に日時の配列をバインドします (NULL対応版、time_point 専用オーバーロード)。
	 * @details time_point を TIMESTAMP 構造体に変換する処理を行ってからバインドします。
	 * 変換処理に失敗した場合はバインドしません。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 * @return TIMESTAMP 構造体への変換に全て成功した場合はtrue、一つでも失敗した場合はfalse。
	 */
	bool bindColumn(int paramIndex, const std::vector<std::optional<std::chrono::system_clock::time_point>>& values);

	/**
	 * @brief 一括挿入用に long double の配列をバインドします (NULL非対応版、long double 専用オーバーロード)。
	 * @details long double を double に丸めてからバインドします。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 */
	[[deprecated("Warning: 'long double' is truncated to 'double' for ODBC binding.")]]
	void bindColumn(int paramIndex, const std::vector<long double>& opt_vals);

	/**
	 * @brief 一括挿入用に long double の配列をバインドします (NULL対応版、long double 専用オーバーロード)。
	 * @details long double を double に丸めてからバインドします。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 */
	[[deprecated("Warning: 'long double' is truncated to 'double' for ODBC binding.")]]
	void bindColumn(int paramIndex, const std::vector<std::optional<long double>>& opt_vals);

	/**
	 * @brief 一括挿入(バルクインサート)で一度に処理する行数を設定します。
	 * @param[in] size 挿入する行数。
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
	// ■ カスタムデリータ ■
	struct StmtDeleter {
		void operator()(SQLHSTMT h) const {
			if (h != SQL_NULL_HSTMT) SQLFreeHandle(SQL_HANDLE_STMT, h);
		}
	};

	// ■ スマートポインタ ■
	/// @brief ODBCステートメントハンドル。
	std::unique_ptr<void, StmtDeleter> hstmt;

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

	/**
	 * @brief ODBCステートメントハンドル(HSTMT)を取得します。
	 * @details インライン関数にしたいのでヘッダーファイル内で定義しています。
	 * @return ODBCステートメントハンドル。
	 */
	SQLHSTMT getStmtHandle() const {
		return static_cast<SQLHSTMT>(hstmt.get());
	}

	/**
	 * @brief (Helper) 単一行用の固定長パラメータをバインドします。
	 * @details string, vector<char> 以外の型 (int, double, long, etc.) のバインド処理を共通化します。
	 * const std::any& を受け取り、中身の変更は行いません。
	 */
	template<typename T>
	void bindFixedSizeParam(int index, const std::any& any_val, SQLLEN* indicator_ptr);

	/**
	 * @brief (Helper) 一括実行用の固定長パラメータをバインドします。
	 * @details vector<char> 以外 (vector<int>, vector<double>, etc.) のバインド処理を共通化します。
	 * std::any& を受け取りますが、中身のデータ変更は行いません。
	 */
	template<typename T>
	void bindFixedSizeColumn(int index, std::any& any_val, std::vector<SQLLEN>& indicators);
};

// --- テンプレート特殊化実装群 ---
/**
 * @brief 現在の行から指定列のデータを取得します (std::string 専用の特殊化)。
 * @details SQL_C_CHAR を std::string に変換します。
 * @param[in] col 取得する列番号 (1から始まる)。
 * @return データを格納したoptional。データがNULLまたは取得失敗の場合は空。
 */
template<> std::optional<std::string> OdbcStatement::getData<std::string>(int col);

/**
 * @brief 現在の行から指定列のデータを取得します (std::vector<char> 専用の特殊化)。
 * @details SQL_C_BINARY を std::vector<char> に変換します。
 * @param[in] col 取得する列番号 (1から始まる)。
 * @return データを格納したoptional。データがNULLまたは取得失敗の場合は空。
 */
template<> std::optional<std::vector<char>> OdbcStatement::getData<std::vector<char>>(int col);

/**
 * @brief 現在の行から指定列のデータを取得します (time_point 専用の特殊化)。
 * @details SQL_TIMESTAMP_STRUCT を std::chrono::system_clock::time_point に変換します。
 * @param[in] col 取得する列番号 (1から始まる)。
 * @return データを格納したoptional。データがNULLまたは取得失敗の場合は空。
 */
template<> std::optional<std::chrono::system_clock::time_point> OdbcStatement::getData<std::chrono::system_clock::time_point>(int col);

//============================================
// セッション管理クラス
//============================================

/**
 * @struct OdbcSession
 * @brief データベース接続と、任意のキーに関連付けられた複数のステートメントを安全に一元管理する高レベルクラス。
 * @details このクラスのunique_ptrを一つ保持して利用するのが基本的な使い方です。
 * RAIIにより、このクラスのオブジェクトが破棄される際に、全てのステートメントが破棄された後で
 * 接続が安全に破棄されることを保証します。
 * @tparam _Key ステートメントを識別するキーの型 (デフォルト: std::string)。
 * @tparam _Compare キーの比較を行う関数オブジェクト (デフォルト: std::less<_Key>)。
 */
template <typename _Key = std::string, typename _Compare = std::less<_Key>>
struct OdbcSession {
	// --- 型定義 ---
	using map_type					= std::map<_Key, std::unique_ptr<OdbcStatement>, _Compare>;
	using key_type					= typename map_type::key_type;
	using mapped_type				= typename map_type::mapped_type;
	using value_type				= typename map_type::value_type;
	using key_compare				= typename map_type::key_compare;
	using allocator_type			= typename map_type::allocator_type;
	using size_type					= typename map_type::size_type;
	using iterator					= typename map_type::iterator;
	using const_iterator			= typename map_type::const_iterator;
	using reverse_iterator			= typename map_type::reverse_iterator;
	using const_reverse_iterator	= typename map_type::const_reverse_iterator;

	/// @brief このセッションが使用する唯一のデータベース接続。
	std::unique_ptr<OdbcConnection> connection;
	/// @brief 名前付きで複数のステートメントを保持するコンテナ。
	map_type statements;

	/**
	 * @brief コンストラクタ。与えられた接続の所有権を移動します。
	 * @param[in] conn OdbcConnectionへのunique_ptr。
	 */
	explicit OdbcSession(std::unique_ptr<OdbcConnection> conn) :
		connection(std::move(conn))
	{}

	/**
	 * @brief コンストラクタ。与えられた引数を使って内部で接続を確立します。
	 * @param[in] dsn データソース名。
	 * @param[in] user ユーザー名。
	 * @param[in] pass パスワード。
	 * @throw OdbcException 接続に失敗した場合。
	 */
	OdbcSession(const std::string& dsn, const std::string& user, const std::string& pass)
		: connection(std::make_unique<OdbcConnection>(dsn, user, pass))
	{
		// OdbcConnectionのコンストラクタで接続に失敗した場合、
		// ここで例外(OdbcException)がそのままスローされます。
		// connection はまだ完全に構築されていないため、デストラクタは呼ばれませんが、
		// unique_ptr の初期化中なのでメモリリークは起きません。安全です。
	}

	/**
	 * @brief このセッションに新しい名前付きステートメントを追加または取得します。
	 * @param[in] key このステートメントを識別するためのキー。
	 * @param[in] overwrite (オプション・デフォルト false) 同名のステートメントが既に存在する場合に上書きするかどうか。
	 * @param[out] is_new (オプション・デフォルト nullptr) 新規に挿入された場合にtrue、既存だった場合にfalseが設定されます。
	 * @return 作成された、または既存のステートメントへの参照。
	 */
	OdbcStatement& add_statement(const key_type& key, bool overwrite = false, bool* is_new = nullptr) {
		if (!connection) {
			throw std::runtime_error("Cannot add statement: OdbcConnection is null.");
		}

		// 1. まず検索する
		auto it = statements.find(key);

		if (it != statements.end()) {
			// --- 既に存在する場合 ---
			if (overwrite) {
				// 上書き指示があれば、新しいインスタンスで置換する
				it->second = std::make_unique<OdbcStatement>(*connection);
				if (is_new) { *is_new = false; } // "新規キー"ではないが、インスタンスは新しい
			} else {
				// 上書き不要なら何もしない（既存を返す）
				if (is_new) { *is_new = false; }
			}
			return *it->second;
		} else {
			// --- 存在しない場合 ---
			// 新規作成して挿入 (emplace は構築した要素へのイテレータと成功boolのペアを返す)
			auto result = statements.emplace(key, std::make_unique<OdbcStatement>(*connection));
			if (is_new) { *is_new = true; }
			return *result.first->second;
		}
	}
};

} // namespace odbc
