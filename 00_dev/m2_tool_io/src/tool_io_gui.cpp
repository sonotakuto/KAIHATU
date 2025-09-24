/**
 * @file tool_io_gui.cpp
 * @brief tool_io_gui.hppで宣言されたクラスのメンバ関数の実装ファイル。
 */
#include "./includes.hpp"
#include "./SimpleGtk_extension.hpp"

typedef enum{
	retrieve,
	store,
	reconcile
} t_gui_mode;

static t_gui_mode gui_mode = retrieve;

/**
 * MainWindowのコンストラクタ実装
 */
MainWindow::MainWindow() :
	// --- メンバ初期化子リスト ---
	// Gladeオブジェクトを初期化
	glade("tool_io.glade"),
	// ヘッダー部
	dateTimeLabel(glade, "dateTimeLabel"),
	retrieveToggleButton(glade, "retrieveToggleButton"),
	storeToggleButton(glade, "storeToggleButton"),
	reconcileToggleButton(glade, "reconcileToggleButton"),
	kanaKanjiButton(glade, "kanaKanjiButton"),
	logoutButton(glade, "logoutButton"),
	userIdEntry(glade, "userIdEntry"),
	// 中央操作部
	messageLabel(glade, "messageLabel"),
	operationStack(glade, "operationStack"),
	lineEntry(glade, "lineEntry"),
	programEntry(glade, "programEntry"),
	storingToolTable(glade, "storingToolTable"), // ★ 初期化を追加
	filterButton(glade, "filterButton"),
	containerEntry(glade, "containerEntry"),
	// テーブル部
	table_stack(glade, "table_stack"),
	table3column1st(glade, "table3column1st"),
	table3column2nd(glade, "table3column2nd"),
	table3column3rd(glade, "table3column3rd"),
	table2column1st(glade, "table2column1st"),
	table2column2nd(glade, "table2column2nd"),
	// フッター部
	paginationBox(glade, "paginationBox"),
	prevButton(glade, "prevButton"),
	nextButton(glade, "nextButton"),
	pageNumberLabel(glade, "pageNumberLabel"),
	trueButton(glade, "trueButton"),
	falseButton(glade, "falseButton"),
	// ダイアログの親
	confirmInsertCountDialog(glade, "confirmInsertCountDialog"),
	confirmRegrindDialog(glade, "confirmRegrindDialog"),
	logoutDialog(glade, "logoutDialog"),
	passwdEntryDialog(glade, "passwdEntryDialog"),
	warningDialog(glade, "warningDialog"),
	// ダイアログ内のウィジェット
	confirmInsertCountLabel(glade, "confirmInsertCountLabel"),
	incrementButton(glade, "incrementButton"),
	decrementButton(glade, "decrementButton"),
	countEntry(glade, "countEntry"),
	insertCountOkButton(glade, "insertCountOkButton"),
	notRegrindButton(glade, "notRegrindButton"),
	regrindButton(glade, "regrindButton"),
	confirmRegrindOkButton(glade, "confirmRegrindOkButton"),
	logoutNoButton(glade, "logoutNoButton"),
	logoutYesButton(glade, "logoutYesButton"),
	cancelBtn(glade, "cancelBtn"),
	enterBtn(glade, "enterBtn"),
	passwordEntry(glade, "passwordEntry"),
	warningLabel(glade, "warningLabel")
{
	// --- コンストラクタ本体 ---

	// 1. Gladeファイルの "storeWindow" の定義からウィンドウを初期化
	this->init(glade, "storeWindow");
	// 2. ウィンドウの閉じるボタンでアプリケーションが終了するように設定
	this->setDefaultCloseOperation(Window::DESTROY);

	this->set_timeout(100);

	//=========================================================================
	// 初期状態の設定
	//=========================================================================
	update_toggle_buttons(&retrieveToggleButton);
	operationStack.set_visible_child_name("retrieveOperationPage");

	//=========================================================================
	// イベントハンドラの登録
	//=========================================================================
	retrieveToggleButton.on_toggled([this]() { this->retrieveToggleButton_on_toggled(); });
	storeToggleButton.on_toggled([this]() { this->storeToggleButton_on_toggled(); });
	reconcileToggleButton.on_toggled([this]() { this->reconcileToggleButton_on_toggled(); });
	logoutButton.on_clicked([this]() { this->logoutButton_on_clicked(); });
	prevButton.on_clicked([this]() { this->prevButton_on_clicked(); });
	nextButton.on_clicked([this]() { this->nextButton_on_clicked(); });
}

bool MainWindow::on_timeout() {
	// 日時ラベルを更新
	system_clock::time_point t = now();
	string str = format("{:%Y / %m / %d   %H : %M}", t);
	dateTimeLabel.set_text(str.c_str());
	return true; // タイマーを継続
}


//=============================================================================
// イベントハンドラ実装
//=============================================================================

/**
 * @brief retrieveToggleButton の "toggled" シグナルに対応するハンドラ。
 */
void MainWindow::retrieveToggleButton_on_toggled() {
	if (m_isProgrammaticallyChangingToggle) return; // プログラムによる変更は無視

	if (retrieveToggleButton.get_active()) {
		// ボタンがオンになった場合、排他制御を実行
		update_toggle_buttons(&retrieveToggleButton);
		operationStack.set_visible_child_name("retrieveOperationPage");
	} else {
		// ユーザーがアクティブなボタンを直接オフにしようとした場合、それを防ぐ
		update_toggle_buttons(&retrieveToggleButton);
	}
}

/**
 * @brief storeToggleButton の "toggled" シグナルに対応するハンドラ。
 */
void MainWindow::storeToggleButton_on_toggled() {
	if (m_isProgrammaticallyChangingToggle) return; // プログラムによる変更は無視

	if (storeToggleButton.get_active()) {
		// ボタンがオンになった場合、パスワード認証を実行
		if (authenticate_with_password()) {
			// 認証成功：排他制御を実行し、画面を切り替え
			update_toggle_buttons(&storeToggleButton);
			operationStack.set_visible_child_name("storeOperationPage");
		} else {
			// 認証失敗/キャンセル：「取り出し」モードに戻す
			update_toggle_buttons(&retrieveToggleButton);
		}
	} else {
		// ユーザーがアクティブなボタンを直接オフにしようとした場合、それを防ぐ
		update_toggle_buttons(&storeToggleButton);
	}
}

/**
 * @brief reconcileToggleButton の "toggled" シグナルに対応するハンドラ。
 */
void MainWindow::reconcileToggleButton_on_toggled() {
	if (m_isProgrammaticallyChangingToggle) return; // プログラムによる変更は無視

	if (reconcileToggleButton.get_active()) {
			// ボタンがオンになった場合、パスワード認証を実行
		if (authenticate_with_password()) {
			// 認証成功：排他制御を実行し、画面を切り替え
			update_toggle_buttons(&reconcileToggleButton);
			operationStack.set_visible_child_name("reconcileOperationPage");
		} else {
			// 認証失敗/キャンセル：「取り出し」モードに戻す
			update_toggle_buttons(&retrieveToggleButton);
		}
	} else {
		// ユーザーがアクティブなボタンを直接オフにしようとした場合、それを防ぐ
		update_toggle_buttons(&reconcileToggleButton);
	}
}

/**
 * @brief logoutButton の "clicked" シグナルに対応するハンドラ。
 */
void MainWindow::logoutButton_on_clicked() {
	fmt::print("ログアウトボタンがクリックされました\n");
	if (logoutDialog.run(this) == GTK_RESPONSE_YES) {
		fmt::print("ログアウトします\n");
		// TODO: 実際のログアウト処理を記述
	} else {
		fmt::print("ログアウトはキャンセルされました\n");
	}
}

/**
 * @brief prevButton の "clicked" シグナルに対応するハンドラ。
 */
void MainWindow::prevButton_on_clicked() {
	fmt::print("前のページへ\n");
	// TODO: テーブルのページを更新する処理
}

/**
 * @brief nextButton の "clicked" シグナルに対応するハンドラ。
 */
void MainWindow::nextButton_on_clicked() {
	fmt::print("次のページへ\n");
	// TODO: テーブルのページを更新する処理
}


//=============================================================================
// ヘルパー関数実装
//=============================================================================

/**
 * @brief パスワード認証ダイアログのヘルパー関数
 */
bool MainWindow::authenticate_with_password() {
	// ダイアログ表示前にパスワード欄をクリア
	passwordEntry.set_text("");

	// run()でダイアログをモーダル表示し、ユーザーの応答を待つ
	int response = passwdEntryDialog.run(this);

	// ダイアログを非表示にする
	passwdEntryDialog.hide();

	if (response == GTK_RESPONSE_OK) {
		// TODO: ここで実際のパスワード検証ロジックを実装します
		// 今回はダミーとして "1234" を正しいパスワードとします
		if (std::string(passwordEntry.get_text()) == "1234") {
			fmt::print("認証成功\n");
			return true;
		} else {
			fmt::print("認証失敗：パスワードが違います\n");
			// TODO: エラーメッセージをユーザーに表示する
			return false;
		}
	}

	// キャンセルボタンが押されたか、ダイアログが閉じられた場合
	fmt::print("認証キャンセル\n");
	return false;
}

/**
 * @brief モード切替トグルボタンの排他制御ヘルパー関数
 */
void MainWindow::update_toggle_buttons(tool_io::ToggleButton* active_button) {
	// これからプログラムで状態を変更することをフラグで示す
	m_isProgrammaticallyChangingToggle = true;

	// 全てのボタンの状態を一度更新する
	// active_buttonだけをtrueに、他はfalseにする
	retrieveToggleButton.set_active(&retrieveToggleButton == active_button);
	storeToggleButton.set_active(&storeToggleButton == active_button);
	reconcileToggleButton.set_active(&reconcileToggleButton == active_button);

	// プログラムによる変更が完了したことをフラグで示す
	m_isProgrammaticallyChangingToggle = false;
}
