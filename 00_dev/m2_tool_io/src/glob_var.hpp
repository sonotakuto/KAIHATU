/**
 * @file glob_var.hpp
 * @brief 複数ファイルで共通して使用するグローバル変数を定義するヘッダファイル。
 */
#pragma once
#include "./includes.hpp"

// 定義方法のサンプル
// inline int sample_value = 100;

// --- グローバル変数 ---
// アプリケーション全体で唯一のウィンドウインスタンスへのポインタ。
inline std::unique_ptr<MainWindow> mainWindow;