#define DECL
#include "oyaki.hpp"

#include "17_login_window.cpp"
#include "18_setting_window.cpp"
#include "19_main_window.cpp"
#include "20_db_window.cpp"

int main(int argc, char *argv[])
{
    // アプリケーションオブジェクトの生成
    Application app;

    // activateハンドラ
    app.on_activate([&app]{
        load_css("hello.css");                      // CSSファイルを読み込む
        loginWindow = make_unique<LoginWindow>();   // ウィンドウをスマートポインタで動的に生成する
        loginWindow->set_application(app);          // アプリケーションにウィンドウを登録する
        loginWindow->show_all();                    // ウィンドウを表示する
        settingWindow=make_unique<SettingWindow>(); // ウィンドウをスマートポインタで動的に生成する
        mainWindow = make_unique<MainWindow>();     // ウィンドウをスマートポインタで動的に生成する
        dbWindow = make_unique<DbWindow>();         // ウィンドウをスマートポインタで動的に生成する
    });

    // メッセージループを開始（activateハンドラが呼び出される）
    app.run(argc, argv);
}