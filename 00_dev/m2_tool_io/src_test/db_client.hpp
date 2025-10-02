#pragma once
#include "includes.hpp"

class MainWindow : public Window
{
public:
    Glade glade;
    // (既存のウィジェット変数は省略)
    Button retrieval_button, replenishment_button, inventory_check_button;

    // ■■■■ 課題 (ダミー動作) ここから追加 ■■■■
    // QRスキャン画面
    Label qr_scan_prompt_label;
    Button simulate_scan_button;

    // 工具選択画面
    ScrolledWindow tool_list_scrolled_window;
    TreeView tool_list_treeview;
    ListStore tool_list_store;
    Label line_label;
    Label program_label;
    Button back_button;
    // (ページ操作ボタンなども同様に追加)
    // ■■■■ 課題 (ダミー動作) ここまで追加 ■■■■

    // (ページ状態管理変数は省略)

    MainWindow();
    void show_initial_menu();
    void show_qr_scan_page(); // QRスキャン画面表示用のメソッド
    void show_page_tool_selection(const ToolingList& tooling_list);

private:
    // (既存のメソッドは省略)
    void on_simulate_scan_button_clicked(); // 仮ボタンのハンドラ
    void on_back_button_clicked(); // 戻るボタンのハンドラ
};