/**
 * @file db_client.hpp
 * @brief データベースにアクセスするための関数をまとめたヘッダファイル。
 */
#pragma once
#include <cstdint>
#include <SimpleGtk>
#include "./typedef.hpp"
#include "./parameters.hpp"

/**
 * @namespace db
 * @brief データベース接続に関連する関数と定数を格納する名前空間。
 */
namespace db {

constexpr int	RETRY_LIMIT	= 3;	// DB接続のリトライ回数
constexpr const char*	ODBC_DSN	= "system";		// ODBCのDSN名
constexpr const char*	ODBC_USER	= "pi";			// ODBCのユーザー名
constexpr const char*	ODBC_PASS	= "pipasswd";	// ODBCのパスワード

/**
 * @brief QRコードの種類を示すenum
 */
enum class QrCodeType {
	User,		// ユーザーID
	Tool,		// 個別工具ID
	Program,	// プログラム割当ID
	Unknown		// 不明
};

/**
 * @brief パースしたQRコードのデータを格納する構造体
 */
struct QrCodeData {
	QrCodeType type;
	std::int64_t id;
	std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief QRコードの文字列を解析し、QrCodeData構造体を返す
 * @param[in] qr_string QRコードから読み取った文字列
 * @return パースに成功した場合はQrCodeDataを含むstd::optional、失敗した場合はstd::nullopt
 */
std::optional<QrCodeData> parse_qr(const std::string& qr_string);

/**
 * @brief データベースへの接続を確立します。
 * @details
 * 内部でOdbcSessionを生成し、グローバルなdb_sessionポインタで管理します。
 * 接続に失敗した場合は、DB_RETRY_LIMITで定義された回数だけ再試行します。
 * @return 接続に成功した場合はtrue、全てのリトライに失敗した場合はfalse。
 */
bool connect(void);

/**
 * @brief データベースとの接続を切断します。
 * @details
 * グローバルなdb_sessionポインタをリセットし、OdbcSessionのデストラクタを呼び出すことで、
 * 安全に接続を解放します。
 */
void disconnect(void);

/**
 * @brief QRコード文字列を基にユーザー情報をデータベースから取得し、結果を g_logged_in_user_info に格納します。
 * @param[in] qr_string 取得対象のQRコード文字列。
 * @return ユーザー情報が見つかった場合は true、見つからなければ false。
 */
bool user_authenticate(const std::string& qr_string);

/**
 * @brief 操作履歴のレコードをデータベースに保存します。
 * @param[in] record 保存する履歴データを含む RecOperationTable 構造体。
 * @return 保存に成功した場合はtrue、失敗した場合はfalse。
 */
bool save_history(const RecOperationTable& record);

/**
 * @brief 複数の操作履歴のレコードをデータベースに保存します。
 * @details パフォーマンス向上のため、トランザクションとODBCのパラメータ配列機能を使用します。
 * @param[in] records 保存する履歴データのリスト。
 * @return 全てのレコードの保存に成功した場合はtrue、一件でも失敗した場合はfalse。
 */
bool save_history(const std::vector<RecOperationTable>& records);

// TODO: 再研磨機数を増やす関数を追加

/**
 * @brief QRコード文字列を基にユーザー情報をデータベースから取得します。
 * @param[in] qr_string 取得対象のQRコード文字列。
 * @return ユーザー情報が見つかった場合はその構造体、見つからなければ std::nullopt。
 */
std::optional<MstUserInfo> get_MstUserInfo(const std::string& qr_string);

/**
 * @brief QRコード文字列を基に、関連する全ての情報（基本情報、保管領域情報など）を結合して取得します。
 * @param[in] qr_string 取得対象のQRコード文字列。
 * @return 工具情報が見つかった場合はその構造体、見つからなければ std::nullopt。
 */
std::optional<UniqueToolInfo> get_UniqueToolInfo(const std::string& qr_string);

/**
 * @brief QRコード文字列を基に、ツーリングリスト全体の情報（それに含まれる全工具の情報を含む）を取得します。
 * @param[in] qr_string 取得対象のQRコード文字列。
 * @return ツーリングリストが見つかった場合はその構造体、見つからなければ std::nullopt。
 */
std::optional<ToolingListInfo> get_ToolingListInfo(const std::string& qr_string);

/**
 * @brief 既存の ToolingListInfo オブジェクトの内容をDBから再取得して更新します。
 * @param[in, out] list_info 更新対象の ToolingListInfo オブジェクト。program.id を使って検索されます。
 * @return 更新に成功した場合は true、失敗した場合は false。
 */
bool update_ToolingListInfo(ToolingListInfo& list_info);

/**
 * @brief 指定された基本工具IDを持つ工具が保管されている領域のうち、保管ケース数が最も少ない保管領域の情報を取得します。
 * @param[in] basic_tool_id 検索対象の基本工具ID。
 * @return 条件に合致する保管領域情報が見つかった場合はその構造体、見つからなければ std::nullopt。
 */
std::optional<StrageAreaInfo> get_StrageAreaInfo_with_min_cases(int basic_tool_id);

/**
 * @brief 指定された基本工具IDを持つ工具が保管されている領域のうち、保管ケース数が最も多い保管領域の情報を取得します。
 * @param[in] basic_tool_id 検索対象の基本工具ID。
 * @return 条件に合致する保管領域情報が見つかった場合はその構造体、見つからなければ std::nullopt。
 */
std::optional<StrageAreaInfo> get_StrageAreaInfo_with_max_cases(int basic_tool_id);

} // namespace db
