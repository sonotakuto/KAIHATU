#include <gtkmm.h>
#include <iostream>
#include <string>

// Glade XMLで定義されたダイアログをC++クラスとして実装
class ConfirmInsertCountDialog : public Gtk::Dialog {
public: // ← この public: セクションごと追加
    ConfirmInsertCountDialog();
    virtual ~ConfirmInsertCountDialog();
    std::string get_entry_text(); // ← この行を追加

protected:
    // ... (以下、既存のコード)
    // シグナルハンドラ (イベントコールバック関数)
    void on_increment_button_clicked();
    void on_decrement_button_clicked();
    void on_ok_button_clicked();

    // メンバー変数 (ウィジェット)
    Gtk::Box m_mainVBox;
    Gtk::Label m_infoLabel;
    Gtk::Box m_inputHBox;
    Gtk::Label m_inputLabel;
    Gtk::Box m_spinnerVBox;
    Gtk::Button m_incrementButton;
    Gtk::Entry m_countEntry;
    Gtk::Button m_decrementButton;
    Gtk::Button* m_okButton; // アクションエリアのボタンはポインタで取得
};

ConfirmInsertCountDialog::ConfirmInsertCountDialog()
    : m_mainVBox(Gtk::ORIENTATION_VERTICAL, 50),
      m_infoLabel("使用するインサートの枚数を\n入力してください"),
      m_inputHBox(Gtk::ORIENTATION_HORIZONTAL, 0),
      m_inputLabel("インサート枚数 ： "),
      m_spinnerVBox(Gtk::ORIENTATION_VERTICAL, 0),
      m_incrementButton("▲"),
      m_decrementButton("▼") {
    
    // 1. ダイアログウィンドウの基本設定
    set_title("再研磨工具か確認");
    set_resizable(false);
    set_position(Gtk::WIN_POS_CENTER);
    set_deletable(false); // タイトルバーのxボタンを無効化

    // 2. メインの垂直ボックスを取得し、プロパティ設定
    Gtk::Box* content_area = get_content_area();
    content_area->set_size_request(670, -1);
    content_area->set_margin_top(50);
    content_area->set_margin_bottom(50);
    
    m_mainVBox.set_halign(Gtk::ALIGN_CENTER);
    m_mainVBox.set_valign(Gtk::ALIGN_CENTER);
    content_area->add(m_mainVBox);

    // 3. 上部の説明ラベルの設定
    m_infoLabel.set_size_request(-1, 90);
    m_infoLabel.set_justify(Gtk::JUSTIFY_CENTER);
    m_infoLabel.set_halign(Gtk::ALIGN_CENTER);
    m_infoLabel.get_style_context()->add_class("font-bold");
    m_infoLabel.get_style_context()->add_class("font-size_30px");
    m_mainVBox.pack_start(m_infoLabel, Gtk::PACK_SHRINK);

    // 4. 中央の入力エリア (水平ボックス) の設定
    m_inputHBox.set_halign(Gtk::ALIGN_CENTER);
    m_inputHBox.set_valign(Gtk::ALIGN_CENTER);
    m_inputHBox.get_style_context()->add_class("font-normal");
    
    // 4-1. "インサート枚数" ラベル
    m_inputLabel.get_style_context()->add_class("font-size_30px");
    m_inputHBox.pack_start(m_inputLabel, Gtk::PACK_SHRINK);

    // 4-2. スピナー風ウィジェット (垂直ボックス)
    // INCREMENTボタン (▲)
    m_incrementButton.set_relief(Gtk::RELIEF_NONE);
    m_incrementButton.set_size_request(-1, 50);
    m_incrementButton.get_style_context()->add_class("font-size_40px");
    m_spinnerVBox.pack_start(m_incrementButton, Gtk::PACK_SHRINK);

    // ENTRY (数値入力欄)
    m_countEntry.set_text("0");
    m_countEntry.set_alignment(0.5); // 中央揃え
    m_countEntry.set_size_request(-1, 50);
    m_countEntry.get_style_context()->add_class("font-size_30px");
    m_spinnerVBox.pack_start(m_countEntry, Gtk::PACK_SHRINK);

    // DECREMENTボタン (▼)
    m_decrementButton.set_relief(Gtk::RELIEF_NONE);
    m_decrementButton.set_size_request(-1, 50);
    m_decrementButton.get_style_context()->add_class("font-size_40px");
    m_spinnerVBox.pack_start(m_decrementButton, Gtk::PACK_SHRINK);

    m_inputHBox.pack_start(m_spinnerVBox, Gtk::PACK_SHRINK);
    m_mainVBox.pack_start(m_inputHBox, Gtk::PACK_SHRINK);

    // 5. 下部のアクションエリア (OKボタン)
    Gtk::ButtonBox* action_area = get_action_area();
    action_area->set_size_request(670, -1);
    action_area->set_layout(Gtk::BUTTONBOX_CENTER);
    action_area->get_style_context()->add_class("font-normal");
    action_area->get_style_context()->add_class("font-size_30px");
    
    // add_buttonはGtk::Button*を返す
    m_okButton = add_button("OK", Gtk::RESPONSE_OK);
    m_okButton->set_size_request(200, 60);
    m_okButton->get_style_context()->add_class("button");
    m_okButton->get_style_context()->add_class("trueButton");

    // 6. シグナルハンドラの接続
    m_incrementButton.signal_clicked().connect(sigc::mem_fun(*this, &ConfirmInsertCountDialog::on_increment_button_clicked));
    m_decrementButton.signal_clicked().connect(sigc::mem_fun(*this, &ConfirmInsertCountDialog::on_decrement_button_clicked));
    // OKボタンはresponseシグナルで処理するため、ここでは特に接続しない
    // もし特別な処理が必要なら、以下のように接続する
    m_okButton->signal_clicked().connect(sigc::mem_fun(*this, &ConfirmInsertCountDialog::on_ok_button_clicked));

    // 7. すべてのウィジェットを表示
    show_all_children();
}

ConfirmInsertCountDialog::~ConfirmInsertCountDialog() {}

// ▲ボタンがクリックされたときの処理
void ConfirmInsertCountDialog::on_increment_button_clicked() {
    try {
        int current_value = std::stoi(m_countEntry.get_text());
        current_value++;
        m_countEntry.set_text(std::to_string(current_value));
    } catch (const std::invalid_argument& ia) {
        // エントリが数字でない場合は何もしない
    }
}

// ▼ボタンがクリックされたときの処理
void ConfirmInsertCountDialog::on_decrement_button_clicked() {
    try {
        int current_value = std::stoi(m_countEntry.get_text());
        if (current_value > 0) { // 0未満にはしない例
            current_value--;
            m_countEntry.set_text(std::to_string(current_value));
        }
    } catch (const std::invalid_argument& ia) {
        // エントリが数字でない場合は何もしない
    }
}

// OKボタンがクリックされたときの処理 (今回は不要だが参考として)
void ConfirmInsertCountDialog::on_ok_button_clicked() {
    std::cout << "OKボタンが押されました。入力値: " << m_countEntry.get_text() << std::endl;
    hide(); // ダイアログを閉じる
}


int main(int argc, char* argv[]) {
    // アプリケーションの初期化
    auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

    // CSSプロバイダの作成とスタイルの適用
    // XML内の<style>タグはCSSクラスを定義しています。
    // これらのスタイルを適用するには、CSSデータが必要です。
    // ここでは、XMLのクラス名から推測される簡単なCSSを定義します。
    auto css_provider = Gtk::CssProvider::create();
    std::string css_data = R"(
        .font-size_30px { font-size: 30px; }
        .font-size_40px { font-size: 40px; }
        .font-bold { font-weight: bold; }
        .font-normal { font-weight: normal; }
        .trueButton { 
            background-color: #4CAF50; /* 例: 緑色 */
            color: white;
        }
        .button {
             border-radius: 5px;
        }
    )";
    
    // CSSデータを読み込む
    if (!css_provider->load_from_data(css_data)) {
        std::cerr << "Failed to load CSS" << std::endl;
        return 1;
    }

    // CSSプロバイダをデフォルトスクリーンに適用
    auto screen = Gdk::Screen::get_default();
    auto style_context = Gtk::StyleContext::create();
    style_context->add_provider_for_screen(screen, css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // ダイアログを作成して実行
    ConfirmInsertCountDialog dialog;
    int result = app->run(dialog); // run()はダイアログが閉じるまでブロックする

    // ダイアログが閉じた後の処理
    std::cout << "ダイアログが閉じられました。レスポンスコード: " << result << std::endl;
    if (result == Gtk::RESPONSE_OK) {
        std::cout << "最終的なインサート枚数: " << dialog.get_entry_text() << std::endl;
    } else {
        std::cout << "キャンセルされました。" << std::endl;
    }

    return 0;
}

// ConfirmInsertCountDialogクラスに、エントリーのテキストを取得する公開メソッドを追加
std::string ConfirmInsertCountDialog::get_entry_text() {
    return m_countEntry.get_text();
}