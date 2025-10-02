#include "glob_var.hpp"

// インスタンスを生成するために、ここで完全なクラス定義をインクルードする
#include "tool_io_gui.hpp"
#include "db_client.hpp"
#include "tool_handling.hpp"

MainWindow_ptr main_window;
DbClient_ptr db_client;
ToolHandling_ptr tool_handling;

int main(int argc, char *argv[])
{
    Application app;

    app.on_activate([&]
                    {
        // ■■■■ 課題 (CSS適用) ここから追加 ■■■■
        // CSSファイルを読み込み、アプリケーション全体にスタイルを適用する
        load_css("tool_io.css");
        // ■■■■ 課題 (CSS適用) ここまで追加 ■■■■

        db_client = make_unique<DbClient>();
        main_window = make_unique<MainWindow>();
        tool_handling = make_unique<ToolHandling>();

        main_window->set_application(app);
        main_window->show_all(); });

    return app.run(argc, argv);
}