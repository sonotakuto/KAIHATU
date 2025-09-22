#include "oyaki.hpp"

// LoginWindowの定義
struct LoginWindow : Window {                       // Windowクラスを継承
    Glade glade{"17_login_window.glade"};           // Gladeファイルを読み込む
    Entry  passEntry{glade, "passEntry"};           // GladeのpassEntryの定義からエントリーウィジェットを生成
    vector<Button> keys;
    Button shutdownBtn{glade, "shutdownBtn"};       // GladeのshutdownBtnの定義からボタンウィジェットを生成
    Button dbBtn{glade, "dbBtn"};                   // GladeのdbBtnの定義からボタンウィジェットを生成
    Button settingBtn{glade, "settingBtn"};         // GladeのsettingBtnの定義からボタンウィジェットを生成
    Label  timeLabel{glade, "timeLabel"};           // GladeのtimeLabelの定義からラベルウィジェットを生成
    MessageDialog dialog{glade, "shutdownDialog"};  // GladeのshutdownDialogの定義からメッセージダイアログを生成

    // コンストラクタ
    LoginWindow() {
        this->init(glade, "loginWindow");           // GladeのloginWindowの定義からウィンドウを初期化
        this->set_timeout(1000);                    // timeoutハンドラのインターバルタイマを1秒間隔に設定

        keys.emplace_back(glade, "key0");           // Gladeのkey0の定義からボタンウィジェットを生成
        keys.emplace_back(glade, "key1");           // Gladeのkey1の定義からボタンウィジェットを生成
        keys.emplace_back(glade, "key2");           // Gladeのkey2の定義からボタンウィジェットを生成
        keys.emplace_back(glade, "key3");           // Gladeのkey3の定義からボタンウィジェットを生成
        keys.emplace_back(glade, "key4");           // Gladeのkey4の定義からボタンウィジェットを生成
        keys.emplace_back(glade, "key5");           // Gladeのkey5の定義からボタンウィジェットを生成
        keys.emplace_back(glade, "key6");           // Gladeのkey6の定義からボタンウィジェットを生成
        keys.emplace_back(glade, "key7");           // Gladeのkey7の定義からボタンウィジェットを生成
        keys.emplace_back(glade, "key8");           // Gladeのkey8の定義からボタンウィジェットを生成
        keys.emplace_back(glade, "key9");           // Gladeのkey9の定義からボタンウィジェットを生成
        keys.emplace_back(glade, "keyBS");          // GladeのkeyBSの定義からボタンウィジェットを生成
        keys.emplace_back(glade, "keyEnter");       // GladeのkeyEnterの定義からボタンウィジェットを生成

        for (Button& key : keys) {
            key.on_clicked([this, &key]{            // ボタンがクリックされたときの処理を登録
                this->key_on_clicked(key);
            });
        }

        shutdownBtn.on_clicked([this](){            // ボタンがクリックされたときの処理を登録
            if (this->dialog.run(this)==GTK_RESPONSE_YES) {
                print("Yes\n");
                this->destroy();
            } else {
                print("No\n");
            }
        });

        dbBtn.on_clicked([this](){                  // ボタンがクリックされたときの処理を登録
            print("ＤＢ\n");
            dbWindow->show_all();
            loginWindow->hide();
        });

        settingBtn.on_clicked([this](){             // ボタンがクリックされたときの処理を登録
            print("設定\n");
            settingWindow->show_all();
            loginWindow->hide();
        });
    }

    // timeoutハンドラ
    bool on_timeout() override {
        time_point t = now();                       // 現在日時を取得
        string str = format("{:%Y/%m/%d}\n",t);     // 年月日の文字列を作成
        str += format("{:%H:%M:%S}",t);             // 時分秒の文字列を追加
        timeLabel.set_text(str.c_str());            // 時刻をラベルに表示
        return true;
    }

    // key0～9,keyBS,keyEnterのクリックハンドラ
    void key_on_clicked(Button& key){
        string key_label = key.get_label();
        //print("key_label={}\n", key_label);

        string passEntry_text = passEntry.get_text();
        if (key_label == "BS") {
            if (passEntry_text.length()>0) {
                passEntry_text.erase(passEntry_text.length()-1);
                passEntry.set_text(passEntry_text.c_str());
            }
        } else if (key_label == "接続") {
            print("子機に接続 : passEntry_text={}\n", passEntry_text);
            mainWindow->show_all();
            loginWindow->hide();
            return;
        } else {
            passEntry_text += key_label;
            passEntry.set_text(passEntry_text.c_str());
        }
        //print("passEntry_text={}\n", passEntry.get_text());
    }
};