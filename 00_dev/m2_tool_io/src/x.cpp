#pragma once
#include "oyaki.hpp" // 提供されたフレームワークのヘッダ
#include <string>

// ConfirmInsertDialogの定義
struct ConfirmInsertDialog : Window { // Windowクラスを継承
    //== UI要素の定義 ==//
    Glade glade{"20_confirm_insert_dialog.glade"}; // UI定義ファイルを読み込む
    Label  countValueLabel{glade, "countValueLabel"}; // 数値を表示するラベル
    Button upBtn{glade, "upBtn"};     // ▲ボタン
    Button downBtn{glade, "downBtn"}; // ▼ボタン
    Button okBtn{glade, "okBtn"};     // OKボタン
    Button cancelBtn{glade, "cancelBtn"}; // キャンセルボタン (Gladeで定義)

    //== 内部データ ==//
    int insertCount = 3; // インサート枚数の初期値を3に設定

    //== コンストラクタ: UIの初期化とイベント接続を行う ==//
    ConfirmInsertDialog() {
        // Windowの初期化
        this->init(glade, "confirmInsertDialog");

        // 初期値をラベルに表示
        this->update_label();

        // ▲ボタンがクリックされた時の処理
        upBtn.on_clicked([this](){
            this->insertCount++;
            this->update_label();
        });

        // ▼ボタンがクリックされた時の処理
        downBtn.on_clicked([this](){
            if (this->insertCount > 1) { // 1未満にはしない
                this->insertCount--;
                this->update_label();
            }
        });

        // OKボタンがクリックされた時の処理
        okBtn.on_clicked([this](){
            // ダイアログを閉じ、GTK_RESPONSE_OKを返す
            this->response(GTK_RESPONSE_OK);
        });

        // キャンセルボタンも同様に
        cancelBtn.on_clicked([this](){
            this->response(GTK_RESPONSE_CANCEL);
        });
    }

    //== 公開メソッド ==//
    // 決定されたインサート枚数を取得する
    int get_insert_count() const {
        return this->insertCount;
    }

private:
    //== ヘルパーメソッド ==//
    // 数値ラベルを現在のinsertCountで更新する
    void update_label() {
        countValueLabel.set_text(std::to_string(this->insertCount).c_str());
    }
};