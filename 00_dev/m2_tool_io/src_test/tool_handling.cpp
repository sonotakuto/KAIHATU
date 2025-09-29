#include "tool_handling.hpp"
#include "glob_var.hpp"

// ■■■■ 課題 (ステップ1-2) ここから追加 ■■■■

void ToolHandling::retrieve_tool(const string &qr_code)
{
    // TODO: ステップ3で実装
    // ここで select_retrieval_tool や verify_tool_checkout を呼び出し、
    // 取り出しプロセス全体を制御する。
    print("retrieve_tool called with QR: {}\n", qr_code);
}

optional<vector<Tool>> ToolHandling::select_retrieval_tool(const string &qr_code)
{
    // TODO: ステップ2, 3で実装
    // 1. db_client->get_tooling_list(qr_code) を呼び出す
    // 2. main_window->show_tool_selection_screen(...) で画面を表示・操作させる
    // 3. ユーザーが選択した工具のリストを返す
    print("select_retrieval_tool called\n");
    return nullopt; // 今は何も選択されなかったとしてnulloptを返す
}

bool ToolHandling::verify_tool_checkout(const Tool &tool_to_verify)
{
    // TODO: ステップ3で実装
    // 1. main_window->show_verify_screen(...) で画面を表示
    // 2. 工具追跡システムからのスキャン通知を待つ
    // 3. 必要なら個数入力ダイアログを表示
    // 4. db_client->save_history(...) で履歴を保存
    // 5. ユーザーの選択に応じて true/false を返す
    print("verify_tool_checkout called for tool: {}\n", tool_to_verify.tool_id);
    return false; // 今は「終了」が押されたとしてfalseを返す
}

// ■■■■ 課題 (ステップ1-2) ここまで追加 ■■■■