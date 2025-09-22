#ifndef CONFIRM_INSERT_COUNT_DIALOG_H
#define CONFIRM_INSERT_COUNT_DIALOG_H

#include <gtkmm.h>
#include <string>

class ConfirmInsertCountDialog : public Gtk::Dialog {
public:
    ConfirmInsertCountDialog();
    virtual ~ConfirmInsertCountDialog();

    std::string get_entry_text();

protected:
    // シグナルハンドラ
    void on_increment_button_clicked();
    void on_decrement_button_clicked();
    void on_ok_button_clicked(); // この行が必要です

    // メンバー変数
    Gtk::Box m_mainVBox;
    Gtk::Label m_infoLabel;
    Gtk::Box m_inputHBox;
    Gtk::Label m_inputLabel;
    Gtk::Box m_spinnerVBox;
    Gtk::Button m_incrementButton;
    Gtk::Entry m_countEntry;
    Gtk::Button m_decrementButton;
    Gtk::Button* m_okButton;
};

#endif // CONFIRM_INSERT_COUNT_DIALOG_H