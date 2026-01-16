/**
 * @file includes.hpp
 * @brief 複数のファイルで共通して使用する型をまとめたヘッダファイル
 */
#pragma once

#include <cstdint>
#include <optional>
#include <SimpleGtk>
#include "./parameters.hpp"
#include "./math_utils.hpp"

//typedef int sample;

/**
 * @brief DBからの文字列を特定の型に変換するテンプレート関数の宣言
 * @tparam T 変換先の型
 * @param[in] str 変換元の文字列
 * @return 変換に成功した場合はT型の値を含むoptional、失敗した場合はnullopt
 */
template<typename T>
std::optional<T> from_db_string(const std::string& str);

/**
 * @struct MstUserTable
 * @brief ユーザー情報を保持するテーブルのデータ構造
 */
struct MstUserTable {
	/// @brief ユーザーID (連番、ユニークID)
	std::int32_t user_id{};
	/// @brief ユーザー名
	std::string user_name{};
	/// @brief 取り出し権限 (常に true)
	bool per_out{true};
	/// @brief 補充権限
	bool per_add{};
	/// @brief 棚卸し権限
	bool per_inventory{};
};

/**
 * @struct MstToolTable
 * @brief 工具の基本情報を保持するテーブルのデータ構造
 */
struct MstToolTable {
	/// @brief 基本工具ID (工具種別ごとの連番)
	std::int32_t basic_tool_id{};
	/// @brief 型番 (工具の型番)
	std::string tool_name{};
	/// @brief メーカー (工具の製造メーカー)
	std::string maker{};
	/// @brief 工具分類 (工具の分類)
	std::string tool_category{};
	/// @brief 材質 (工具の材質)
	std::string tool_material{};
	/// @brief 在庫数 (現在工具棚に保管されている個数)
	std::optional<std::int32_t> stc{};
	/// @brief 凍結フラグ (true: 凍結、false: 通常)
	bool is_frozen{};
};

/**
 * @struct MstAddressTable
 * @brief 画面表示住所とPLC制御住所を紐づける辞書テーブルのデータ構造
 */
struct MstAddressTable {
	/// @brief 画面表示住所
	std::string display_address{};
	/// @brief PLC制御住所
	std::string control_address{};
};

/**
 * @struct StrageAreaTable
 * @brief 工具保管領域の情報を保持するテーブルのデータ構造
 */
struct StrageAreaTable {
	/// @brief 保管領域ID (工具保管領域識別用連番)
	std::int32_t storage_area_id{};
	/// @brief 基本工具ID (工具種別ごとの連番)
	std::int32_t basic_tool_id{};
	/// @brief 保管ケース個数 (工具ケースの保管個数)
	std::optional<std::int32_t> toolcase_stc_num{};
	/// @brief 画面表示住所
	std::string display_address{};
	/// @brief 最終棚卸し日時
	std::optional<std::chrono::system_clock::time_point> last_reconciliation_time{};
};

/**
 * @enum StorageCondition
 * @brief 工具の状態を示す列挙型
 */
enum class StorageCondition {
	/// @brief 補充前
	NOT_STORED,
	/// @brief 保管中 (使用可能)
	STORED,
	/// @brief 取出
	TAKEN_OUT
};

inline constexpr const char* StorageCondition_NOT_STORED_STR	= "補充前";	// NOT_STORED
inline constexpr const char* StorageCondition_STORED_STR		= "保管中";	// STORED
inline constexpr const char* StorageCondition_TAKEN_OUT_STR		= "取出";	// TAKEN_OUT

/**
 * @brief StorageCondition 列挙型をDB保存用の文字列に変換するヘルパー関数
 * @param[in] condition 変換元の StorageCondition 列挙型の値
 * @return 変換後の文字列
 */
inline std::string to_db_string(StorageCondition condition) {
	switch (condition) {
		case StorageCondition::NOT_STORED:	return StorageCondition_NOT_STORED_STR;
		case StorageCondition::STORED:		return StorageCondition_STORED_STR;
		case StorageCondition::TAKEN_OUT:	return StorageCondition_TAKEN_OUT_STR;
	}
}

/**
 * @brief DB保存用の文字列を StorageCondition 列挙型に変換するヘルパー関数
 * @param[in] str 変換元の文字列
 * @return 変換後の StorageCondition 列挙型の値を含むoptional、変換に失敗した場合はnullopt
 */
template<>
inline std::optional<StorageCondition> from_db_string<StorageCondition>(const std::string& str) {
	if (str == StorageCondition_NOT_STORED_STR)	{ return StorageCondition::NOT_STORED; }
	if (str == StorageCondition_STORED_STR)		{ return StorageCondition::STORED; }
	if (str == StorageCondition_TAKEN_OUT_STR)	{ return StorageCondition::TAKEN_OUT; }
	return std::nullopt;
}

/**
 * @struct UniqueToolTable
 * @brief 工具の個別情報を保持するテーブルのデータ構造
 */
struct UniqueToolTable {
	/// @brief 個別工具ID (個別工具識別用)
	std::int64_t unique_tool_id{};
	/// @brief 基本工具ID (工具種別識別用連番)
	std::int32_t basic_tool_id{};
	/// @brief 個別番号 (個別の連番)
	std::int32_t unique_num{};
	/// @brief ケース内個数
	std::int32_t case_pack_num{};
	/// @brief 保管領域ID (保管領域識別用連番)
	std::optional<std::int32_t> storage_area_id{};
	/// @brief 保管状態 (補充前、保管中、取出)
	StorageCondition storage_condition{StorageCondition::NOT_STORED};
	/// @brief 再研磨回数 (工具の再研磨回数)
	std::optional<std::int32_t> regrind_count{};
};

/**
 * @struct MstLineTable
 * @brief プログラム割当一覧表のライン情報を保持するテーブルのデータ構造
 */
struct MstLineTable {
	/// @brief ラインID
	std::int32_t line_id{};
	/// @brief ライン名
	std::string line_name{};
};

/**
 * @struct MstProgramTable
 * @brief プログラム割当一覧表のプログラム情報を保持するテーブルのデータ構造
 */
struct MstProgramTable {
	/// @brief プログラム割り当てID (ライン、プログラムごとの連番)
	std::int32_t program_id{};
	/// @brief ラインID
	std::int32_t line_id{};
	/// @brief プログラム番号
	std::string program_name{};
};

/**
 * @struct MstToolAssignmentTable
 * @brief ツーリングリストの工具割当情報を保持するテーブルのデータ構造
 */
struct MstToolAssignmentTable {
	/// @brief 工具割当ID (工具割当識別用ID)
	std::int32_t tool_assignment_id{};
	/// @brief プログラム割り当てID (ライン、プログラムごとの連番)
	std::int32_t program_id{};
	/// @brief ツール番号
	std::string tool_num{};
	/// @brief 基本工具ID (工具種別識別用連番)
	std::int32_t basic_tool_id{};
};

/**
 * @enum OperationClass
 * @brief 操作の分類を示す列挙型
 */
enum class OperationClass {
	/// @brief 取り出し
	OUT,
	/// @brief 補充
	ADD,
	/// @brief 棚卸し
	INVENTORY
};

inline constexpr const char* OperationClass_OUT_STR			= "取出";	// OUT
inline constexpr const char* OperationClass_ADD_STR			= "補充";	// ADD
inline constexpr const char* OperationClass_INVENTORY_STR	= "棚卸";	// INVENTORY

/**
 * @brief OperationClass 列挙型をDB保存用の文字列に変換するヘルパー関数
 * @param[in] op_class 変換元の OperationClass 列挙型の値
 * @return 変換後の文字列
 */
inline std::string to_db_string(OperationClass op_class) {
	switch (op_class) {
		case OperationClass::OUT:		return OperationClass_OUT_STR;
		case OperationClass::ADD:		return OperationClass_ADD_STR;
		case OperationClass::INVENTORY:	return OperationClass_INVENTORY_STR;
	}
	return "";
}

/**
 * @brief DB保存用の文字列を OperationClass 列挙型に変換するヘルパー関数
 * @param[in] str 変換元の文字列
 * @return 変換後の OperationClass 列挙型の値を含むoptional、変換に失敗した場合はnullopt
 */
template<>
inline std::optional<OperationClass> from_db_string<OperationClass>(const std::string& str) {
	if (str == OperationClass_OUT_STR)			{ return OperationClass::OUT; }
	if (str == OperationClass_ADD_STR)			{ return OperationClass::ADD; }
	if (str == OperationClass_INVENTORY_STR)	{ return OperationClass::INVENTORY; }
	return std::nullopt;
}

/**
 * @enum TrackResult
 * @brief 追跡システムの結果を示す列挙型
 */
enum class TrackResult {
	/// @brief なし (正常)
	NORMAL,
	/// @brief あり (警告無視)
	OVERRIDDEN
};

inline constexpr const char* TrackResult_NORMAL_STR		= "なし";	// NORMAL
inline constexpr const char* TrackResult_OVERRIDDEN_STR	= "あり";	// OVERRIDDEN

/**
 * @brief TrackResult 列挙型をDB保存用の文字列に変換するヘルパー関数
 * @param[in] result 変換元の TrackResult 列挙型の値
 * @return 変換後の文字列
 */
inline std::string to_db_string(TrackResult result) {
	switch (result) {
		case TrackResult::NORMAL:		return TrackResult_NORMAL_STR;
		case TrackResult::OVERRIDDEN:	return TrackResult_OVERRIDDEN_STR;
	}
	return "";
}

/**
 * @brief DB保存用の文字列を TrackResult 列挙型に変換するヘルパー関数
 * @param[in] str 変換元の文字列
 * @return 変換後の TrackResult 列挙型の値を含むoptional、変換に失敗した場合はnullopt
 */
template<>
inline std::optional<TrackResult> from_db_string<TrackResult>(const std::string& str) {
	if (str == TrackResult_NORMAL_STR)		{ return TrackResult::NORMAL; }
	if (str == TrackResult_OVERRIDDEN_STR)	{ return TrackResult::OVERRIDDEN; }
	return std::nullopt;
}

/**
 * @struct RecOperationTable
 * @brief 操作履歴を保持するテーブルのデータ構造
 */
struct RecOperationTable {
	/// @brief 履歴ID (記録用連番)
	std::int32_t operation_id{};
	/// @brief 履歴分類 (履歴分類名称)
	OperationClass operation_class{OperationClass::OUT};
	/// @brief ユーザーID (ユーザー識別用連番)
	std::int32_t user_id{};
	/// @brief 処理時刻 (処理日時)
	std::chrono::system_clock::time_point proc_time{};
	/// @brief 保管領域ID (工具保管領域識別用連番)
	std::int32_t storage_area_id{};
	/// @brief 個別工具ID (工具識別用連番)
	std::optional<std::int64_t> unique_tool_id{};
	/// @brief 動画パス (動画の保存先リンク)
	std::string video_path{};
	/// @brief 操作個数 (補充、取出の際の個数)
	std::optional<std::int32_t> tool_num{};
	/// @brief ラインID (ライン識別用連番)
	std::optional<std::int32_t> line_id{};
	/// @brief 工具追跡の結果
	std::optional<TrackResult> track_result{};
};

/**
 * @struct MstUserInfo
 * @brief ユーザー情報を保持する構造体
 */
struct MstUserInfo {
	/// @brief ユーザーID (連番、ユニークID)
	std::int32_t id{};
	/// @brief ユーザー名
	std::string name{};
	/// @brief 取り出し権限 (常に true)
	bool per_out{true};
	/// @brief 補充権限
	bool per_add{};
	/// @brief 棚卸し権限
	bool per_inventory{};
};

/**
 * @struct MstToolInfo
 * @brief 工具の基本情報を保持する構造体
 */
struct MstToolInfo {
	/// @brief 基本工具ID (工具種別ごとの連番)
	std::int32_t id{};
	/// @brief 型番 (工具の型番)
	std::string name{};
	/// @brief メーカー (工具の製造メーカー)
	std::string maker{};
	/// @brief 工具分類 (工具の分類)
	std::string category{};
	/// @brief 材質 (工具の材質)
	std::string material{};
	/// @brief 在庫数 (現在工具棚に保管されている個数)
	std::optional<std::int32_t> stc{};
	/// @brief 凍結フラグ (true: 凍結、false: 通常)
	bool is_frozen{};
};

/**
 * @struct MstAddressInfo
 * @brief 画面表示住所とPLC制御住所の情報を保持する構造体
 */
using MstAddressInfo = MstAddressTable;

/**
 * @struct StrageAreaInfo
 * @brief 工具保管領域の情報を保持する構造体
 */
struct StrageAreaInfo {
	/// @brief 保管領域ID (工具保管領域識別用連番)
	std::int32_t id{};
	/// @brief 保管している工具の基本情報
	MstToolInfo item{};
	/// @brief 保管ケース個数 (工具ケースの保管個数)
	std::optional<std::int32_t> toolcase_stc_num{};
	/// @brief 住所 (画面表示住所とPLC制御住所)
	MstAddressInfo mst_address_info{};
	/// @brief 最終棚卸し日時
	std::optional<std::chrono::system_clock::time_point> last_reconciliation_time{};
};

/**
 * @struct UniqueToolInfo
 * @brief 工具の個別情報を保持する構造体
 */
struct UniqueToolInfo {
	/// @brief 個別工具ID (個別工具識別用)
	std::int64_t id{};
	/// @brief 工具の基本情報
	MstToolInfo mst_tool_info{};
	/// @brief 個別番号 (個別の連番)
	std::int32_t unique_num{};
	/// @brief ケース内個数
	std::int32_t case_pack_num{};
	/// @brief 保管領域の情報
	std::optional<StrageAreaInfo> storage_area_info{};
	/// @brief 保管状態 (補充前、保管中、取出)
	StorageCondition storage_condition{StorageCondition::NOT_STORED};
	/// @brief 再研磨回数 (工具の再研磨回数)
	std::optional<std::int32_t> regrind_count{};
};

/**
 * @struct MstLineInfo
 * @brief プログラム割当一覧表のライン情報を保持する構造体
 */
struct MstLineInfo {
	/// @brief ラインID
	std::int32_t id{};
	/// @brief ライン名
	std::string name{};
};

/**
 * @struct MstLineInfo
 * @brief プログラム割当一覧表のプログラム情報を保持する構造体
 */
struct MstProgramInfo {
	/// @brief プログラム割り当てID (ライン、プログラムごとの連番)
	std::int32_t id{};
	/// @brief プログラム番号
	std::string name{};
};

/**
 * @struct ToolAssignmentInfo
 * @brief ツーリングリスト内の工具一行分の情報を保持する構造体
 */
struct ToolAssignmentInfo {
	/// @brief 工具割当ID (工具割当識別用ID)
	std::int32_t id{};
	/// @brief ツール番号
	std::string tool_num{};
	/// @brief 工具基本情報
	MstToolInfo mst_tool_info{};
};

/**
 * @struct ToolingListInfo
 * @brief ツーリングリスト全体の情報を保持する構造体
 */
struct ToolingListInfo {
	/// @brief ライン情報
	MstLineInfo line{};
	/// @brief プログラム情報
	MstProgramInfo program{};
	/// @brief ツーリングリストに含まれる工具のリスト
	std::vector<ToolAssignmentInfo> tool_assignments{};
};

// --- 個別工具ID (UniqueToolID) 関連 ---

/// @brief 個別工具IDの桁数 (10桁)
inline constexpr int UNIQUE_TOOL_ID_DIGITS = 10;

/// @brief 個別工具ID内の「個別番号」部分の桁数 (5桁)
inline constexpr int UNIQUE_TOOL_ID_UNIQUE_DIGITS = 5;

/// @brief 個別工具IDから基本工具IDと個別番号を分離するための除数 (10の UNIQUE_TOOL_ID_UNIQUE_DIGITS 乗)
inline constexpr std::int64_t UNIQUE_TOOL_ID_DIVISOR = utils::constexpr_pow(static_cast<std::int64_t>(10), static_cast<std::size_t>(UNIQUE_TOOL_ID_UNIQUE_DIGITS));

/// @brief 有効な個別工具IDの最小値 (10^9)
inline constexpr std::int64_t MIN_UNIQUE_TOOL_ID = 0;

/// @brief 有効な個別工具IDの最大値 (10^10 - 1)
inline constexpr std::int64_t MAX_UNIQUE_TOOL_ID = utils::constexpr_pow(static_cast<std::int64_t>(10), static_cast<std::size_t>(10)) - 1;

/**
 * @brief 10桁の個別工具IDを「基本工具ID」と「個別番号」に分割します。
 * @details 桁数が10桁でないID (MIN/MAXの範囲外) の場合は nullopt を返します。
 * @param unique_tool_id 10桁の個別工具ID (例: 0000100001)
 * @return std::pair<基本工具ID, 個別番号> を含む optional (first: 上位5桁, second: 下位5桁)。
 * 桁数不正時は std::nullopt。
 */
inline std::optional<std::pair<std::int32_t, std::int32_t>>
split_unique_tool_id(std::int64_t unique_tool_id) {
	if (unique_tool_id < MIN_UNIQUE_TOOL_ID || unique_tool_id > MAX_UNIQUE_TOOL_ID) {
		return std::nullopt;
	}
	auto basic_id = static_cast<std::int32_t>(unique_tool_id / UNIQUE_TOOL_ID_DIVISOR);
	auto unique_num = static_cast<std::int32_t>(unique_tool_id % UNIQUE_TOOL_ID_DIVISOR);
	return std::make_pair(basic_id, unique_num);
}
