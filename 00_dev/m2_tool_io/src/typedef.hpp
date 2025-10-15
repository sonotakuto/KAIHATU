/**
 * @file includes.hpp
 * @brief 複数のファイルで共通して使用する型をまとめたヘッダファイル
 */
#pragma once

#include <SimpleGtk>
#include "./parameters.hpp"

//typedef int sample;

/**
 * @struct MstUserTable
 * @brief ユーザー情報を保持するテーブルのデータ構造
 */
struct MstUserTable {
	/// @brief ユーザーID (連番、ユニークID)
	int user_id{};
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
	int basic_tool_id{};
	/// @brief 型番 (工具の型番)
	std::string tool_name{};
	/// @brief メーカー (工具の製造メーカー)
	std::string maker{};
	/// @brief 工具分類 (工具の分類)
	std::string tool_category{};
	/// @brief 材質 (工具の材質)
	std::string tool_material{};
	/// @brief 在庫数 (現在工具棚に保管されている個数)
	int stc{};
};

/**
 * @enum StorageCondition
 * @brief 工具の状態を示す列挙型
 */
enum class StorageCondition {
	/// @brief 未補充
	NOT_STORED,
	/// @brief 使用可能
	USABLE,
	/// @brief 取り出された
	TAKEN_OUT
};

/**
 * @struct UniqueToolTable
 * @brief 工具の個別情報を保持するテーブルのデータ構造
 */
struct UniqueToolTable {
	/// @brief 個別工具ID (個別工具識別用)
	int unique_tool_id{};
	/// @brief 基本工具ID (工具種別識別用連番)
	int basic_tool_id{};
	/// @brief 個別番号 (個別の連番)
	int unique_num{};
	/// @brief ケース内個数
	int case_pack_num{};
	/// @brief 保管領域ID (保管領域識別用連番)
	int storage_area_id{};
	/// @brief 保管状態 (補充前、保管中、取出)
	StorageCondition storage_condition{StorageCondition::NOT_STORED};
	/// @brief 再研磨回数 (工具の再研磨回数)
	std::optional<int> regrind_count{};
};

/**
 * @struct ToolAssignmentTable
 * @brief ツーリングリストの工具割当情報を保持するテーブルのデータ構造
 */
struct ToolAssignmentTable {
	/// @brief 工具割当ID (工具割当識別用ID)
	int tool_assignment_id{};
	/// @brief プログラム割り当てID (ライン、プログラムごとの連番)
	int program_assignment_id{};
	/// @brief ツール番号
	std::string tool_number{};
	/// @brief 工具基本情報
	MstToolInfo mst_tool{};
};

/**
 * @struct ProgramAssignmentListTable
 * @brief プログラム割当一覧表の情報を保持するテーブルのデータ構造
 */
struct ProgramAssignmentListTable {
	/// @brief プログラム割り当てID (ライン、プログラムごとの連番)
	int program_assignment_id{};
	/// @brief ライン名
	std::string line_name{};
	/// @brief プログラム番号
	std::string program_num{};
};

/**
 * @struct MstAddressTable
 * @brief 画面表示住所とPLC制御住所を紐づける辞書テーブルのデータ構造
 */
struct MstAddressTable {
	/// @brief 画面表示住所
	std::string display_address{};
	/// @brief PLC制御住所
	int control_address{};
};

/**
 * @struct StrageAreaTable
 * @brief 工具保管領域の情報を保持するテーブルのデータ構造
 */
struct StrageAreaTable {
	/// @brief 保管領域ID (工具保管領域識別用連番)
	int storage_area_id{};
	/// @brief 基本工具ID (工具種別ごとの連番)
	int basic_tool_id{};
	/// @brief 保管ケース個数 (工具ケースの保管個数)
	int toolcase_stc_num{};
	/// @brief 画面表示住所
	std::string display_address{};
};

/**
 * @enum OperationClass
 * @brief 操作の分類を示す列挙型
 */
enum class OperationClass {
	OUT,
	ADD,
	INVENTORY
};

/**
 * @struct MstAddressTable
 * @brief 操作履歴を保持するテーブルのデータ構造
 */
struct RecOperationTable {
	/// @brief 履歴ID (記録用連番)
	int operation_id{};
	/// @brief 履歴分類 (履歴分類名称)
	OperationClass operation_class{OperationClass::OUT};
	/// @brief ユーザーID (ユーザー識別用連番)
	int user_id{};
	/// @brief 処理時刻 (処理日時)
	std::chrono::system_clock::time_point proc_time{};
	/// @brief 保管領域ID (工具保管領域識別用連番)
	int storage_area_id{};
	/// @brief 個別工具ID (工具識別用連番)
	int unique_tool_id{};
	/// @brief 動画パス (動画の保存先リンク)
	std::string video_path{};
};

/**
 * @struct MstUserInfo
 * @brief ユーザー情報を保持する構造体
 */
using MstUserInfo = MstUserTable;

/**
 * @struct MstToolInfo
 * @brief 工具の基本情報を保持する構造体
 */
using MstToolInfo = MstToolTable;

/**
 * @struct UniqueToolInfo
 * @brief 工具の個別情報を保持する構造体(インサート以外)
 */
struct UniqueToolInfo {
	/// @brief 個別工具ID (個別工具識別用)
	int unique_tool_id{};
	/// @brief 工具の基本情報
	MstToolInfo mst_tool{};
	/// @brief 個別番号 (個別の連番)
	int unique_num{};
	/// @brief ケース内個数
	int case_pack_num{};
	/// @brief 保管領域の情報
	std::optional<StrageAreaInfo> storage_area{};
	/// @brief 保管状態 (補充前、保管中、取出)
	StorageCondition storage_condition{StorageCondition::NOT_STORED};
	/// @brief 再研磨回数 (工具の再研磨回数)
	std::optional<int> regrind_count{};
};

/**
 * @struct ToolAssignmentInfo
 * @brief ツーリングリスト内の工具一行分の情報を保持する構造体
 */
struct ToolAssignmentInfo {
	/// @brief 工具割当ID (工具割当識別用ID)
	int tool_assignment_id{};
	/// @brief ツール番号
	std::string tool_number{};
	/// @brief 工具基本情報
	MstToolInfo mst_tool{};
};

/**
 * @struct ToolingListInfo
 * @brief ツーリングリスト全体の情報を保持する構造体
 */
struct ToolingListInfo {
	/// @brief プログラム割り当てID (ライン、プログラムごとの連番)
	int program_assignment_id{};
	/// @brief ライン名
	std::string line_name{};
	/// @brief プログラム番号
	std::string program_num{};
	/// @brief ツーリングリストに含まれる工具のリスト
	std::vector<ToolAssignmentInfo> tool_assignments{};
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
	int storage_area_id{};
	/// @brief 保管している工具の基本情報
	MstToolInfo item{};
	/// @brief 保管ケース個数 (工具ケースの保管個数)
	int toolcase_stc_num{};
	/// @brief 住所 (画面表示住所とPLC制御住所)
	MstAddressInfo mst_address{};
};

/**
 * @struct BoxInfo
 * @brief コンテナ情報を保持する構造体
 */
struct BoxInfo {
	/// @brief コンテナID (主キー)
	int box_id{};
	/// @brief 基本工具情報
	MstToolInfo item{};
};
