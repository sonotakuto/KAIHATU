#include "ConfirmInsertCountDialog.h"
#include <iostream>

int main(int argc, char* argv[]) {
    // アプリケーションの初期化
    auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example.dialog");

    // ダイアログを作成
    ConfirmInsertCountDialog dialog;
    
    // ダイアログを実行し、ユーザーの応答を待つ
    // `run()`はダイアログが閉じるまで処理を停止します
    int result = app->run(dialog);

    // ダイアログが閉じた後の処理
    if (result == Gtk::RESPONSE_OK) {
        std::cout << "OKボタンが押されました。" << std::endl;
        std::cout << "最終的なインサート枚数: " << dialog.get_entry_text() << std::endl;
    } else {
        std::cout << "ダイアログが閉じられました (応答コード: " << result << ")" << std::endl;
    }

    return 0;
}