#define DECL
#include "oyaki.hpp"
#include <string>

// --- ConfirmInsertCountDialogクラスの定義 ---
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

    app.on_activate([&]{
        // 修正点1: Gladeファイルを読み込むオブジェクトを作成
        sg::Glade glade("gui/tool_io.glade");

        // 修正点2: Gladeファイルからmain_winを初期化して、有効な親ウィンドウを作成
        auto main_win = make_unique<sg::Window>();
        main_win->init(glade, "main_win");

        // ダイアログのインスタンスを生成
        confirmInsertDialog = make_unique<ConfirmInsertCountDialog>();

        // 有効な親ウィンドウを渡してダイアログを実行
        static_cast<sg::Dialog*>(confirmInsertDialog.get())->run(main_win.get());
        
        // ダイアログが閉じたらアプリは終了
    });

    return app.run(argc, argv);
}