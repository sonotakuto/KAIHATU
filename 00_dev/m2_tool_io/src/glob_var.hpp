/**
 * @file glob_var.hpp
 * @brief 複数ファイルで共通して使用するグローバル変数を定義するヘッダファイル。
 */
#pragma once
#include "./includes.hpp"

// 定義方法のサンプル
// inline int sample_value = 100;

// --- グローバル変数 ---
/// @brief ログイン中のユーザー情報を保持する構造体。
inline std::optional<UserInfo> g_logged_in_user_info = std::nullopt;
/// @brief パスワード認証が成功したかどうかのフラグ。
inline bool g_password_authenticated = false;