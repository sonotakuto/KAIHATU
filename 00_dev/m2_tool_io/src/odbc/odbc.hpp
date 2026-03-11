/**
 * @file odbc.hpp
 * @brief ODBC接続をC++のクラスでラップし、RAIIパターンで安全にリソース管理を行うためのヘッダーファイル。
 * @details OdbcException, OdbcConnection, OdbcStatement, OdbcSessionのクラス群を提供します。
 */
#pragma once

#include <algorithm>
#include <any>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <unordered_map>
#include <vector>
#include <sqlext.h>	// ODBCのコアヘッダー
#include "utils/string_utils.hpp"

/**
 * @namespace odbc
 * @brief ODBC接続に関連するクラスと関数を格納する名前空間。
 */
namespace odbc{

//============================================
// ログマネージャー
//============================================

/**
 * @struct LogManager
 * @brief ライブラリ内のログ出力を一元管理する静的クラス
 */
struct LogManager {
	/**
	 * @brief ユーザーがログ出力の方法を指定できるようにするためのコールバック関数
	 * @warning スレッドセーフティではないため、マルチスレッドプログラムの場合は、起動直後以外での設定は避けることを推奨します。
	 */
	inline static std::function<void(const std::string&)> error_callback = nullptr;

	/**
	 * @brief エラーメッセージをログ出力するための静的関数。
	 * @param[in] msg 出力したいエラーメッセージ。
	 */
	static void logError(const std::string& msg) {
		if (error_callback) {
			error_callback(msg);
		}
		// 未設定時は何もしない、または std::cerr にフォールバックする等を選択
	}
};

//============================================
// 型特性
//============================================

/**
 * @brief 符号が曖昧な整数型(char, wchar_t)を除外する型特性。
 * @tparam T 判定対象の型。
 */
template<typename T>
inline constexpr bool is_unambiguous_integral_v =
	std::is_integral_v<T> &&
	!std::is_same_v<std::remove_cv_t<T>, char> &&
	!std::is_same_v<std::remove_cv_t<T>, wchar_t>;

/**
 * @brief 浮動小数点型 または 符号が明確な整数型 かどうかを判定する型特性。
 * @tparam T 判定対象の型。
 */
template<typename T>
inline constexpr bool is_explicit_arithmetic_v =
	std::is_floating_point_v<T> || is_unambiguous_integral_v<T>;

/**
 * @brief std::optional<T> かどうかを判定する型特性。
 * @tparam T 判定対象の型。
 */
template <typename T>
struct is_optional : std::false_type {};

/**
 * @brief std::optional<T> かどうかを判定する型特性の特殊化。
 * @tparam T 判定対象の型。
 */
template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

/**
 * @brief std::vector<T> かどうかを判定する型特性。
 * @tparam T 判定対象の型。
 */
template <typename T>
struct is_vector : std::false_type {};

/**
 * @brief std::vector<T> かどうかを判定する型特性の特殊化。
 * @tparam T 判定対象の型。
 */
template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

// 1バイトの要素を持つ std::vector かどうかを判定する型特性
/**
 * @brief 1バイトの要素を持つ std::vector かどうかを判定する型特性。
 * @tparam T 判定対象の型。
 */
template <typename T>
struct is_byte_vector : std::false_type {};

/**
 * @brief 1バイトの要素を持つ std::vector かどうかを判定する型特性の特殊化。
 * @tparam T 判定対象の型。
 */
template <typename T, typename Alloc>
struct is_byte_vector<std::vector<T, Alloc>> : std::integral_constant<bool, sizeof(T) == 1 && std::is_trivial_v<T>> {};

/**
 * @brief is_byte_vector の値を直接取得できる変数テンプレート。
 * @tparam T 判定対象の型。
 */
template <typename T>
inline constexpr bool is_byte_vector_v = is_byte_vector<T>::value;

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
	explicit OdbcConnection(std::string dsn, std::string user, std::string pass);

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
		void operator()(SQLHENV h) const noexcept {
			if (h != SQL_NULL_HENV) { SQLFreeHandle(SQL_HANDLE_ENV, h); }
		}
	};

	// 接続ハンドル用デリータ
	// ※重要: 接続ハンドルは「切断(Disconnect)」してから「解放(Free)」する必要がある
	struct DbcDeleter {
		void operator()(SQLHDBC h) const noexcept {
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

/**
 * @class OdbcTypeTraits
 * @brief 型 T から C型 と SQL型 を決定するための型特性。
 */
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

template<> struct OdbcTypeTraits<SQL_NUMERIC_STRUCT> {
	static constexpr SQLSMALLINT c_type = SQL_C_NUMERIC;
	static constexpr SQLSMALLINT sql_type = SQL_NUMERIC;
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
	// === 定数 ===
	constexpr static int ATTR_PARAMSET_SIZE_MAX_VALUE = 32767 ;	// ODBC 3.0で定義されている最大値

	// === 内部型定義 ===
	/**
	 * @class ColumnValue
	 * @brief パラメータバインド時に計算されたメタデータを保持する構造体
	 */
	struct ParamMetadata {
		SQLUSMALLINT io_type;
		SQLSMALLINT c_type;
		SQLSMALLINT sql_type;
		SQLULEN col_size;
		SQLSMALLINT decimal_digits;
		SQLLEN buf_length;
	};

	// === コンストラクタ・デストラクタ ===

	/**
	 * @brief OdbcConnectionからステートメントハンドルを生成するコンストラクタ。
	 * @param[in] conn 親となるOdbcConnectionオブジェクト。
	 * @throw OdbcException ステートメントハンドルの生成に失敗した場合。
	 */
	explicit OdbcStatement(const OdbcConnection& conn);

	/**
	 * @brief ステートメントハンドルを解放するデストラクタ。
	 */
	~OdbcStatement() {
		// メンバ変数（C++側のバッファや配列）が破棄されるよりも先に、
		// ステートメントハンドルを強制的に解放する。
		// これにより、ODBCドライバが持つすべてのポインタ参照が確実に断ち切られる。
		hstmt.reset();
	}

	// === コピー・ムーブセマンティクス ===

	// コピーは禁止
	OdbcStatement(const OdbcStatement&) = delete;
	OdbcStatement& operator=(const OdbcStatement&) = delete;

	// ムーブは許可
	OdbcStatement(OdbcStatement&&) = default;
	OdbcStatement& operator=(OdbcStatement&&) = default;

	// === 公開メンバ関数

	/**
	 * @brief SQL文を直接実行します。
	 * @details 結果セットを返さないINSERT/UPDATE/DELETE文や、パラメータを含まないSELECT文に使用します。
	 * @param[in] sql 実行するSQL文字列。
	 * @return 影響を受けた行数。
	 * @throw OdbcException クエリの実行に失敗した場合。
	 */
	int execute(std::string sql);

	/**
	 * @brief SELECT文の結果セットから次の行にカーソルを移動させます。
	 * @return 次の行が存在すればtrue、存在しなければfalse。
	 * @throw OdbcException フェッチ処理中にエラーが発生した場合。
	 */
	bool fetch();

	/**
	 * @brief 現在の行から指定列のデータを取得します。
	 * @tparam T 取得したいC++のデータ型 (例: int, std::string, std::chrono::system_clock::time_pointなど)。
	 * @param[in] col 取得する列番号 (1から始まる)。
	 * @return データを格納したoptional。データがNULLまたは取得失敗の場合は空。
	 * @throw OdbcException データの取得に失敗した場合。
	 */
	template<typename T>
	std::optional<T> getData(int col) {
		if constexpr (is_byte_vector_v<T>) {
			// --- BLOB 用の処理 (1バイトのvectorなら何でも対応) ---
			T buffer;
			SQLLEN indicator;
			SQLRETURN ret;
			const int CHUNK_SIZE = 4096;	// 一度に読み込むサイズ

			// T::value_type を使うことで、uint8_t, char, std::byte 全てに自動適応します
			typename T::value_type chunk[CHUNK_SIZE];

			// まずデータがNULLでないかを確認するために、バッファ長0で呼び出す
			ret = SQLGetData(getStmtHandle(), col, SQL_C_BINARY, chunk, 0, &indicator);
			if (indicator == SQL_NULL_DATA) { return std::nullopt; }

			// データがNULLでなければ、SQL_NO_DATAが返されるまでチャンク単位で全て読み出す
			while ((ret = SQLGetData(getStmtHandle(), col, SQL_C_BINARY, chunk, CHUNK_SIZE, &indicator)) != SQL_NO_DATA) {
				if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
					throw OdbcException("Failed to get BLOB data", getStmtHandle(), SQL_HANDLE_STMT);
				}
				// indicator には SQLGetData 呼び出し時点での読み取り可能バイト数が入る
				long bytesInChunk = (indicator > CHUNK_SIZE || indicator < 0) ? (CHUNK_SIZE) : (indicator);
				buffer.insert(buffer.end(), chunk, chunk + bytesInChunk);
			}
			return buffer;

		} else if constexpr (std::is_same_v<T, std::string>) {
			// --- 文字列 用の処理 ---
			std::string buffer;
			SQLLEN indicator;
			SQLRETURN ret;
			const int CHUNK_SIZE = 4096;
			char chunk[CHUNK_SIZE];

			// まずデータがNULLでないかを確認するために、バッファ長0で呼び出す
			ret = SQLGetData(getStmtHandle(), col, SQL_C_CHAR, chunk, 0, &indicator);
			if (indicator == SQL_NULL_DATA) return std::nullopt;

			// データがNULLでなければ、SQL_NO_DATAが返されるまでチャンク単位で全て読み出す
			while ((ret = SQLGetData(getStmtHandle(), col, SQL_C_CHAR, chunk, CHUNK_SIZE, &indicator)) != SQL_NO_DATA) {
				if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
					throw OdbcException("Failed to get string data", getStmtHandle(), SQL_HANDLE_STMT);
				}

				// SQL_C_CHARの場合、ドライバは必ず末尾にNUL文字('\0')を付与する。
				// したがって、今回のチャンクで取得できた有効な文字数は最大で CHUNK_SIZE - 1 となる。
				long bytesInChunk = (indicator >= CHUNK_SIZE || indicator < 0) ? (CHUNK_SIZE - 1) : (indicator);

				// 有効な文字部分だけをバッファに追記
				buffer.append(chunk, bytesInChunk);
			}
			return buffer;

		} else if constexpr (std::is_same_v<T, std::chrono::system_clock::time_point>) {
			// --- 日時 用の処理 ---
			SQL_TIMESTAMP_STRUCT ts;
			SQLLEN indicator;
			SQLRETURN ret = SQLGetData(getStmtHandle(), col, SQL_C_TYPE_TIMESTAMP, &ts, sizeof(ts), &indicator);

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
				throw OdbcException("Failed to get data", getStmtHandle(), SQL_HANDLE_STMT);
			}
			if (indicator == SQL_NULL_DATA) { return std::nullopt; }
			return to_time_point(ts);

		} else {
			// --- 汎用数値型 用の処理 (従来の OdbcTypeTraits を使用) ---
			T value;
			SQLLEN indicator;
			constexpr SQLSMALLINT c_type = OdbcTypeTraits<T>::c_type;
			SQLRETURN ret = SQLGetData(getStmtHandle(), col, c_type, &value, sizeof(value), &indicator);

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
				throw OdbcException("Failed to get data", getStmtHandle(), SQL_HANDLE_STMT);
			}
			if (indicator == SQL_NULL_DATA) { return std::nullopt; }
			return value;
		}
	}

	/**
	 * @brief 指定された列名のデータを取得します。
	 * @tparam T 取得したい型。
	 * @param columnName 列名（大文字小文字を区別しません）。
	 * @return 取得したデータの optional。
	 * @throw std::invalid_argument 指定された列名が存在しない場合。
	 * @throw OdbcException データの取得に失敗した場合。
	 */
	template<typename T>
	std::optional<T> getData(std::string columnName) {
		// 未初期化なら自動でキャッシュを構築
		if (!column_map_initialized) {
			initColumnMap();
		}

		// 検索キーも小文字化
		std::transform(columnName.begin(), columnName.end(), columnName.begin(),
			[](unsigned char c) { return std::tolower(c); }
		);

		auto it = column_map_cache.find(columnName);
		if (it == column_map_cache.end()) {
			throw std::invalid_argument("Column name not found: " + columnName);
		}

		// 既存のインデックス版 getData に処理を委譲
		return getData<T>(it->second);
	}

	/**
	 * @brief 現在開いているカーソルを閉じ、結果セットを破棄します。
	 * @details SELECT文の結果を最後まで読まずに次のクエリを実行する場合などに呼び出します。
	 * カーソルが既に閉じていてもエラーになりません（べき等性）。
	 * @throw OdbcException カーソルを閉じる際に予期せぬエラーが発生した場合。
	 */
	void closeCursor();

	/**
	 * @brief 行データを任意の型に変換するマッパー関数を受け取り、全行を取得します。
	 * @details 内部で列名キャッシュを自動構築し、型安全かつ高速に結果セットを C++ のオブジェクト配列に変換します。
	 * @tparam Mapper OdbcStatement& を受け取り、任意の型 T を返す関数オブジェクト(ラムダ式など)。
	 * @param mapper 行ごとのマッピング処理を記述した関数オブジェクト。
	 * @return マッパー関数が返した型 T の std::vector。
	 * @throw OdbcException データの取得中にエラーが発生した場合。
	 */
	template <typename Mapper>
	auto fetchMapped(Mapper mapper) {
		// 1. ループの前に列名キャッシュを構築（未初期化の場合のみ実行される）
		if (!column_map_initialized) {
			initColumnMap();
		}

		// 2. マッパー関数（ラムダ式など）の戻り値の型をコンパイラに推論させる
		using ResultType = decltype(mapper(*this));

		std::vector<ResultType> results;

		// 3. 全行をフェッチし、現在のステートメント参照をマッパー関数に渡して結果を構築
		while (this->fetch()) {
			results.push_back(mapper(*this));
		}

		// 4. 取得完了後にカーソルを閉じる
		this->closeCursor();

		return results;
	}

	/**
	 * @brief SELECT文の結果セット全行を、辞書型の配列として一括取得します。
	 * @details 内部で最後までfetchし、自動でカーソルを閉じるため、呼び出し後にcloseCursorは不要です。
	 * @warning 【注意】大量の行を取得する場合、メモリ確保のオーバーヘッドにより、著しいパフォーマンス低下を招く恐れがあります。
	 * \n
	 * 数百行を超えるデータや、本番環境でのメインクエリには、静的型付けの `fetchMapped` の使用を強く推奨します。
	 * @return 全ての行データ。
	 * @throw OdbcException データ取得中にエラーが発生した場合。
	 */
	std::vector<std::map<std::string, std::any>> fetchDynamic();

	// --- プリペアドステートメント関連 ---

	/**
	 * @brief プリペアドステートメント用のSQL文を準備（コンパイル）します。
	 * @param[in] sql プレースホルダ(?)を含むSQL文。
	 * @throw OdbcException SQL文の準備に失敗した場合。
	 */
	void prepare(std::string sql);

	/**
	 * @brief 一括挿入(バルクインサート)で一度に処理する行数を設定します。
	 * \n
	 * 失敗時は、ParamsProcessedとParamStatusesの値は不定になります。
	 * @param[in] size 挿入する行数。1以下の場合は一括処理を無効にして単一行処理に戻ります。最大値はATTR_PARAMSET_SIZE_MAX_VALUEで定義されています。
	 * @throw std::invalid_argument sizeがODBCの最大値を超える場合。
	 * @throw OdbcException ODBCドライバが属性の設定に失敗した場合。
	 */
	void setBatchSize(size_t size);

	/**
	 * @brief 一括挿入(バルクインサート)の設定を解除します。
	 * \n
	 * 失敗時は、ParamsProcessedとParamStatusesの値は不定になります。
	 * @throw OdbcException ODBCドライバが属性の設定に失敗した場合。
	 */
	void resetBatchSize() {
		setBatchSize(1);
	}

	// --- 未定義型用エスケープハッチ (bindParamAsType) ---

	/**
	 * @brief 未定義の型や特殊な型(DECIMALなど)をバインドするための汎用エスケープハッチ (単一行用・NULL非対応版)。
	 * @details C++側からは文字列としてデータを渡し、ODBCドライバに指定したSQL型への変換を委ねます。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] value バインドするパラメータの値。
	 * @param[in] target_sql_type バインドするパラメータのSQL型 (例: SQL_DECIMAL, SQL_VARCHARなど)。
	 * @throw std::out_of_range paramIndexが1未満、もしくは文字列長がODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	void bindParamAsType(int paramIndex, const std::string& value, SQLSMALLINT target_sql_type) {
		checkSingleRowMode();
		bindParamAsType(paramIndex, std::make_optional(value), target_sql_type);
	}

	/**
	 * @brief 未定義の型や特殊な型(DECIMALなど)をバインドするための汎用エスケープハッチ (単一行用・NULL対応版)。
	 * @details C++側からは文字列としてデータを渡し、ODBCドライバに指定したSQL型への変換を委ねます。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] opt_val バインドするパラメータの値。NULLをバインドしたければ nullopt。
	 * @param[in] target_sql_type バインドするパラメータのSQL型 (例: SQL_DECIMAL, SQL_VARCHARなど)。
	 * @param[in] column_size (オプション・デフォルト 0) 文字列型の場合の最大長。数値型の場合は全体の桁数。DECIMAL型などで有効。0の場合はODBCドライバに任せる。
	 * @param[in] decimal_digits (オプション・デフォルト 0) 数値型の場合の小数点以下の桁数。DECIMAL型などで有効。
	 * @throw std::out_of_range paramIndexが1未満、もしくは文字列長がODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	void bindParamAsType(int paramIndex, const std::optional<std::string>& opt_val, SQLSMALLINT target_sql_type, SQLULEN column_size = 0, SQLSMALLINT decimal_digits = 0);

	/**
	 * @brief 未定義の型や特殊な型(DECIMALなど)をバインドするための汎用エスケープハッチ (一括実行用・NULL非対応版)。
	 * @details C++側からは文字列としてデータを渡し、ODBCドライバに指定したSQL型への変換を委ねます。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] values バインドするデータのvector。
	 * @param[in] target_sql_type バインドするパラメータのSQL型 (例: SQL_DECIMAL, SQL_VARCHARなど)。
	 * @param[in] column_size (オプション・デフォルト 0) 文字列型の場合の最大長。数値型の場合は全体の桁数。DECIMAL型などで有効。0の場合はODBCドライバに任せる。
	 * @param[in] decimal_digits (オプション・デフォルト 0) 数値型の場合の小数点以下の桁数。DECIMAL型などで有効。
	 * @throw std::out_of_range paramIndexが1未満、もしくは文字列長がODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 */
	void bindParamAsType(int paramIndex, const std::vector<std::string>& values, SQLSMALLINT target_sql_type, SQLULEN column_size = 0, SQLSMALLINT decimal_digits = 0);

	/**
	 * @brief 未定義の型や特殊な型(DECIMALなど)をバインドするための汎用エスケープハッチ (一括実行用・NULL対応版)。
	 * @details C++側からは文字列としてデータを渡し、ODBCドライバに指定したSQL型への変換を委ねます。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 * @param[in] target_sql_type バインドするパラメータのSQL型 (例: SQL_DECIMAL, SQL_VARCHARなど)。
	 * @param[in] column_size (オプション・デフォルト 0) 文字列型の場合の最大長。数値型の場合は全体の桁数。DECIMAL型などで有効。0の場合はODBCドライバに任せる。
	 * @param[in] decimal_digits (オプション・デフォルト 0) 数値型の場合の小数点以下の桁数。DECIMAL型などで有効。
	 * @throw std::out_of_range paramIndexが1未満、もしくは文字列長がODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 */
	void bindParamAsType(int paramIndex, const std::vector<std::optional<std::string>>& opt_vals, SQLSMALLINT target_sql_type, SQLULEN column_size = 0, SQLSMALLINT decimal_digits = 0);

	// === 数値・汎用型バインド ===

	/**
	 * @brief パラメータをバインドします (NULL非対応版)。
	 * @details value を std::optional で包んで、適切なNULL対応版のオーバーロードへ処理を委譲します。
	 * @tparam T バインドするパラメータの型。数値型(int, doubleなど)に限定されます。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] value バインドするパラメータの値。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	template<typename T,
		// SFINAE: Tが数値型(int, double等)の場合のみ、このテンプレートを有効にする
		std::enable_if_t<is_explicit_arithmetic_v<T>, int> = 0
	>
	void bindParam(int paramIndex, const T& value) {
		checkSingleRowMode();
		bindParam(paramIndex, std::make_optional(value));
	}

	/**
	 * @brief パラメータをバインドします (NULL対応版)。
	 * @tparam T バインドするパラメータの型。数値型(int, doubleなど)に限定されます。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] opt_value バインドするパラメータの値。NULLをバインドしたければ nullopt。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	template<typename T,
		// SFINAE: Tが数値型(int, double等)の場合のみ、このテンプレートを有効にする
		std::enable_if_t<is_explicit_arithmetic_v<T>, int> = 0
	>
	void bindParam(int paramIndex, const std::optional<T>& opt_val) {
		checkSingleRowMode();
		bindSingleCore(
			paramIndex,
			opt_val,
			static_cast<SQLLEN>((opt_val.has_value()) ? (sizeof(T)) : (SQL_NULL_DATA)),
			ParamMetadata{SQL_PARAM_INPUT, OdbcTypeTraits<T>::c_type, OdbcTypeTraits<T>::sql_type, sizeof(T), 0, static_cast<SQLLEN>(sizeof(T))}
		);
	}

	/**
	 * @brief 一括挿入用に、パラメータの配列を列単位でバインドします (NULL非対応版)。
	 * @details int, double, long long などの固定長で連続メモリに配置される型を扱うための汎用テンプレートです。
	 * @tparam T バインドするパラメータの型。数値型(int, doubleなど)に限定されます。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] values バインドするデータのvector。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 */
	template<typename T,
		// SFINAE: Tが数値型(int, double等)の場合のみ、このテンプレートを有効にする
		std::enable_if_t<is_explicit_arithmetic_v<T>, int> = 0
	>
	void bindParam(int paramIndex, const std::vector<T>& values) {
		checkBatchSize(values.size());
		if (values.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
			bindParam(paramIndex, values[0]);
			return;
		}

		bindBatchCore(
			paramIndex,
			values,
			std::vector<SQLLEN>(values.size(), static_cast<SQLLEN>(sizeof(T))),
			ParamMetadata{SQL_PARAM_INPUT, OdbcTypeTraits<T>::c_type, OdbcTypeTraits<T>::sql_type, sizeof(T), 0, static_cast<SQLLEN>(sizeof(T))}
		);
	}

	/**
	 * @brief 一括挿入用に、パラメータの配列を列単位でバインドします (NULL対応版)。
	 * @details int, double, long long などの固定長で連続メモリに配置される型を扱うための汎用テンプレートです。
	 * @tparam T バインドするパラメータの型。数値型(int, doubleなど)に限定されます。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 */
	template<typename T,
		// SFINAE: Tが数値型(int, double等)の場合のみ、このテンプレートを有効にする
		std::enable_if_t<is_explicit_arithmetic_v<T>, int> = 0
	>
	void bindParam(int paramIndex, const std::vector<std::optional<T>>& opt_vals) {
		if (opt_vals.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
			bindParam(paramIndex, opt_vals[0]);
			return;
		}

		checkBatchSize(opt_vals.size());
		// 1. データ配列とインジケータ配列を作成
		std::vector<T> buffer(opt_vals.size());
		std::vector<SQLLEN> indicators(opt_vals.size());
		for (size_t i = 0; i < opt_vals.size(); i++) {
			if (opt_vals[i].has_value()) {
				buffer[i] = opt_vals[i].value();
				// 固定長データの場合、インジケータは0で良いが、一応サイズをセットしておく
				indicators[i] = static_cast<SQLLEN>(sizeof(T));
			} else {
				// buffer[i] はデフォルト値(0など)のまま
				indicators[i] = SQL_NULL_DATA;
			}
		}
		// 2. 作成した配列をバッファに保存
		bindBatchCore(
			paramIndex,
			std::move(buffer),
			std::move(indicators),
			ParamMetadata{SQL_PARAM_INPUT, OdbcTypeTraits<T>::c_type, OdbcTypeTraits<T>::sql_type, sizeof(T), 0, static_cast<SQLLEN>(sizeof(T))}
		);
	}

	// === 文字列 ===

	/**
	 * @brief 文字列パラメータをバインドします (NULL非対応版)。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] value バインドするパラメータの値。
	 * @throw std::invalid_argument valueがnullptrの場合。NULLをバインドしたい場合は、std::optional<std::string>を使用してください。
	 * @throw std::out_of_range paramIndexが1未満、もしくは文字列長がODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	void bindParam(int paramIndex, const char* value);

	/**
	 * @brief 文字列パラメータをバインドします (NULL非対応版)。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] value バインドするパラメータの値。
	 * @throw std::invalid_argument valueがnullptrの場合。NULLをバインドしたい場合は、std::optional<std::string>を使用してください。
	 * @throw std::out_of_range paramIndexが1未満、もしくは文字列長がODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	void bindParam(int paramIndex, const signed char* value) {
		checkSingleRowMode();
		bindParam(paramIndex, reinterpret_cast<const char*>(value));
	}

	/**
	 * @brief 文字列パラメータをバインドします (NULL非対応版)。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] value バインドするパラメータの値。
	 * @throw std::invalid_argument valueがnullptrの場合。NULLをバインドしたい場合は、std::optional<std::string>を使用してください。
	 * @throw std::out_of_range paramIndexが1未満、もしくは文字列長がODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	void bindParam(int paramIndex, const unsigned char* value) {
		checkSingleRowMode();
		bindParam(paramIndex, reinterpret_cast<const char*>(value));
	}

	/**
	 * @brief 文字列パラメータをバインドします (NULL非対応版)。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] value バインドするパラメータの値。
	 * @throw std::out_of_range paramIndexが1未満、もしくは文字列長がODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	void bindParam(int paramIndex, const std::string& value) {
		checkSingleRowMode();
		bindParam(paramIndex, std::make_optional(value));
	}

	/**
	 * @brief 文字列パラメータをバインドします (NULL対応版)。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] opt_val バインドするパラメータの値。NULLをバインドしたければ nullopt。
	 * @throw std::out_of_range paramIndexが1未満、もしくは文字列長がODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	void bindParam(int paramIndex, const std::optional<std::string>& opt_val);

	/**
	 * @brief 一括挿入用に文字列の配列をバインドします (NULL非対応版、string 専用オーバーロード)。
	 * @details 可変長の文字列をODBCが要求する固定長の連続メモリにコピーする処理を行います。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] values バインドするデータのvector。
	 * @throw std::out_of_range paramIndexが1未満、もしくは文字列長がODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 */
	void bindParam(int paramIndex, const std::vector<std::string>& values);

	/**
	 * @brief 一括挿入用に文字列の配列をバインドします (NULL対応版、string 専用オーバーロード)。
	 * @details 可変長の文字列をODBCが要求する固定長の連続メモリにコピーする処理を行います。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 * @throw std::out_of_range paramIndexが1未満、もしくは文字列長がODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 */
	void bindParam(int paramIndex, const std::vector<std::optional<std::string>>& opt_vals);

	// === 日時型 (TIMESTAMP_STRUCT, time_point) - 精度指定対応 ===

	/**
	 * @brief パラメータをバインドします (NULL非対応版、SQL_TIMESTAMP_STRUCT 専用オーバーロード)。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] value バインドするパラメータの値。
	 * @param[in] precision (オプション・デフォルト0) ODBCのTIMESTAMP型の小数点以下の桁数を指定します。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	void bindParam(int paramIndex, const SQL_TIMESTAMP_STRUCT& value, SQLSMALLINT precision = 0) {
		checkSingleRowMode();
		bindParam(paramIndex, std::make_optional(value), precision);
	}

	/**
	 * @brief パラメータをバインドします (NULL対応版、SQL_TIMESTAMP_STRUCT 専用オーバーロード)。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] opt_val バインドするパラメータの値 (time_point)。NULLをバインドしたければ nullopt。
	 * @param[in] precision (オプション・デフォルト0) ODBCのTIMESTAMP型の小数点以下の桁数を指定します。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	void bindParam(int paramIndex, const std::optional<SQL_TIMESTAMP_STRUCT>& opt_val, SQLSMALLINT precision = 0);

	/**
	 * @brief 一括挿入用に日時の配列をバインドします (NULL非対応版、SQL_TIMESTAMP_STRUCT 専用オーバーロード)。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] values バインドするデータのvector。
	 * @param[in] precision (オプション・デフォルト0) ODBCのTIMESTAMP型の小数点以下の桁数を指定します。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 */
	void bindParam(int paramIndex, const std::vector<SQL_TIMESTAMP_STRUCT>& values, SQLSMALLINT precision = 0);

	/**
	 * @brief 一括挿入用に日時の配列をバインドします (NULL対応版、SQL_TIMESTAMP_STRUCT 専用オーバーロード)。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 * @param[in] precision (オプション・デフォルト0) ODBCのTIMESTAMP型の小数点以下の桁数を指定します。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 */
	void bindParam(int paramIndex, const std::vector<std::optional<SQL_TIMESTAMP_STRUCT>>& opt_vals, SQLSMALLINT precision = 0);

	/**
	 * @brief パラメータをバインドします (NULL非対応版、time_point 専用オーバーロード)。
	 * @details time_point を ODBC用の TIMESTAMP 構造体に変換する処理を行ってからバインドします。
	 * 変換処理に失敗した場合はバインドしません。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] value バインドするパラメータの値。
	 * @param[in] precision (オプション・デフォルト0) ODBCのTIMESTAMP型の小数点以下の桁数を指定します。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 * @throw std::invalid_argument value を SQL_TIMESTAMP_STRUCT に変換できない場合。
	 */
	void bindParam(int paramIndex, const std::chrono::system_clock::time_point& value, SQLSMALLINT precision = 0) {
		checkSingleRowMode();
		bindParam(paramIndex, std::make_optional(value), precision);
	}

	/**
	 * @brief パラメータをバインドします (NULL対応版、time_point 専用オーバーロード)。
	 * @details time_point を ODBC用の TIMESTAMP 構造体に変換する処理を行ってからバインドします。
	 * 変換処理に失敗した場合はバインドしません。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] opt_val バインドするパラメータの値 (time_point)。NULLをバインドしたければ nullopt。
	 * @param[in] precision (オプション・デフォルト0) ODBCのTIMESTAMP型の小数点以下の桁数を指定します。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 * @throw std::invalid_argument value を SQL_TIMESTAMP_STRUCT に変換できない場合。
	 */
	void bindParam(int paramIndex, const std::optional<std::chrono::system_clock::time_point>& opt_val, SQLSMALLINT precision = 0);

	/**
	 * @brief 一括挿入用に日時の配列をバインドします (NULL非対応版、time_point 専用オーバーロード)。
	 * @details time_point を TIMESTAMP 構造体に変換する処理を行ってからバインドします。
	 * 変換処理に失敗した場合はバインドしません。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] values バインドするデータのvector。
	 * @param[in] precision (オプション・デフォルト0) ODBCのTIMESTAMP型の小数点以下の桁数を指定します。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 * @throw std::invalid_argument value を SQL_TIMESTAMP_STRUCT に変換できない場合。
	 */
	void bindParam(int paramIndex, const std::vector<std::chrono::system_clock::time_point>& values, SQLSMALLINT precision = 0);

	/**
	 * @brief 一括挿入用に日時の配列をバインドします (NULL対応版、time_point 専用オーバーロード)。
	 * @details time_point を TIMESTAMP 構造体に変換する処理を行ってからバインドします。
	 * 変換処理に失敗した場合はバインドしません。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 * @param[in] precision (オプション・デフォルト0) ODBCのTIMESTAMP型の小数点以下の桁数を指定します。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 * @throw std::invalid_argument value を SQL_TIMESTAMP_STRUCT に変換できない場合。
	 */
	void bindParam(int paramIndex, const std::vector<std::optional<std::chrono::system_clock::time_point>>& opt_vals, SQLSMALLINT precision = 0);

	// === long double (非推奨) ===

	/**
	 * @brief パラメータをバインドします (NULL対応版、long double 専用オーバーロード)。
	 * @details long double を double に丸めてからバインドします。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] opt_val バインドするパラメータの値。NULLをバインドしたければ nullopt。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	[[deprecated("Warning: 'long double' is truncated to 'double' for ODBC binding.")]]
	void bindParam(int paramIndex, const std::optional<long double>& opt_val);

	/**
	 * @brief 一括挿入用に long double の配列をバインドします (NULL非対応版、long double 専用オーバーロード)。
	 * @details long double を double に丸めてからバインドします。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 */
	[[deprecated("Warning: 'long double' is truncated to 'double' for ODBC binding.")]]
	void bindParam(int paramIndex, const std::vector<long double>& opt_vals);

	/**
	 * @brief 一括挿入用に long double の配列をバインドします (NULL対応版、long double 専用オーバーロード)。
	 * @details long double を double に丸めてからバインドします。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 */
	[[deprecated("Warning: 'long double' is truncated to 'double' for ODBC binding.")]]
	void bindParam(int paramIndex, const std::vector<std::optional<long double>>& opt_vals);

	// === BLOB (バイナリデータ) ===

	/**
	 * @brief 単一行のBLOBデータを生ポインタとサイズからバインドします。(ポインタ版)
	 * \n
	 * - data == nullptr の場合: データベースには「NULL」が挿入されます。
	 * \n
	 * - data != nullptr かつ size == 0 の場合: データベースには「空データ(長さ0)」が挿入されます。
	 * @details 要素サイズが1バイトのvector (char, unsigned char, std::byte等) を全て受け入れます。
	 * @tparam T バインドするBLOBデータの要素型 (char, unsigned char, std::byteなど)。要素サイズが1バイトで、かつトリビアルな型のみ許可されます。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] data バインドするBLOBデータへのポインタ。NULLをバインドしたければ nullptr。
	 * @param[in] size バインドするBLOBデータのサイズ。dataがnullptrの場合は無視されます。
	 * @throw std::invalid_argument dataがnullptrの場合。NULLをバインドしたい場合は、std::optional<std::vector<T>>を使用してください。
	 * @throw std::out_of_range paramIndexが1未満、もしくは size がODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	template<typename T,
		// SFINAE: 要素のサイズが1バイト、かつメモリコピー可能なトリビアル型のみ許可
		std::enable_if_t<sizeof(T) == 1 && std::is_trivial_v<T>, int> = 0
	>
	void bindBlobParam(int paramIndex, const T* data, size_t size) {
		checkSingleRowMode();
		// SQLLEN (符号付き) で表現できる最大値を超えていないかチェック
		if (size > static_cast<size_t>(std::numeric_limits<SQLLEN>::max())) {
			throw std::out_of_range("BLOB size exceeds the maximum capacity of SQLLEN.");
		}

		std::optional<std::vector<std::uint8_t>> buffer_opt;
		SQLLEN indicator{};
		SQLULEN col_size{};
		SQLLEN buf_length{};

		if (data == nullptr) {
			throw std::invalid_argument("bindBlobParam: data pointer must not be nullptr. Use std::optional for database NULL.");
		}

		// --- データが存在する場合 (サイズ0の空データも含む) ---
		std::vector<std::uint8_t> normalized_vec;

		if (size > 0) {
			// サイズが1以上の場合はメモリをコピー
			normalized_vec.assign(
				reinterpret_cast<const std::uint8_t*>(data),
				reinterpret_cast<const std::uint8_t*>(data) + size
			);
		}

		// sizeが0の場合は空の normalized_vec が、それ以外はコピーされたデータが保存される
		buf_length = std::max(1UL, size);
		indicator = static_cast<SQLLEN>(size);
		buffer_opt = std::move(normalized_vec);
		col_size = std::max(1UL, size);

		// メタデータ生成
		bindSingleCore(
			paramIndex,
			std::move(buffer_opt),
			indicator,
			ParamMetadata{SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, col_size, 0, static_cast<SQLLEN>(buf_length)}
		);
	}

	/**
	 * @brief 単一行のBLOBデータを生ポインタとサイズからバインドします。(void* 専用オーバーロード)
	 * @details C言語APIなどから取得した汎用ポインタをキャストなしで受け入れます。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] data バインドするBLOBデータへのポインタ。NULLをバインドしたければ nullptr。
	 * @param[in] size バインドするBLOBデータのサイズ。dataがnullptrの場合は無視されます。
	 * @throw std::invalid_argument dataがnullptrの場合。NULLをバインドしたい場合は、std::optional<std::vector<T>>を使用してください。
	 * @throw std::out_of_range paramIndexが1未満、もしくは size がODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	inline void bindBlobParam(int paramIndex, const void* data, size_t size) {
		checkSingleRowMode();
		// 内部で std::uint8_t* にキャストして、テンプレート版のポインタ用関数へ処理を丸投げする
		bindBlobParam(paramIndex, static_cast<const std::uint8_t*>(data), size);
	}

	/**
	 * @brief 単一行のBLOBデータをバインドします。(NULL対応 Vector版)
	 * @details 要素サイズが1バイトのvector (char, unsigned char, std::byte等) を全て受け入れます。
	 * @tparam T バインドするBLOBデータの要素型 (char, unsigned char, std::byteなど)。要素サイズが1バイトで、かつトリビアルな型のみ許可されます。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] opt_val バインドするBLOBデータのvector。NULLをバインドしたければ nullopt。
	 * @throw std::out_of_range paramIndexが1未満、もしくはBLOBのサイズがODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	template<typename T,
		// SFINAE: 要素のサイズが1バイト、かつメモリコピー可能なトリビアル型のみ許可
		std::enable_if_t<sizeof(T) == 1 && std::is_trivial_v<T>, int> = 0
	>
	void bindBlobParam(int paramIndex, const std::optional<std::vector<T>>& opt_val) {
		checkSingleRowMode();
		if (opt_val.has_value()) {
			const auto& vec = opt_val.value();
			const T* ptr = vec.data();

			// 【安全対策】空のベクターの data() が nullptr を返した場合、NULL扱いされるのを防ぐため、
			// 確実に 非nullptr なダミーアドレスにすり替えておく
			if (vec.empty()) {
				static const T dummy_byte{0};
				ptr = &dummy_byte;
			}

			// ポインタ版へ委譲 (空データの場合は ptr=ダミー, size=0 が渡る)
			bindBlobParam(paramIndex, ptr, vec.size());
		} else {
			// ポインタ版には委譲せず、ここで直接 NULL として登録する
			bindSingleCore(
				paramIndex,
				std::optional<std::vector<std::uint8_t>>(),
				SQL_NULL_DATA,
				ParamMetadata{SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, 1, 0, 0}
			);
		}
	}

	/**
	 * @brief 単一行のBLOBデータをバインドします。(NULL非対応版)
	 * @details 要素サイズが1バイトのvector (char, unsigned char, std::byte等) を全て受け入れます。
	 * @tparam T バインドするBLOBデータの要素型 (char, unsigned char, std::byteなど)。要素サイズが1バイトで、かつトリビアルな型のみ許可されます。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] value バインドするBLOBデータのvector。
	 * @throw std::out_of_range paramIndexが1未満、もしくはBLOBのサイズがODBCパラメータの最大許容サイズを超える場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	template<typename T,
		// SFINAE: 要素のサイズが1バイト、かつメモリコピー可能なトリビアル型のみ許可
		std::enable_if_t<sizeof(T) == 1 && std::is_trivial_v<T>, int> = 0
	>
	void bindBlobParam(int paramIndex, const std::vector<T>& value) {
		checkSingleRowMode();
		bindBlobParam(paramIndex, std::make_optional(value));
	}

	/**
	 * @brief 一括挿入用にBLOBの配列をバインドします (NULL非対応版)。
	 * @details 要素サイズが1バイトのvector (char, unsigned char, std::byte等) の配列を受け入れ、固定長の連続メモリにコピーする処理を行います。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] values バインドするデータのvector。
	 * @throw std::out_of_range paramIndexが1未満、もしくはBLOBのサイズがSQLLENの最大値を超える場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 */
	template<typename T,
		// SFINAE: 要素のサイズが1バイト、かつメモリコピー可能なトリビアル型のみ許可
		std::enable_if_t<sizeof(T) == 1 && std::is_trivial_v<T>, int> = 0
	>
	void bindBlobParam(int paramIndex, const std::vector<std::vector<T>>& values) {
		checkBatchSize(values.size());
		if (values.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
			bindBlobParam(paramIndex, values[0]);
			return;
		}

		// 1. 全てのBLOBを格納できる最大の長さを特定
		size_t max_len = 0;
		for (const auto& value : values) {
			if (value.size() > max_len) {
				max_len = value.size();
			}
		}
		if (max_len == 0) { max_len = 1; }

		// SQLLEN (符号付き) で表現できる最大値を超えていないかチェック
		if (max_len > static_cast<size_t>(std::numeric_limits<SQLLEN>::max())) {
			throw std::out_of_range("BLOB size exceeds the maximum capacity of SQLLEN.");
		}

		// 2. 連続したメモリブロック (正規化型 std::uint8_t) をゼロクリアで確保
		std::vector<std::uint8_t> flat_buffer(values.size() * max_len, 0);
		std::vector<SQLLEN> indicators(values.size());

		// 3. 各データをメモリブロックの正しい位置にコピーする
		for (size_t i = 0; i < values.size(); i++) {
			if (!values[i].empty()) {
				memcpy(flat_buffer.data() + (i * max_len), values[i].data(), values[i].size());
			}
			indicators[i] = static_cast<SQLLEN>(values[i].size());
		}

		// 4. 構築した連続メモリブロックとストライドを保存
		bindBatchCore(
			paramIndex,
			std::move(flat_buffer),
			std::move(indicators),
			ParamMetadata{SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, max_len, 0, static_cast<SQLLEN>(max_len)}
		);
	}

	/**
	 * @brief 一括挿入用にBLOBの配列をバインドします (NULL対応版)。
	 * @details 要素サイズが1バイトのvector (char, unsigned char, std::byte等) の配列を受け入れ、固定長の連続メモリにコピーする処理を行います。
	 * @param[in] paramIndex パラメータのインデックス (1から始まる)。
	 * @param[in] opt_vals バインドするデータのvector。NULLをバインドしたければ、そのデータは nullopt にする。
	 * @throw std::out_of_range paramIndexが1未満、もしくはBLOBのサイズがSQLLENの最大値を超える場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 */
	template<typename T,
		// SFINAE: 要素のサイズが1バイト、かつメモリコピー可能なトリビアル型のみ許可
		std::enable_if_t<sizeof(T) == 1 && std::is_trivial_v<T>, int> = 0
	>
	void bindBlobParam(int paramIndex, const std::vector<std::optional<std::vector<T>>>& opt_vals) {
		if (opt_vals.size() == 1) {	// 要素数が 1 の場合は、単一行版に委譲
			bindBlobParam(paramIndex, opt_vals[0]);
			return;
		}

		checkBatchSize(opt_vals.size());
		// 1. 全てのBLOBを格納できる最大の長さを特定
		size_t max_len = 0;
		for (const auto& opt_val : opt_vals) {
			if (opt_val.has_value() && opt_val.value().size() > max_len) {
				max_len = opt_val.value().size();
			}
		}
		if (max_len == 0) { max_len = 1; }

		// SQLLEN (符号付き) で表現できる最大値を超えていないかチェック
		if (max_len > static_cast<size_t>(std::numeric_limits<SQLLEN>::max())) {
			throw std::out_of_range("BLOB size exceeds the maximum capacity of SQLLEN.");
		}

		// 2. 連続したメモリブロックを確保
		std::vector<std::uint8_t> flat_buffer(opt_vals.size() * max_len, 0);
		std::vector<SQLLEN> indicators(opt_vals.size());

		// 3. 各データをメモリブロックの正しい位置にコピーする
		for (size_t i = 0; i < opt_vals.size(); i++) {
			if (opt_vals[i].has_value()) {
				if (!opt_vals[i].value().empty()) {
					memcpy(flat_buffer.data() + (i * max_len), opt_vals[i].value().data(), opt_vals[i].value().size());
				}
				indicators[i] = static_cast<SQLLEN>(opt_vals[i].value().size());
			} else {
				indicators[i] = SQL_NULL_DATA;
			}
		}

		// 4. 構築した連続メモリブロックとストライドを保存
		bindBatchCore(
			paramIndex,
			std::move(flat_buffer),
			std::move(indicators),
			ParamMetadata{SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, max_len, 0, static_cast<SQLLEN>(max_len)}
		);
	}

	/**
	 * @brief ODBCドライバへのパラメータバインドを解除し、パラメータの値を保持する内部コンテナをクリアします。
	 * @throw OdbcException ODBCドライバからエラーが返された場合。
	 */
	void clearParams();

	/**
	 * @brief 準備・バインドされたステートメントを実行します。
	 * @details set_paramset_sizeが呼び出されている場合は一括実行、そうでなければ単一行実行を行います。
	 * @return 影響を受けた行数。
	 * @throw OdbcException ステートメントの実行に失敗した場合。
	 */
	int executePrepared();

	/**
	 * @brief 一括実行で実際に処理された行数を取得します。
	 */
	SQLULEN getParamsProcessed() const noexcept {
		return *params_processed;
	}

	/**
	 * @brief 一括実行における各行の実行ステータス配列を取得します。
	 * @details 返される配列の要素数は paramset_size と同じです。
	 * 各要素は SQL_PARAM_SUCCESS, SQL_PARAM_ERROR などの値を持ちます。
	 */
	const std::vector<SQLUSMALLINT>& getParamStatuses() const noexcept {
		return param_status_array;
	}

	/**
	 * @brief 直前のINSERT/UPDATE/DELETEで影響を受けた行数を取得します。
	 * @return 影響を受けた行数。
	 * @throw OdbcException ODBCドライバからエラーが返された場合。
	 */
	int getAffectedRowCount();

private:
	// === 型定義 ===

	/**
	 * @brief パラメータの実体を保持するための抽象基底クラス。
	 */
	struct IParamBuffer {
		virtual ~IParamBuffer() = default;
		// 実行時にデータポインタを取得するための純粋仮想関数
		virtual SQLPOINTER getPointer() = 0;

	protected:
		// すべての ParamBuffer<T> で共有される唯一のダミーアドレスを返す
		static SQLPOINTER getDummy() {
			static char dummy = 0;
			return reinterpret_cast<SQLPOINTER>(&dummy);
		}
	};

	/**
	 * @brief 任意の型 T の値を保持し、ODBCドライバに渡すためのポインタを提供するクラス。
	 * @tparam T バインドする値の型。std::optional と std::vector の2種類を想定しています。
	 */
	template <typename T>
	struct ParamBuffer : public IParamBuffer {
		T data;

		explicit ParamBuffer(T val) : data(std::move(val)) {}

		SQLPOINTER getPointer() override {
			// 保持している型 T の構造に応じて、適切なポインタの先頭アドレスを返す
			if constexpr (is_optional<T>::value) {
				if (!data.has_value()) { return nullptr; }

				using InnerType = typename T::value_type;

				if constexpr (std::is_same_v<InnerType, std::string>) {
					// ※現在の実装では std::vector<char> 等に移し替えているためここを通ることはありませんが、将来の拡張用に残しています。
					return reinterpret_cast<SQLPOINTER>(data.value().data());
				} else if constexpr (is_vector<InnerType>::value) {
					return (data.value().empty()) ? (getDummy()) : (reinterpret_cast<SQLPOINTER>(data.value().data()));
				} else {
					return reinterpret_cast<SQLPOINTER>(&data.value());
				}
			} else if constexpr (is_vector<T>::value) {
				return (data.empty()) ? (getDummy()) : (reinterpret_cast<SQLPOINTER>(data.data()));
			} else {
				// 実行時エラーではなく、コンパイル時に確実に気づけるようにする
				static_assert(!std::is_same_v<T, T>, "Unsupported type in ParamBuffer. Must be std::optional or std::vector.");
				return nullptr;
			}
		}
	};

	// ■ カスタムデリータ ■
	struct StmtDeleter {
		void operator()(SQLHSTMT h) const noexcept {
			if (h != SQL_NULL_HSTMT) { SQLFreeHandle(SQL_HANDLE_STMT, h); }
		}
	};

	// === プライベートメンバ変数 ===

	// ■ スマートポインタ ■
	/// @brief ODBCステートメントハンドル。
	std::unique_ptr<void, StmtDeleter> hstmt;

	// 列名(小文字)とインデックス(1始まり)を紐づけるキャッシュ
	std::unordered_map<std::string, int> column_map_cache;
	bool column_map_initialized = false;

	// --- プリペアドステートメント関連 ---

	/// @brief (単一・一括共通) 事前計算されたメタデータを保持するコンテナ
	std::map<int, ParamMetadata> param_metadata;
	/// @brief (単一・一括共通) 実行フェーズまでポインタ寿命を維持するためのデータバッファ
	std::map<int, std::unique_ptr<IParamBuffer>> param_buffers;

	/// @brief (単一行用) バインドされた文字列やBLOBの長さを保持するためのコンテナ。
	std::map<int, SQLLEN> param_indicators;

	/// @brief (一括実行用) 一括実行で処理する行数。
	size_t paramset_size = 1;
	/// @brief (一括実行用) バインドされた可変長データ(文字列など)の長さの配列を保持するバッファ。
	std::map<int, std::vector<SQLLEN>> bulk_param_indicators;
	/// @brief (一括実行用) 各行の実行ステータスを受け取るバッファ。
	std::vector<SQLUSMALLINT> param_status_array;
	/// @brief (一括実行用) 実際に処理された行数を受け取る変数。
	std::unique_ptr<SQLULEN> params_processed = std::make_unique<SQLULEN>(0);

	// === プライベートメンバ関数 ===

	/**
	 * @brief ODBCステートメントハンドル(HSTMT)を取得します。
	 * @details インライン関数にしたいのでヘッダーファイル内で定義しています。
	 * @return ODBCステートメントハンドル。
	 */
	SQLHSTMT getStmtHandle() const {
		return static_cast<SQLHSTMT>(hstmt.get());
	}

	/**
	 * @brief 列名とインデックスのマッピングを初期化します。
	 * @details 列名を小文字に変換してキャッシュすることで、データベースの違い（大文字小文字の揺れ）を吸収します。
	 */
	void initColumnMap();

	/**
	 * @brief ODBCドライバへのパラメータバインドを解除します。
	 * @return SQL_SUCCESS, SQL_SUCCESS_WITH_INFO, SQL_ERROR, SQL_INVALID_HANDLE のいずれか。
	 */
	SQLRETURN resetParams() noexcept;

	// --- 安全確保用ヘルパー ---

	/**
	 * @brief パラメータインデックスがODBCの1-based indexingに従っていることを確認します。もし0以下であれば例外を投げます。
	 * @param[in] index チェックするパラメータインデックス。
	 * @throw std::out_of_range indexが1未満の場合。
	 */
	void checkParamIndex(int index) const {
		if (index < 1) {
			throw std::out_of_range("Parameter index must be 1 or greater. (ODBC uses 1-based indexing)");
		}
	}

	/**
	 * @brief 単一行モードであることを確認します。もし一括モードであれば例外を投げます。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	void checkSingleRowMode() const {
		if (paramset_size > 1) {
			throw std::logic_error("Called single-row bindParam in batch mode.");
		}
	}

	/**
	 * @brief 一括モードであることを確認します。もし単一行モードであれば例外を投げます。
	 * @throw std::logic_error 入力サイズが paramset_size と一致しない場合。
	 */
	void checkBatchSize(size_t input_size) const {
		if (paramset_size != input_size) {
			throw std::logic_error(utils::concat("Parameter array size (", std::to_string(input_size), ") does not match paramset_size (", std::to_string(paramset_size), ")."));
		}
	}

	// --- コア登録ヘルパー (hppのprivate等に定義) ---

	/**
	 * @brief (Helper) 単一行用のバインドパラメータを内部バッファに登録します。
	 * @tparam BufferType バインドする値を受け取るバッファの型。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] buffer_val バインドする値を受け取るバッファ。
	 * @param[in] indicator バインドする値の長さやNULLを示すインジケータ。
	 * @param[in] meta バインドするパラメータのメタデータ。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 配列のサイズが バッチサイズ と一致しない場合。
	 */
	template <typename BufferType>
	void bindSingleCore(int paramIndex, BufferType&& buffer_val, SQLLEN indicator, const ParamMetadata& meta) {
		checkParamIndex(paramIndex);
		checkSingleRowMode();
		param_indicators.insert_or_assign(paramIndex, indicator);
		param_metadata.insert_or_assign(paramIndex, meta);

		// std::decay_t で参照等を外し、値として保存させる
		using DecayedT = std::decay_t<BufferType>;
		param_buffers.insert_or_assign(paramIndex, std::make_unique<ParamBuffer<DecayedT>>(std::forward<BufferType>(buffer_val)));
	}

	/**
	 * @brief (Helper) 一括実行用のバインドパラメータを内部バッファに登録します。
	 * @tparam BufferType バインドする値を受け取るバッファの型。
	 * @param[in] paramIndex バインドするパラメータの位置（1から始まる）。
	 * @param[in] buffer_val バインドする値を受け取るバッファ。
	 * @param[in] indicators バインドする値の長さやNULLを示すインジケータのベクター。
	 * @param[in] meta バインドするパラメータのメタデータ。
	 * @throw std::out_of_range paramIndexが1未満の場合。
	 * @throw std::logic_error 単一行モードで呼び出された場合。
	 */
	template <typename BufferType>
	void bindBatchCore(int paramIndex, BufferType&& buffer_val, std::vector<SQLLEN> indicators, const ParamMetadata& meta) {
		checkParamIndex(paramIndex);
		checkBatchSize(indicators.size());
		bulk_param_indicators.insert_or_assign(paramIndex, std::move(indicators));
		param_metadata.insert_or_assign(paramIndex, meta);

		// std::decay_t で参照等を外し、値として保存させる
		using DecayedT = std::decay_t<BufferType>;
		param_buffers.insert_or_assign(paramIndex, std::make_unique<ParamBuffer<DecayedT>>(std::forward<BufferType>(buffer_val)));
	}

	// --- 文字列フラット化ヘルパー (C++17 if constexprを使用) ---

	/**
	 * @brief (Helper) 文字列の配列をODBCドライバに渡すための固定長フラットバッファとインジケータのペアを構築します。
	 * @details 文字列の最大長を特定し、全ての文字列をゼロパディングされた固定長バッファにコピーします。
	 * さらに、各文字列の長さを示すインジケータのベクターも構築します。
	 * @tparam T 文字列の型。std::string と std::optional<std::string> のみを許可します。
	 * @param[in] vals バインドする文字列のvector。NULLをバインドしたければ、std::optional<std::string> の場合は nullopt を、std::string の場合は空文字列を使用してください。
	 * @return フラットバッファ、インジケータのベクター、そして各文字列の最大長を含むタプル。
	 * @throw std::out_of_range 文字列長がODBCパラメータの最大許容サイズを超える場合。
	 */
	template <typename T,
		// SFINAE: Tが std::string か std::optional<std::string> の場合のみ許可
		std::enable_if_t<std::is_same_v<T, std::string> || std::is_same_v<T, std::optional<std::string>>, int> = 0
	>
	auto buildStringFlatBuffer(const std::vector<T>& vals) {
		// 1. 全ての文字列を格納できる最大の長さを特定（+1は終端文字用）
		size_t max_len = 0;
		for (const auto& v : vals) {
			size_t len = 0;
			if constexpr (std::is_same_v<T, std::optional<std::string>>) {
				if (v.has_value()) { len = v.value().length(); }
			} else {
				len = v.length();
			}
			if (len > max_len) { max_len = len; }
		}
		max_len += 1;	// 終端文字('\0')用

		// SQLLEN (符号付き) で表現できる最大値を超えていないかチェック
		if (max_len > static_cast<size_t>(std::numeric_limits<SQLLEN>::max())) {
			throw std::out_of_range("String length exceeds maximum allowed size for ODBC parameters");
		}

		// 2. 連続したメモリブロックを確保
		std::vector<char> flat_buffer(vals.size() * max_len, 0);
		std::vector<SQLLEN> indicators(vals.size());

		// 3. 各文字列をメモリブロックの正しい位置にコピーする
		for (size_t i = 0; i < vals.size(); i++) {
			if constexpr (std::is_same_v<T, std::optional<std::string>>) {
				if (vals[i].has_value()) {
					memcpy(flat_buffer.data() + (i * max_len), vals[i].value().c_str(), vals[i].value().length());
					indicators[i] = static_cast<SQLLEN>(vals[i].value().length());
				} else {
					indicators[i] = SQL_NULL_DATA;
				}
			} else {
				memcpy(flat_buffer.data() + (i * max_len), vals[i].c_str(), vals[i].length());
				indicators[i] = static_cast<SQLLEN>(vals[i].length());
			}
		}
		// 4. 構築した連続メモリブロック、各部屋の大きさ（ストライド）、最大長をまとめて返す
		return std::make_tuple(std::move(flat_buffer), std::move(indicators), max_len);
	}
};

//============================================
// セッション管理クラス
//============================================

/**
 * @struct OdbcSession
 * @brief データベース接続と、任意のキーに関連付けられた複数のステートメントを安全に一元管理する高レベルクラス。
 * @details このクラスのunique_ptrを一つ保持して利用するのが基本的な使い方です。
 * RAIIにより、このクラスのオブジェクトが破棄される際に、全てのステートメントが破棄された後で
 * 接続が安全に破棄されることを保証します。
 * @tparam Key ステートメントを識別するキーの型 (デフォルト: std::string)。
 * @tparam Compare キーの比較を行う関数オブジェクト (デフォルト: std::less<Key>)。
 */
template <typename Key = std::string, typename Compare = std::less<Key>>
struct OdbcSession {
	// --- 型定義 ---
	using map_type					= std::map<Key, std::unique_ptr<OdbcStatement>, Compare>;
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
	OdbcSession(std::string dsn, std::string user, std::string pass)
		: connection(std::make_unique<OdbcConnection>(std::move(dsn), std::move(user), std::move(pass)))
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
	 * @throw std::runtime_error 接続がnullの場合。
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
