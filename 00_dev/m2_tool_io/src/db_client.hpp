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

bool save_history(void);

std::optional<MstUserTable> get_user_data(int user_id);

std::optional<MstToolTable> get_mst_tool_data(int basic_tool_id);

std::optional<UniqueToolTable> get_unique_tool_data(int unique_tool_id);

std::optional<ToolingListInfo> get_tooling_list(int line_id);

} // namespace db