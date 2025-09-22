#pragma once
#include "oyaki.hpp"
#include <string>

struct ConfirmInsertDialog : InsertWindow {
    Glade glade{"tool_io.glade"};
    Label  countValueLabel{glade, "countValueLabel"};
    Button upBtn{glade, "upBtn"};
    Button downBtn{glade, "downBtn"};
    Button okBtn{glade, "okBtn"};
    Button cancelBtn{glade, "cancelBtn"};

    int insertCount = 3;
    int response_type = GTK_RESPONSE_CANCEL; // 結果を保持する変数

    ConfirmInsertDialog() {
        this->init(glade, "confirmInsertDialog");
        this->update_label();
        upBtn.on_clicked([this](){ this->insertCount++; this->update_label(); });
        downBtn.on_clicked([this](){ if (this->insertCount > 1) { this->insertCount--; this->update_label(); }});

        // OK/Cancelボタンが押されたら結果を保存してダイアログを閉じる(hide)
        okBtn.on_clicked([this](){ this->response_type = GTK_RESPONSE_OK; this->hide(); });
        cancelBtn.on_clicked([this](){ this->response_type = GTK_RESPONSE_CANCEL; this->hide(); });
    }

    int get_insert_count() const { return this->insertCount; }
    int get_response_type() const { return this->response_type; }

private:
    void update_label() { countValueLabel.set_text(std::to_string(this->insertCount).c_str()); }
};