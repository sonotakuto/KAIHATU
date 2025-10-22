/**
 * @file db_client.hpp
 * @brief データベースにアクセスするための関数をまとめたヘッダファイル。
 */
#pragma once
#include <SimpleGtk>
#include "./typedef.hpp"
#include "./parameters.hpp"

/**
 * @namespace db
 * @brief データベース接続に関連する関数と定数を格納する名前空間。
 */
namespace db {

constexpr int	RETRY_LIMIT	= 3;	// DB接続のリトライ回数
constexpr const char*	ODBC_DSN	= "odbc_test";	// ODBCのDSN名
constexpr const char*	ODBC_USER	= "pi";			// ODBCのユーザー名
constexpr const char*	ODBC_PASS	= "pipasswd";	// ODBCのパスワード

bool connect(void);

void disconnect(void);

bool user_authenticate(int user_id);

bool save_history(const RecOperationTable& rec);

/**
 * @brief ユーザーIDを基にユーザー情報をデータベースから取得します。
 * @param user_id 取得対象のユーザーID。
 * @return std::optional<MstUserInfo> ユーザー情報が見つかった場合はその構造体、見つからなければstd::nullopt。
 */
std::optional<MstUserInfo> get_MstUserInfo(int user_id);

/**
 * @brief 個別工具IDを基に、関連する全ての情報（基本情報、保管領域情報など）を結合して取得します。
 * @param unique_tool_id 取得対象の個別工具ID。
 * @return std::optional<UniqueToolInfo> 工具情報が見つかった場合はその構造体、見つからなければstd::nullopt。
 */
std::optional<UniqueToolInfo> get_UniqueToolInfo(int unique_tool_id);

/**
 * @brief プログラム割り当てIDを基に、ツーリングリスト全体の情報（それに含まれる全工具の情報を含む）を取得します。
 * @param program_assignment_id 取得対象のプログラム割り当てID。
 * @return std::optional<ToolingListInfo> ツーリングリストが見つかった場合はその構造体、見つからなければstd::nullopt。
 */
std::optional<ToolingListInfo> get_ToolingListInfo(int program_assignment_id);

} // namespace db