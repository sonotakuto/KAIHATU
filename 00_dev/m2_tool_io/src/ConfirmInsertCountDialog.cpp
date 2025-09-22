#include "ConfirmInsertCountDialog.h"

ConfirmInsertCountDialog::ConfirmInsertCountDialog()
    : m_mainVBox(Gtk::ORIENTATION_VERTICAL, 50),
      m_infoLabel("使用するインサートの枚数を\n入力してください"),
      m_inputHBox(Gtk::ORIENTATION_HORIZONTAL, 0),
      m_inputLabel("インサート枚数 ： "),
      m_spinnerVBox(Gtk::ORIENTATION_VERTICAL, 0),
      m_incrementButton("▲"),
      m_decrementButton("▼") {
    
    set_title("使用インサート枚数");
    set_resizable(false);
    set_position(Gtk::WIN_POS_CENTER);
    set_deletable(false);

    Gtk::Box* content_area = get_content_area();
    content_area->set_size_request(670, -1);
    content_area->set_margin_top(50);
    content_area->set_margin_bottom(50);
    
    m_mainVBox.set_halign(Gtk::ALIGN_CENTER);
    m_mainVBox.set_valign(Gtk::ALIGN_CENTER);
    content_area->add(m_mainVBox);

    m_infoLabel.set_size_request(-1, 90);
    m_infoLabel.set_justify(Gtk::JUSTIFY_CENTER);
    m_infoLabel.set_halign(Gtk::ALIGN_CENTER);
    m_infoLabel.get_style_context()->add_class("font-bold");
    m_infoLabel.get_style_context()->add_class("font-size_30px");
    m_mainVBox.pack_start(m_infoLabel, Gtk::PACK_SHRINK);

    m_inputHBox.set_halign(Gtk::ALIGN_CENTER);
    m_inputHBox.set_valign(Gtk::ALIGN_CENTER);
    m_inputHBox.get_style_context()->add_class("font-normal");
    
    m_inputLabel.get_style_context()->add_class("font-size_30px");
    m_inputHBox.pack_start(m_inputLabel, Gtk::PACK_SHRINK);

    m_incrementButton.set_relief(Gtk::RELIEF_NONE);
    m_incrementButton.set_size_request(-1, 50);
    m_incrementButton.get_style_context()->add_class("font-size_40px");
    m_spinnerVBox.pack_start(m_incrementButton, Gtk::PACK_SHRINK);

    m_countEntry.set_text("3");
    m_countEntry.set_alignment(0.5);
    m_countEntry.set_size_request(-1, 50);
    m_countEntry.get_style_context()->add_class("font-size_30px");
    m_spinnerVBox.pack_start(m_countEntry, Gtk::PACK_SHRINK);

    m_decrementButton.set_relief(Gtk::RELIEF_NONE);
    m_decrementButton.set_size_request(-1, 50);
    m_decrementButton.get_style_context()->add_class("font-size_40px");
    m_spinnerVBox.pack_start(m_decrementButton, Gtk::PACK_SHRINK);

    m_inputHBox.pack_start(m_spinnerVBox, Gtk::PACK_SHRINK);
    m_mainVBox.pack_start(m_inputHBox, Gtk::PACK_SHRINK);

    Gtk::ButtonBox* action_area = get_action_area();
    action_area->set_size_request(670, -1);
    action_area->set_layout(Gtk::BUTTONBOX_CENTER);
    action_area->get_style_context()->add_class("font-normal");
    action_area->get_style_context()->add_class("font-size_30px");
    
    // add_buttonの第2引数は、destroy()を使う場合はあまり意味がありませんが、
    // Gtk::RESPONSE_DELETE_EVENTにしておくと整合性がとれます。
    m_okButton = add_button("OK", Gtk::RESPONSE_DELETE_EVENT); 
    m_okButton->set_size_request(200, 60);
    m_okButton->get_style_context()->add_class("button");
    m_okButton->get_style_context()->add_class("trueButton");

    m_incrementButton.signal_clicked().connect(sigc::mem_fun(*this, &ConfirmInsertCountDialog::on_increment_button_clicked));
    m_decrementButton.signal_clicked().connect(sigc::mem_fun(*this, &ConfirmInsertCountDialog::on_decrement_button_clicked));
    m_okButton->signal_clicked().connect(sigc::mem_fun(*this, &ConfirmInsertCountDialog::on_ok_button_clicked));

    show_all_children();
}

ConfirmInsertCountDialog::~ConfirmInsertCountDialog() {}

void ConfirmInsertCountDialog::on_increment_button_clicked() {
    try {
        int current_value = std::stoi(m_countEntry.get_text());
        current_value++;
        m_countEntry.set_text(std::to_string(current_value));
    } catch (const std::exception& e) {}
}

void ConfirmInsertCountDialog::on_decrement_button_clicked() {
    try {
        int current_value = std::stoi(m_countEntry.get_text());
        if (current_value > 0) {
            current_value--;
            m_countEntry.set_text(std::to_string(current_value));
        }
    } catch (const std::exception& e) {}
}

std::string ConfirmInsertCountDialog::get_entry_text() {
    return m_countEntry.get_text();
}

void ConfirmInsertCountDialog::on_ok_button_clicked()
{
    // ★★ 修正点 ★★
    // response()の代わりに、destroy()を直接呼び出します。
    // これはこのウィジェットを強制的にメモリから破棄する命令です。
    destroy();
}