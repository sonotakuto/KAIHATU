/**
 * @file db_client.hpp
 * @brief データベースにアクセスするための関数をまとめたヘッダファイル。
 */
#pragma once
#include <cstdint>
#include <SimpleGtk>
#include "typedef.hpp"
#include "parameters.hpp"

/**
 * @namespace db
 * @brief データベース接続に関連する関数と定数を格納する名前空間。
 */
namespace db {

inline constexpr int			RETRY_LIMIT	= 3;			// DB接続のリトライ回数
inline constexpr const char*	ODBC_DSN	= "system";		// ODBCのDSN名

// 開発環境と本番環境でユーザー名とパスワードを分ける
#ifdef PRODUCTION
inline constexpr const char*	ODBC_USER	= "root";		// ODBCのユーザー名
inline constexpr const char*	ODBC_PASS	= "gr4";		// ODBCのパスワード
#else
inline constexpr const char*	ODBC_USER	= "root";			// ODBCのユーザー名
inline constexpr const char*	ODBC_PASS	= "gr4";	// ODBCのパスワード
#endif

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
 * @brief システムパスワードを認証します。
 * @details mst_password テーブル内のハッシュと照合します。
 * @param[in] input_password ユーザーが入力した平文のパスワード。
 * @return パスワードが一致すれば true、不一致またはエラーの場合は false。
 */
bool authenticate_with_password(const std::string& input_password);

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

/**
 * @brief 指定された工具リストの再研磨回数を1ずつ増やします。
 * @details regrind_count が NULL の場合は 0 とみなして 1 に設定します。
 * トランザクション内で一括処理されます。
 * @param[in] tool_ids 再研磨を行った工具のユニークIDリスト。
 * @return 全ての更新に成功した場合はtrue、失敗した場合はfalse。
 */
bool increment_regrind_counts(const std::set<std::int64_t>& tool_ids);

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

// ============================================================
// 棚卸し (RECONCILE) 機能用
// ============================================================

/**
 * @brief 棚卸し対象として選択可能な保管領域 (コンテナ) の一覧を取得します。
 * @details storage_area テーブルを mst_address, mst_tool と結合し、全件を取得します。
 * @return 保管領域情報のリスト (0件の場合は空のvector)。取得処理自体に失敗した場合は std::nullopt。
 */
std::optional<std::vector<StrageAreaInfo>> get_StrageAreaInfoList_for_reconcile();

/**
 * @brief 指定した保管領域に「現在、保管中」の個別工具一覧を取得します。
 * @details unique_tool テーブルを storage_area_id と storage_condition='保管中' で絞り込み、
 * mst_tool と結合して取得します。棚卸しの「期待リスト」の元データとなります。
 * @param[in] storage_area_id 対象の保管領域ID (StrageAreaInfo::id)。
 * @return 個別工具情報のリスト (0件の場合は空のvector)。取得処理自体に失敗した場合は std::nullopt。
 */
std::optional<std::vector<UniqueToolInfo>> get_UniqueToolInfoList_by_StrageArea(std::int32_t storage_area_id);

/**
 * @brief 指定した保管領域の最終棚卸し日時 (storage_area.last_reconciliation_time) を更新します。
 * @param[in] storage_area_id 対象の保管領域ID。
 * @param[in] time 記録する日時 (通常は sg::now() の結果)。
 * @return 更新に成功した場合は true、対象が存在しない場合やDBエラーの場合は false。
 */
bool update_last_reconciliation_time(std::int32_t storage_area_id, std::chrono::system_clock::time_point time);

/**
 * @brief 棚卸しで未読だった個別工具を、コンテナから無くなったものとして数字だけ更新します。
 * @details unique_tool.case_pack_num を 0 に、storage_area.toolcase_stc_num を1、
 * mst_tool.stc を case_pack_num 分だけ減らします。storage_condition 等は変更しません。
 * 全件を1トランザクションで処理し、case_pack_num が既に0の工具は何もしません(冪等)。
 * @param[in] unique_tool_ids 未読だった個別工具IDのリスト。
 * @return 全件成功なら true。
 */
bool apply_unread_reconcile(const std::vector<std::int64_t>& unique_tool_ids);

} // namespace db
