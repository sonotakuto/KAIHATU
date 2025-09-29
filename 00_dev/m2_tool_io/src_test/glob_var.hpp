#pragma once
#include <memory> // unique_ptrのために必要

// ■■■■ 課題 (コンパイルエラー修正) ここから修正 ■■■■

// クラスの完全な定義は不要なため、前方宣言に切り替える
class MainWindow;
class DbClient;
class ToolHandling;

// unique_ptr を前方宣言された型で使うための typedef
using MainWindow_ptr = std::unique_ptr<MainWindow>;
using DbClient_ptr = std::unique_ptr<DbClient>;
using ToolHandling_ptr = std::unique_ptr<ToolHandling>;

extern MainWindow_ptr main_window;
extern DbClient_ptr db_client;
extern ToolHandling_ptr tool_handling;

// ■■■■ 課題 (コンパイルエラー修正) ここまで修正 ■■■■