#include <gtkmm.h>
#include <iostream>
#include <string>

// --- ダイアログの定義と実装をこのファイル内にすべて記述 ---
class TestDialog : public Gtk::Dialog {
public:
    TestDialog();
    virtual ~TestDialog();

protected:
    // メンバー変数
    Gtk::Label m_infoLabel;
    Gtk::Button* m_okButton;
    Gtk::Button* m_cancelButton;
};

TestDialog::TestDialog()
    : m_infoLabel("このテストダイアログは\nOKボタンで閉じますか？") {
    
    // ダイアログの基本設定
    set_title("最小構成テスト");
    set_size_request(400, 200);
    set_position(Gtk::WIN_POS_CENTER);

    // ラベルを配置
    Gtk::Box* content_area = get_content_area();
    content_area->set_halign(Gtk::ALIGN_CENTER);
    content_area->set_valign(Gtk::ALIGN_CENTER);
    content_area->add(m_infoLabel);

    // ボタンを追加 (これが最も標準的な実装です)
    m_okButton = add_button("OK", Gtk::RESPONSE_OK);
    m_cancelButton = add_button("キャンセル", Gtk::RESPONSE_CANCEL);
    
    show_all_children();
}

TestDialog::~TestDialog() {}
// --- ここまでダイアログのコード ---


// --- メインの実行部分 ---
int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.gtkmm.test");

    std::cout << "テストダイアログを表示します..." << std::endl;

    TestDialog dialog;
    int result = app->run(dialog); // run()が応答を待つ

    // ダイアログが閉じた後に実行される
    if (result == Gtk::RESPONSE_OK) {
        std::cout << "成功: OKボタンでダイアログが閉じました。" << std::endl;
    } else if (result == Gtk::RESPONSE_CANCEL) {
        std::cout << "成功: キャンセルボタンでダイアログが閉じました。" << std::endl;
    } else {
        std::cout << "ダイアログが予期せず閉じられました。コード: " << result << std::endl;
    }

    return 0;
}