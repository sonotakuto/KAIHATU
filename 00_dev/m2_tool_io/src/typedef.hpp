/**
 * @file includes.hpp
 * @brief 複数のファイルで共通して使用する型をまとめたヘッダファイル
 */
#pragma once

#include <cstdint>
#include <SimpleGtk>
#include "./parameters.hpp"

//typedef int sample;

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
	std::int32_t stc{};
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
	std::int32_t toolcase_stc_num{};
	/// @brief 画面表示住所
	std::string display_address{};
};

/**
 * @enum StorageCondition
 * @brief 工具の状態を示す列挙型
 */
enum class StorageCondition {
	/// @brief 未補充
	NOT_STORED,
	/// @brief 使用可能 (保管中)
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
	OUT,
	ADD,
	INVENTORY
};

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
	std::optional<std::string> video_path{};
	/// @brief 操作個数 (補充、取出の際の個数)
	std::optional<std::int32_t> tool_num{};
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
	std::int32_t stc{};
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
	std::int32_t toolcase_stc_num{};
	/// @brief 住所 (画面表示住所とPLC制御住所)
	MstAddressInfo mst_address_info{};
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
