/**
 * @file tool_io_gui.cpp
 * @brief tool_io_gui.hppで宣言されたクラスのメンバ関数の実装ファイル。
 */
#include "./includes.hpp"
#include "./SimpleGtk_extension.hpp"


//=============================================================================
// 静的メンバ変数の定義と初期化
//=============================================================================

const std::map<MainWindow::AppState, MainWindow::UiStateConfig> MainWindow::m_ui_state_map = {
	// --- 状態：ユーザーログイン待ち ---
	{ AppState::WAITING_FOR_USER_LOGIN, {
		/* Text */
		{ "名札のQRコードを\nQRコードリーダーにかざしてください", "なふだの QRコードを\nQRコードリーダーに かざしてください" },
		{ "", "" }, false,
		{ "", "" }, false,
		/* Pages */
		"retrieveOperationPage", "table3column", // 初期ページ
		/* Callbacks */
		[](MainWindow* thiz){ gtk_widget_grab_focus(GTK_WIDGET(thiz->passwordEntry.object)); },
		nullptr, // Trueボタンアクション
		nullptr  // Falseボタンアクション
	}},

	// --- 状態：取り出し - プログラムQR待ち ---
	{ AppState::RETRIEVE_WAITING_FOR_PROGRAM_QR, {
		/* Text */
		{ "プログラム割当一覧表のQRコードを\nQRコードリーダーにかざしてください", "プログラムわりあていちらんひょうの QRコードを..." },
		{ "", "" }, false,
		{ "", "" }, false,
		/* Pages */
		"retrieveOperationPage", "table3column",
		/* Callbacks */
		[](MainWindow* thiz){ /* TODO: テーブルをクリアするなどの初期化処理 */ },
		nullptr,
		nullptr
	}},

	// --- 状態：取り出し - 工具選択中 ---
	{ AppState::RETRIEVE_SELECTING_TOOLS, {
		/* Text */
		{ "取り出す工具を選択してください", "とりだす こうぐを せんたく してください" },
		{ "確定", "かくてい" }, true,
		{ "全選択解除", "ぜんせんたくかいじょ" }, true,
		/* Pages */
		"retrieveOperationPage", "table3column",
		/* Callbacks */
		[](MainWindow* thiz){ /* TODO: DBから取得した工具リストをテーブルに表示する */ },
		[](MainWindow* thiz){ thiz->set_app_state(AppState::RETRIEVE_CONFIRMING_SELECTION); },
		[](MainWindow* thiz){ /* TODO: テーブルの全選択を解除する処理 */ }
	}},

	// --- 状態：取り出し - 選択内容の確認中 ---
	{ AppState::RETRIEVE_CONFIRMING_SELECTION, {
		/* Text */
		{ "選択した工具は以下の通りです\nよろしければ「搬出」を押してください", "せんたくした こうぐは いかのとおりです..." },
		{ "搬出", "はんしゅつ" }, true,
		{ "キャンセル", "きゃんせる" }, true,
		/* Pages */
		"retrieveOperationPage", "table2column",
		/* Callbacks */
		[](MainWindow* thiz){ /* TODO: 選択された工具のみをテーブルに表示する */ },
		[](MainWindow* thiz){ /* TODO: 搬出処理を開始し、次の状態へ */ },
		[](MainWindow* thiz){ thiz->set_app_state(AppState::RETRIEVE_SELECTING_TOOLS); } // 前の状態に戻る
	}},

	// TODO: 他のすべての状態(RETRIEVE_WAITING_FOR_CONTAINERなど)をここに追加していく
};

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
	storingToolTable(glade, "storingToolTable"),
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
	passwordEntryDialog(glade, "passwordEntryDialog"),
	warningDialog(glade, "warningDialog"),
	// インサート枚数確認ダイアログ内
	confirmInsertCountLabel(glade, "confirmInsertCountLabel"),
	insertCountIncrementButton(glade, "insertCountIncrementButton"),
	insertCountDecrementButton(glade, "insertCountDecrementButton"),
	insertCountEntry(glade, "insertCountEntry"),
	insertCountOkButton(glade, "insertCountOkButton"),
	// 再研磨確認ダイアログ内
	confirmRegrindLabel(glade, "confirmRegrindLabel"),
	notRegrindButton(glade, "notRegrindButton"),
	regrindButton(glade, "regrindButton"),
	confirmRegrindOkButton(glade, "confirmRegrindOkButton"),
	// ログアウト確認ダイアログ内
	logoutNoButton(glade, "logoutNoButton"),
	logoutYesButton(glade, "logoutYesButton"),
	// パスワード入力ダイアログ内
	passwordEntryLabel(glade, "passwordEntryLabel"),
	passwordCancelButton(glade, "passwordCancelButton"),
	passwordEnterButton(glade, "passwordEnterButton"),
	passwordEntry(glade, "passwordEntry"),
	// 警告ダイアログ内
	warningLabel(glade, "warningLabel"),
	// 棚卸し絞り込みダイアログ内
	reconcileToolTypeFilterLabel(glade, "reconcileToolTypeFilterLabel"),
	reconcilePeriodFilterLabel(glade, "reconcilePeriodFilterLabel"),
	reconcileFilterEnterButton(glade, "reconcileFilterEnterButton"),
	reconcileFilterResetButton(glade, "reconcileFilterResetButton"),
	reconcileToolTypeFilterComboBox(glade, "reconcileToolTypeFilterComboBox"),
	reconcilePeriodFilterSwitch(glade, "reconcilePeriodFilterSwitch"),
	reconcileFilterStartYearIncrementButton(glade, "reconcileFilterStartYearIncrementButton"),
	reconcileFilterStartYearEntry(glade, "reconcileFilterStartYearEntry"),
	reconcileFilterStartYearDecrementButton(glade, "reconcileFilterStartYearDecrementButton"),
	reconcileFilterStartMonthIncrementButton(glade, "reconcileFilterStartMonthIncrementButton"),
	reconcileFilterStartMonthEntry(glade, "reconcileFilterStartMonthEntry"),
	reconcileFilterStartMonthDecrementButton(glade, "reconcileFilterStartMonthDecrementButton"),
	reconcileFilterEndYearIncrementButton(glade, "reconcileFilterEndYearIncrementButton"),
	reconcileFilterEndYearEntry(glade, "reconcileFilterEndYearEntry"),
	reconcileFilterEndYearDecrementButton(glade, "reconcileFilterEndYearDecrementButton"),
	reconcileFilterEndMonthIncrementButton(glade, "reconcileFilterEndMonthIncrementButton"),
	reconcileFilterEndMonthEntry(glade, "reconcileFilterEndMonthEntry"),
	reconcileFilterEndMonthDecrementButton(glade, "reconcileFilterEndMonthDecrementButton"),
	// 状態管理メンバ
	m_isProgrammaticallyChangingToggle(false),
	m_gui_mode(GuiMode::RETRIEVE),
	m_app_state(AppState::WAITING_FOR_USER_LOGIN),
	m_text_mode(TextMode::KANJI)
{
	// --- コンストラクタ本体 ---

	// 1. Gladeファイルの "storeWindow" の定義からウィンドウを初期化
	this->init(glade, "storeWindow");
	// 2. ウィンドウの閉じるボタンでアプリケーションが終了するように設定
	this->setDefaultCloseOperation(Window::DESTROY);

	this->set_timeout(100);

	//=========================================================================
	// イベントハンドラの登録
	//=========================================================================
	retrieveToggleButton.on_toggled([this]() { this->retrieveToggleButton_on_toggled(); });
	storeToggleButton.on_toggled([this]() { this->storeToggleButton_on_toggled(); });
	reconcileToggleButton.on_toggled([this]() { this->reconcileToggleButton_on_toggled(); });
	kanaKanjiButton.on_clicked([this]() { this->kanaKanjiButton_on_clicked(); });
	logoutButton.on_clicked([this]() { this->logoutButton_on_clicked(); });
	prevButton.on_clicked([this]() { this->prevButton_on_clicked(); });
	nextButton.on_clicked([this]() { this->nextButton_on_clicked(); });
	trueButton.on_clicked([this]() { this->trueButton_on_clicked(); });
	falseButton.on_clicked([this]() { this->falseButton_on_clicked(); });
	passwordEntryDialog.on_show([this]() { this->passwordEntryDialog_on_show(); });
	passwordCancelButton.on_clicked([this]() { this->passwordCancelButton_on_clicked(); });
	passwordEnterButton.on_clicked([this]() { this->passwordEnterButton_on_clicked(); });
	passwordEntry.on_activate([this]() { this->passwordEnterButton_on_clicked(); });	// 確定ボタンがクリックされた時と同じ処理を呼び出す

	//=========================================================================
	// 静的テキスト更新用辞書の構築
	//=========================================================================
	m_static_text_updaters[&retrieveToggleButton] = {
		"取り出し", "とりだし",
		[this](const char* text){ this->retrieveToggleButton.set_label(text); }
	};
	m_static_text_updaters[&storeToggleButton] = {
		"補充", "ほじゅう",
		[this](const char* text){ this->storeToggleButton.set_label(text); }
	};
	m_static_text_updaters[&reconcileToggleButton] = {
		"棚卸し", "たなおろし",
		[this](const char* text){ this->reconcileToggleButton.set_label(text); }
	};
	// TODO: 他のかな/漢字切替が必要なウィジェットも同様に追加

	//=========================================================================
	// 初期状態の設定
	//=========================================================================
	set_mode(GuiMode::RETRIEVE);
	set_app_state(AppState::WAITING_FOR_USER_LOGIN);
}

//=============================================================================
// 状態遷移関数
//=============================================================================

void MainWindow::set_app_state(AppState state) {
	// 1. メンバ変数の状態を更新
	m_app_state = state;

	// 2. mapから現在の状態に対応するUI設定を検索
	auto it = m_ui_state_map.find(state);
	if (it == m_ui_state_map.end()) {
		messageLabel.set_text("エラー: 未定義の状態です");
		trueButton.set_sensitive(false);
		falseButton.set_sensitive(false);
		return;
	}
	const UiStateConfig& config = it->second;

	// 3. テキストモードに応じて表示文字列を選択し、ウィジェットに設定
	if (m_text_mode == TextMode::KANA) {
		messageLabel.set_text(config.message.kana.c_str());
		trueButton.set_label(config.trueButtonText.kana.c_str());
		falseButton.set_label(config.falseButtonText.kana.c_str());
	} else {
		messageLabel.set_text(config.message.kanji.c_str());
		trueButton.set_label(config.trueButtonText.kanji.c_str());
		falseButton.set_label(config.falseButtonText.kanji.c_str());
	}

	// 4. ボタンの表示/非表示を設定
	trueButton.set_sensitive(config.isTrueButtonSensitive);
	falseButton.set_sensitive(config.isFalseButtonSensitive);

	// 5. 表示するページを切り替え
	if (!config.operationStackPageName.empty()) {
		operationStack.set_visible_child_name(config.operationStackPageName.c_str());
	}
	if (!config.tableStackPageName.empty()) {
		table_stack.set_visible_child_name(config.tableStackPageName.c_str());
	}

	// 6. 状態遷移直後に実行するアクションがあれば実行
	if (config.on_enter_state) {
		config.on_enter_state(this);
	}
}

//=============================================================================
// ヘルパー関数実装
//=============================================================================

void MainWindow::set_mode(GuiMode mode) {
	// 1. メンバ変数を更新
	m_gui_mode = mode;

	// 2. トグルボタンの状態を排他的に更新
	m_isProgrammaticallyChangingToggle = true;
	retrieveToggleButton.set_active(mode == GuiMode::RETRIEVE);
	storeToggleButton.set_active(mode == GuiMode::STORE);
	reconcileToggleButton.set_active(mode == GuiMode::RECONCILE);
	m_isProgrammaticallyChangingToggle = false;

	// 3. モードに応じて表示する操作パネルを切り替え
	switch (mode) {
	case GuiMode::RETRIEVE:
		fmt::print("取り出しモード\n");
		operationStack.set_visible_child_name("retrieveOperationPage");
		operationStack.set_visible_child_name("table3column");
		break;
	case GuiMode::STORE:
		fmt::print("補充モード\n");
		operationStack.set_visible_child_name("storeOperationPage");
		operationStack.set_visible_child_name("table2column");
		break;
	case GuiMode::RECONCILE:
		fmt::print("棚卸しモード\n");
		operationStack.set_visible_child_name("reconcileOperationPage");
		operationStack.set_visible_child_name("table2column");
		break;
	}
}

bool MainWindow::authenticate_with_password() {
	// ダイアログ表示前にパスワード欄をクリア
	passwordEntry.set_text("");

	// run()でダイアログをモーダル表示し、ユーザーの応答を待つ
	GtkResponseType response = passwordEntryDialog.run(this);

	// ダイアログを非表示にする
	passwordEntryDialog.hide();

	if (response == GTK_RESPONSE_OK) {
		// TODO: ここで実際のパスワード検証ロジックを実装します
		// 今回はダミーとして "1234" を正しいパスワードとします
		fmt::print("認証開始\n");
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

void MainWindow::refresh_static_texts() {
	// 辞書をループし、登録された各ウィジェットのテキストを更新する
	for (const auto& pair : m_static_text_updaters) {
		// C++17 構造化束縛: pairからキーと値を直接取り出す
		const auto& [widget, updater] = pair;

		// 現在のテキストモードに応じて表示する文字列を選択
		const char* text_to_set = (m_text_mode == TextMode::KANA) ?
								updater.text.kana.c_str() :
								updater.text.kanji.c_str();

		// 登録されているセッター関数を呼び出して、テキストを設定
		if (updater.setter) {
			updater.setter(text_to_set);
		}
	}
}

//=============================================================================
// イベントハンドラ実装
//=============================================================================

bool MainWindow::on_timeout() {
	// 日時ラベルを更新
	system_clock::time_point t = now();
	string str = format("{:%Y / %m / %d   %H : %M}", t);
	dateTimeLabel.set_text(str.c_str());
	return true; // タイマーを継続
}

void MainWindow::retrieveToggleButton_on_toggled() {
	fmt::print("[取り出し]\n");
	if (m_isProgrammaticallyChangingToggle) {
		fmt::print("無視\n");
		return;	// プログラムによる変更は無視
	}

	// このボタンに対する操作は、常に「取り出し」モードへの設定を試みる
	// これにより、ユーザーがアクティブなこのボタンを押してオフにしようとしても、
	// set_modeが再度このボタンをオンにし、状態が維持される
	set_mode(GuiMode::RETRIEVE);
}

void MainWindow::storeToggleButton_on_toggled() {
	fmt::print("[補充]\n");
	if (m_isProgrammaticallyChangingToggle) {
		fmt::print("無視\n");
		return;	// プログラムによる変更は無視
	}

	if (storeToggleButton.get_active()) {
		// ボタンがオンにされた場合：パスワード認証を行う
		if (authenticate_with_password()) {
			set_mode(GuiMode::STORE);
		} else {
			// 認証失敗/キャンセル：現在のモード(m_gui_mode)に戻す
			set_mode(m_gui_mode);
		}
	} else {
		// ボタンがオフにされた場合：現在のモードを再設定してオフ化を防ぐ
		set_mode(m_gui_mode);
	}
}

void MainWindow::reconcileToggleButton_on_toggled() {
	fmt::print("[棚卸し]\n");
	if (m_isProgrammaticallyChangingToggle) {
		fmt::print("無視\n");
		return;	// プログラムによる変更は無視
	}

	if (reconcileToggleButton.get_active()) {
		// ボタンがオンにされた場合：パスワード認証を行う
		if (authenticate_with_password()) {
			set_mode(GuiMode::RECONCILE);
		} else {
			// 認証失敗/キャンセル：現在のモード(m_gui_mode)に戻す
			set_mode(m_gui_mode);
		}
	} else {
		// ボタンがオフにされた場合：現在のモードを再設定してオフ化を防ぐ
		set_mode(m_gui_mode);
	}
}

void MainWindow::kanaKanjiButton_on_clicked() {
	m_text_mode = (m_text_mode == TextMode::KANJI) ? TextMode::KANA : TextMode::KANJI;
	set_app_state(m_app_state); // 現在の状態のままUIテキストを更新
}

void MainWindow::logoutButton_on_clicked() {
	fmt::print("ログアウトボタンがクリックされました\n");
	if (logoutDialog.run(this) == GTK_RESPONSE_YES) {
		fmt::print("ログアウトします\n");
		// TODO: 実際のログアウト処理を記述
	} else {
		fmt::print("ログアウトはキャンセルされました\n");
	}
}

void MainWindow::prevButton_on_clicked() {
	fmt::print("前のページへ\n");
	// TODO: テーブルのページを更新する処理
}

void MainWindow::nextButton_on_clicked() {
	fmt::print("次のページへ\n");
	// TODO: テーブルのページを更新する処理
}

void MainWindow::trueButton_on_clicked() {
	auto it = m_ui_state_map.find(m_app_state);
	if (it != m_ui_state_map.end() && it->second.on_true_button_clicked) {
		it->second.on_true_button_clicked(this);
	}
}

void MainWindow::falseButton_on_clicked() {
	auto it = m_ui_state_map.find(m_app_state);
	if (it != m_ui_state_map.end() && it->second.on_false_button_clicked) {
		it->second.on_false_button_clicked(this);
	}
}

void MainWindow::passwordEntryDialog_on_show() {
	// Gtkウィジェットにキーボードフォーカスを強制的に設定する
	gtk_widget_grab_focus(GTK_WIDGET(passwordEntry.object));
}

void MainWindow::passwordCancelButton_on_clicked() {
	// passwdEntryDialogに対して、GTK_RESPONSE_OK (-5) の応答シグナルを送信する
	// これにより、dialog.run()が GTK_RESPONSE_OK を返して終了する
	fmt::print("[キャンセル]\n");
	gtk_dialog_response(GTK_DIALOG(this->passwordEntryDialog.object), GTK_RESPONSE_CANCEL);
}

void MainWindow::passwordEnterButton_on_clicked() {
	// passwdEntryDialogに対して、GTK_RESPONSE_CANCEL (-6) の応答シグナルを送信する
	// これにより、dialog.run()が GTK_RESPONSE_CANCEL を返して終了する
	fmt::print("[確定]\n");
	gtk_dialog_response(GTK_DIALOG(this->passwordEntryDialog.object), GTK_RESPONSE_OK);
}
