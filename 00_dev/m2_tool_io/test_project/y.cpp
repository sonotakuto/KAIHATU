#define DECL
#include "oyaki.hpp"
#include <string>

// --- ダイアログの定義 ---
struct ConfirmInsertCountDialog : sg::MessageDialog {
    sg::Glade glade{"gui/tool_io.glade"};
    sg::Button  insertCountOkButton{glade, "insertCountOkButton"};
    ConfirmInsertCountDialog() {
        this->init(glade, "confirmInsertCountDialog");
        insertCountOkButton.on_clicked([this](){
            this->hide();
        });
    }
};

// --- main関数 ---
int main(int argc, char* argv[]) {
    sg::Application app; 

    // アプリケーションが起動した時に、この[ ]の中の処理を実行する
    app.on_activate([&]{
        // 1. ダイアログの親となる、空のメインウィンドウを作成する
        auto main_win = make_unique<sg::Window>();

        // 2. ダイアログのインスタンスを生成する
        confirmDialog = make_unique<ConfirmInsertCountDialog>();

        // 3. メインウィンドウを親としてダイアログを実行する
        static_cast<sg::Dialog*>(confirmDialog.get())->run(main_win.get());

        // 4. ダイアログが閉じられたらアプリケーションは自動で終了するため、app.quit()は削除
    });

    // アプリケーションのメインループを開始する (argc, argvを渡すように修正)
    return app.run(argc, argv);
}