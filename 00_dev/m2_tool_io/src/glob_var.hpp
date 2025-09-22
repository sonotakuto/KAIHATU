#pragma once

#include "./includes.hpp"

// global変数を一括で実体とextern宣言する。
// include元で_GLOB_VARが定義されていれば実体宣言、定義されていなければ外部宣言となる。

// 初期化子が必要なクラスは以下で直接宣言
#ifdef _GLOB_VAR
#define GLOBAL
//std::vector<bool> sample{false, true};
#else
#define GLOBAL extern
//extern std::vector<bool> sample;
#endif

//GLOBAL int sumple;
GLOBAL std::unique_ptr<OdbcSession> g_db_session;