/**
 * @file db_client.hpp
 * @brief データベースにアクセスするための関数をまとめたヘッダファイル。
 */
#pragma once

#include <SimpleGtk>
#include "./typedef.hpp"
#include "./parameters.hpp"

bool db_connect(void);

void db_disconnect(void);

bool user_authenticate(int user_id);

bool save_history(void);

std::optional<ToolIndividualInfo> get_tool_data(int basic_tool_id);

std::optional<ToolBasicInfo> get_basic_tool_data(int basic_tool_id);

std::optional<ToolingListInfo> get_tooling_list(int line_id);