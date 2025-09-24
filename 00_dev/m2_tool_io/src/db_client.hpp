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

void user_authenticate(void);

void save_history(void);

void get_tool_data(void);

void get_tooling_list(void);