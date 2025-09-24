/**
 * @file tool_io_gui.hpp
 * @brief アプリケーションのメインウィンドウクラスを定義したヘッダファイル。
 */
#pragma once
#include <SimpleGtk>
#include "./typedef.hpp"
#include "./parameters.hpp"
#include "./SimpleGtk_extension.hpp"

/**
 * @class MainWindow
 * @brief アプリケーションのメインウィンドウクラス。
 * @details GladeファイルからUI定義を読み込み、ウィジェットの管理とイベント処理を担当します。
 */
struct MainWindow : public sg::Window {
	//=========================================================================
	// メンバ変数 (UIウィジェット)
	//=========================================================================

	//! GladeファイルからUI定義を読み込むためのオブジェクト
	Glade glade;

	// --- ヘッダー部 ---
	//! 日時を表示するラベル (ID: dateTimeLabel)
	sg::Label dateTimeLabel;
	//! 操作モードを「取り出し」に切り替えるトグルボタン (ID: retrieveToggleButton)
	tool_io::ToggleButton retrieveToggleButton;
	//! 操作モードを「補充」に切り替えるトグルボタン (ID: storeToggleButton)
	tool_io::ToggleButton storeToggleButton;
	//! 操作モードを「棚卸し」に切り替えるトグルボタン (ID: reconcileToggleButton)
	tool_io::ToggleButton reconcileToggleButton;
	//! かな/漢字入力を切り替えるボタン (ID: kanaKanjiButton)
	sg::Button kanaKanjiButton;
	//! ログアウト処理を開始するボタン (ID: logoutButton)
	sg::Button logoutButton;
	//! ログイン中のユーザーIDを表示/入力するエントリー (ID: userIdEntry)
	sg::Entry userIdEntry;

	// --- 中央操作部 ---
	//! ユーザーへの指示などを表示するメッセージラベル (ID: messageLabel)
	sg::Label messageLabel;
	//! "取り出し", "補充", "棚卸し"の各操作パネルを切り替えるスタック (ID: operationStack)
	tool_io::Stack operationStack;
	//! (取り出し時) ライン番号を入力するエントリー (ID: lineEntry)
	sg::Entry lineEntry;
	//! (取り出し時) プログラム番号を入力するエントリー (ID: programEntry)
	sg::Entry programEntry;
	//! (補充時) 補充する工具の情報を表示するテーブル (ID: storingToolTable)
	sg::TreeView storingToolTable;
	//! (棚卸し時) 絞り込みダイアログを開くボタン (ID: filterButton)
	sg::Button filterButton;
	//! (棚卸し時) 現在操作中のコンテナ番号を表示するエントリー (ID: containerEntry)
	sg::Entry containerEntry;

	// --- テーブル表示部 ---
	//! 複数のテーブル表示形式を切り替えるスタック (ID: table_stack)
	tool_io::Stack table_stack;
	//! 3列表示テーブル (1) (ID: table3column1st)
	sg::TreeView table3column1st;
	//! 3列表示テーブル (2) (ID: table3column2nd)
	sg::TreeView table3column2nd;
	//! 3列表示テーブル (3) (ID: table3column3rd)
	sg::TreeView table3column3rd;
	//! 2列表示テーブル (1) (ID: table2column1st)
	sg::TreeView table2column1st;
	//! 2列表示テーブル (2) (ID: table2column2nd)
	sg::TreeView table2column2nd;

	// --- フッター部 ---
	//! テーブルのページネーション(ページ送り)UIを格納するボックス (ID: paginationBox)
	tool_io::Box paginationBox;
	//! 前のページへ移動するボタン (ID: prevButton)
	sg::Button prevButton;
	//! 次のページへ移動するボタン (ID: nextButton)
	sg::Button nextButton;
	//! 現在のページ番号を表示するラベル (ID: pageNumberLabel)
	sg::Label pageNumberLabel;
	//! 肯定的なアクション（次へ, 確定など）を行うボタン (ID: trueButton)
	sg::Button trueButton;
	//! 否定的なアクション（キャンセルなど）を行うボタン (ID: falseButton)
	sg::Button falseButton;

	// --- ダイアログの親ウィジェット ---
	//! インサート枚数確認ダイアログ (ID: confirmInsertCountDialog)
	sg::Dialog confirmInsertCountDialog;
	//! 再研磨確認ダイアログ (ID: confirmRegrindDialog)
	sg::Dialog confirmRegrindDialog;
	//! ログアウト確認ダイアログ (ID: logoutDialog)
	sg::Dialog logoutDialog;
	//! パスワード入力ダイアログ (ID: passwdEntryDialog)
	sg::Dialog passwdEntryDialog;
	//! 警告表示ダイアログ (ID: warningDialog)
	sg::Dialog warningDialog;

	// --- ダイアログ内のウィジェット ---
	//! (枚数確認) ダイアログのメインメッセージラベル (ID: confirmInsertCountLabel)
	sg::Label confirmInsertCountLabel;
	//! (枚数確認) 枚数を増やすボタン (ID: incrementButton)
	sg::Button incrementButton;
	//! (枚数確認) 枚数を減らすボタン (ID: decrementButton)
	sg::Button decrementButton;
	//! (枚数確認) 枚数を入力するエントリー (ID: countEntry)
	sg::Entry countEntry;
	//! (枚数確認) OKボタン (ID: insertCountOkButton)
	sg::Button insertCountOkButton;
	//! (再研磨確認) 「いいえ」トグルボタン (ID: notRegrindButton)
	tool_io::ToggleButton notRegrindButton;
	//! (再研磨確認) 「はい」トグルボタン (ID: regrindButton)
	tool_io::ToggleButton regrindButton;
	//! (再研磨確認) OKボタン (ID: confirmRegrindOkButton)
	sg::Button confirmRegrindOkButton;
	//! (ログアウト確認) 「いいえ」ボタン (ID: logoutNoButton)
	sg::Button logoutNoButton;
	//! (ログアウト確認) 「はい」ボタン (ID: logoutYesButton)
	sg::Button logoutYesButton;
	//! (パスワード) キャンセルボタン (ID: cancelBtn)
	sg::Button cancelBtn;
	//! (パスワード) 確定ボタン (ID: enterBtn)
	sg::Button enterBtn;
	//! (パスワード) パスワード入力欄 (ID: passwordEntry)
	sg::Entry passwordEntry;
	//! (警告) 警告メッセージを表示するラベル (ID: warningLabel)
	sg::Label warningLabel;

	//=========================================================================
	// コンストラクタ
	//=========================================================================

	/**
	 * @brief MainWindowのコンストラクタ。
	 * @details ウィジェットの初期化とイベントハンドラの登録を行います。
	 */
	MainWindow();

	//=========================================================================
	// オーバーライドする仮想関数
	//=========================================================================

	/**
	 * @brief set_timeoutで設定された間隔ごとに呼び出されるタイマーハンドラ。
	 * @return bool trueを返すとタイマーが継続し、falseを返すと停止します。
	 */
	bool on_timeout() override;

private:
	//=========================================================================
	// プライベートメンバ関数 (ヘルパー関数)
	//=========================================================================

	/**
	 * @brief モード切替トグルボタンの状態を排他的に更新します。
	 * @param active_button アクティブにするボタンのポインタ。
	 * @details 指定されたボタンをオンにし、他の2つのボタンをオフにします。
	 * この関数内での状態変更が、再びtoggledイベントを連鎖的に発生させないよう制御します。
	 */
	void update_toggle_buttons(tool_io::ToggleButton* active_button);

	/**
	 * @brief パスワード認証ダイアログを表示し、認証を行います。
	 * @return bool 認証に成功した場合はtrue、失敗またはキャンセルの場合はfalseを返します。
	 */
	bool authenticate_with_password();

	//=========================================================================
	// プライベートメンバ関数 (イベントハンドラ)
	//=========================================================================
	void retrieveToggleButton_on_toggled();
	void storeToggleButton_on_toggled();
	void reconcileToggleButton_on_toggled();
	void logoutButton_on_clicked();
	void prevButton_on_clicked();
	void nextButton_on_clicked();

	//=========================================================================
	// プライベートメンバ変数 (状態管理用)
	//=========================================================================

	//! プログラムが内部的にトグルボタンの状態を変更しているかを示すフラグ。
	//! trueの間、toggledイベントハンドラは何もしません。
	bool m_isProgrammaticallyChangingToggle;
};