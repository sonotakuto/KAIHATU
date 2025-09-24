/**
 * @file tool_io_gui.cpp
 * @brief tool_io_gui.hppで宣言されたクラスのメンバ関数の実装ファイル。
 */
#include "./includes.hpp"
#include "./SimpleGtk_extension.hpp"

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
	set_mode(RETRIEVE);

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
	// このボタンに対する操作は、常に「取り出し」モードへの設定を試みる
	// これにより、ユーザーがアクティブなこのボタンを押してオフにしようとしても、
	// set_modeが再度このボタンをオンにし、状態が維持される
	set_mode(RETRIEVE);
}

/**
 * @brief storeToggleButton の "toggled" シグナルに対応するハンドラ。
 */
void MainWindow::storeToggleButton_on_toggled() {
	if (storeToggleButton.get_active()) {
		// ボタンがオンにされた場合：パスワード認証を行う
		if (authenticate_with_password()) {
			set_mode(STORE);
		} else {
			// 認証失敗/キャンセル：現在のモード(m_gui_mode)に戻す
			set_mode(m_gui_mode);
		}
	} else {
		// ボタンがオフにされた場合：現在のモードを再設定してオフ化を防ぐ
		set_mode(m_gui_mode);
	}
}

/**
 * @brief reconcileToggleButton の "toggled" シグナルに対応するハンドラ。
 */
void MainWindow::reconcileToggleButton_on_toggled() {
	if (reconcileToggleButton.get_active()) {
		// ボタンがオンにされた場合：パスワード認証を行う
		if (authenticate_with_password()) {
			set_mode(RECONCILE);
		} else {
			// 認証失敗/キャンセル：現在のモード(m_gui_mode)に戻す
			set_mode(m_gui_mode);
		}
	} else {
		// ボタンがオフにされた場合：現在のモードを再設定してオフ化を防ぐ
		set_mode(m_gui_mode);
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
 * @brief アプリケーションの操作モードを設定するヘルパー関数
 */
void MainWindow::set_mode(GuiMode mode) {
	// 1. メンバ変数を更新
	m_gui_mode = mode;

	// 2. トグルボタンの状態を排他的に更新
	//    プログラムによる変更なので、イベントが連鎖しないように注意は不要
	retrieveToggleButton.set_active(mode == RETRIEVE);
	storeToggleButton.set_active(mode == STORE);
	reconcileToggleButton.set_active(mode == RECONCILE);

	// 3. モードに応じて表示する操作パネルを切り替え
	switch (mode) {
		case RETRIEVE:
			operationStack.set_visible_child_name("retrieveOperationPage");
			break;
		case STORE:
			operationStack.set_visible_child_name("storeOperationPage");
			break;
		case RECONCILE:
			operationStack.set_visible_child_name("reconcileOperationPage");
			break;
	}
}

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