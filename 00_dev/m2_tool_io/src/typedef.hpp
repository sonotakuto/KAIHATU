/**
 * @file includes.hpp
 * @brief 複数のファイルで共通して使用する型をまとめたヘッダファイル
 */
#pragma once

#include <SimpleGtk>
#include "./parameters.hpp"

//typedef int sample;

/**
 * @struct UserInfo
 * @brief ユーザー情報を保持する構造体
 */
struct UserInfo {
	/// @brief ユーザーID
	int user_id{};
	/// @brief ユーザー名
	std::string user_name{};
	/// @brief 取り出し権限
	bool per_out{};
	/// @brief 補充権限
	bool per_add{};
	/// @brief 棚卸し権限
	bool per_reconcile{};
};

/**
 * @struct ToolBasicInfo
 * @brief 工具の基本情報を保持する構造体
 */
struct ToolBasicInfo {
	/// @brief 基本情報ID (主キー)
	int basic_tool_id{};
	/// @brief 工具分類
	std::string class_name{};
	/// @brief 型番
	std::string tool_name{};
	/// @brief メーカー名
	std::string maker{};
	/// @brief 材質
	std::string material{};
	/// @brief 在庫数
	int stc{};
};

/**
 * @enum ToolCondition
 * @brief 工具の状態を示す列挙型
 */
enum class ToolCondition {
	/// @brief 未補充
	NOT_STORED,
	/// @brief 使用可能
	USABLE,
	/// @brief 取り出された
	TAKEN_OUT
};

/**
 * @struct ToolIndividualInfo
 * @brief 工具の個別情報を保持する構造体(インサート以外)
 */
struct ToolIndividualInfo {
	/// @brief 工具ID
	int id{};
	/// @brief 基本情報
	ToolBasicInfo basic_info{};
	/// @brief 工具の状態
	ToolCondition condition{};
	/// @brief 再研磨回数
	std::optional<int> reground_count{};
	/// @brief ケース内インサート個数
	std::optional<int> insert_count{};
	/// @brief QRコード文字列
//	std::string qr_code{};
};

/**
 * @struct ToolingListItem
 * @brief ツーリングリスト内の工具一行分の情報を保持する構造体
 */
struct ToolingListItem {
	/// @brief ツール番号
	std::string tool_number{};
	/// @brief 工具基本情報
	ToolBasicInfo basic_info{};
};

/**
 * @struct ToolingListInfo
 * @brief ツーリングリスト全体の情報を保持する構造体
 */
struct ToolingListInfo {
	/// @brief ツーリングリストID (主キー)
	int line_id{};
	/// @brief ライン名
	std::string line_name{};
	/// @brief プログラム名
	std::string prog_name{};
	/// @brief ツーリングリストに含まれる工具のリスト
	std::vector<ToolingListItem> items{};
};

struct BoxInfo {
	/// @brief ボックスID (主キー)
	int box_id{};
	/// @brief 基本工具情報
	ToolBasicInfo item{};
};