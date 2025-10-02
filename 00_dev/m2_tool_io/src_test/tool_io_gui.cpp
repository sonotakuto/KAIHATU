/**
 * @file tool_io_gui.cpp
 * @brief tool_io_gui.hppで宣言されたクラスのメンバ関数の実装ファイル。
 */
#include "./includes.hpp"
#include "./SimpleGtk_extension.hpp"

//=============================================================================
// MainWindowのメンバクラスに関する関数
//=============================================================================

// --- UiText ---

bool operator==(const MainWindow::UiText& lhs, const MainWindow::UiText& rhs) {
	// 漢字とかなの両方のテキストが一致する場合にtrueを返す
	return lhs.kanji == rhs.kanji && lhs.kana == rhs.kana;
}

bool operator!=(const MainWindow::UiText& lhs, const MainWindow::UiText& rhs) {
	return !(lhs == rhs);
}

// --- WidgetState ---

bool operator==(const MainWindow::WidgetState& lhs, const MainWindow::WidgetState& rhs) {
	// 全てのoptionalメンバーが一致する場合にtrueを返す
	return lhs.text == rhs.text &&
			lhs.is_sensitive == rhs.is_sensitive &&
			lhs.is_visible == rhs.is_visible;
}

bool operator!=(const MainWindow::WidgetState& lhs, const MainWindow::WidgetState& rhs) {
	return !(lhs == rhs);
}

bool MainWindow::WidgetState::merge(const WidgetState& other) {
	bool changed = false;
	if (*this != other) {
		if (other.text.has_value()) {
			this->text = other.text;
		}
		if (other.is_sensitive.has_value()) {
			this->is_sensitive = other.is_sensitive;
		}
		if (other.is_visible.has_value()) {
			this->is_visible = other.is_visible;
		}
		changed = true;
	}
	return changed;
}

// --- TableState ---

bool operator==(const MainWindow::TableState& lhs, const MainWindow::TableState& rhs) {
	// 全てのoptionalメンバーが一致する場合にtrueを返す
	return lhs.column_headers == rhs.column_headers &&
			lhs.is_sensitive == rhs.is_sensitive &&
			lhs.is_visible == rhs.is_visible;
}

bool operator!=(const MainWindow::TableState& lhs, const MainWindow::TableState& rhs) {
	return !(lhs == rhs);
}

bool MainWindow::TableState::merge(const TableState& other) {
	bool changed = false;
	if (*this != other) {
		if (other.column_headers.has_value()) {
			this->column_headers = other.column_headers;
		}
		if (other.is_sensitive.has_value()) {
			this->is_sensitive = other.is_sensitive;
		}
		if (other.is_visible.has_value()) {
			this->is_visible = other.is_visible;
		}
		changed = true;
	}
	return changed;
}

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
	lineLabel(glade, "lineLabel"),
	lineEntry(glade, "lineEntry"),
	programLabel(glade, "programLabel"),
	programEntry(glade, "programEntry"),
	storingToolTable(glade, "storingToolTable"),
	filterButton(glade, "filterButton"),
	containerLabel(glade, "containerLabel"),
	containerEntry(glade, "containerEntry"),
	// テーブル部
	table_stack(glade, "table_stack"),
	table3column1stBox(glade, "table3column1stBox"),
	table3column1st(glade, "table3column1st"),
	table3column2ndBox(glade, "table3column2ndBox"),
	table3column2nd(glade, "table3column2nd"),
	table3column3rdBox(glade, "table3column3rdBox"),
	table3column3rd(glade, "table3column3rd"),
	table2column1stBox(glade, "table2column1stBox"),
	table2column1st(glade, "table2column1st"),
	table2column2ndBox(glade, "table2column2ndBox"),
	table2column2nd(glade, "table2column2nd"),
	// フッター部
	paginationBox(glade, "paginationBox"),
	prevButton(glade, "prevButton"),
	nextButton(glade, "nextButton"),
	pageNumberLabel(glade, "pageNumberLabel"),
	qrCodeEntryBox(glade, "qrCodeEntryBox"),
	qrCodeEntryLabel(glade, "qrCodeEntryLabel"),
	qrCodeEntry(glade, "qrCodeEntry"),
	falseButton(glade, "falseButton"),
	trueButton(glade, "trueButton"),
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
	m_is_first_show(true),
	m_isProgrammaticallyChangingToggle(false),
	m_gui_mode(GuiMode::RETRIEVE),
	m_app_state(AppState::WAITING_FOR_USER_LOGIN),
	m_text_mode(TextMode::KANJI)
{
	// --- コンストラクタ本体 ---

	// 1. Gladeファイルの "storeWindow" の定義からウィンドウを初期化
	this->init(glade, "storeWindow");
	storingToolListstore = storingToolTable.get_model();
	liststore2column1st = table2column1st.get_model();
	liststore2column2nd = table2column2nd.get_model();

	// 2. ウィンドウの閉じるボタンでアプリケーションが終了するように設定
	this->setDefaultCloseOperation(Window::DESTROY);

	this->set_timeout(100);

	//=========================================================================
	// イベントハンドラの登録
	//=========================================================================
	this->on_show([this](){ this->on_window_show(); });
	retrieveToggleButton.on_toggled([this]() { this->retrieveToggleButton_on_toggled(); });
	storeToggleButton.on_toggled([this]() { this->storeToggleButton_on_toggled(); });
	reconcileToggleButton.on_toggled([this]() { this->reconcileToggleButton_on_toggled(); });
	kanaKanjiButton.on_clicked([this]() { this->kanaKanjiButton_on_clicked(); });
	logoutButton.on_clicked([this]() { this->logoutButton_on_clicked(); });
	userIdEntry.on_activate([this]() { this->userIdEntry_on_activate(); });
	prevButton.on_clicked([this]() { this->prevButton_on_clicked(); });
	nextButton.on_clicked([this]() { this->nextButton_on_clicked(); });
	qrCodeEntry.on_activate([this]() { this->qrCodeEntry_on_activate(); });
	falseButton.on_clicked([this]() { this->falseButton_on_clicked(); });
	trueButton.on_clicked([this]() { this->trueButton_on_clicked(); });
	logoutNoButton.on_clicked([this]() { this->logoutNoButton_on_clicked(); });
	logoutYesButton.on_clicked([this]() { this->logoutYesButton_on_clicked(); });
	passwordEntryDialog.on_show([this]() { this->passwordEntryDialog_on_show(); });
	passwordCancelButton.on_clicked([this]() { this->passwordCancelButton_on_clicked(); });
	passwordEnterButton.on_clicked([this]() { this->passwordEnterButton_on_clicked(); });
	passwordEntry.on_activate([this]() { this->passwordEnterButton_on_clicked(); });	// 確定ボタンがクリックされた時と同じ処理を呼び出す

	//=========================================================================
	// ウィジェットの状態を保持するマップの構築
	//=========================================================================

	// --- 「取り出し」トグルボタン ---
	m_widget_states.insert(make_pair(&retrieveToggleButton, WidgetStateUpdater{
		{UiText{"取り出し", "とりだし"}, false, true},
		[this](const WidgetState& state){
			this->retrieveToggleButton.set_label(get_display_text(state.text.value()).c_str());
			this->retrieveToggleButton.set_sensitive(state.is_sensitive.value());
			this->retrieveToggleButton.set_visible(state.is_visible.value());
		}
	}));

	// --- 「補充」トグルボタン ---
	m_widget_states.insert(make_pair(&storeToggleButton, WidgetStateUpdater{
		{UiText{"補充", "ほじゅう"}, false, true},
		[this](const WidgetState& state){
			this->storeToggleButton.set_label(get_display_text(state.text.value()).c_str());
			this->storeToggleButton.set_sensitive(state.is_sensitive.value());
			this->storeToggleButton.set_visible(state.is_visible.value());
		}
	}));

	// --- 「棚卸し」トグルボタン ---
	m_widget_states.insert(make_pair(&reconcileToggleButton, WidgetStateUpdater{
		{UiText{"棚卸し", "たなおろし"}, false, true},
		[this](const WidgetState& state){
			this->reconcileToggleButton.set_label(get_display_text(state.text.value()).c_str());
			this->reconcileToggleButton.set_sensitive(state.is_sensitive.value());
			this->reconcileToggleButton.set_visible(state.is_visible.value());
		}
	}));

	// --- 「かな/漢字」ボタン ---
	m_widget_states.insert(make_pair(&kanaKanjiButton, WidgetStateUpdater{
		{UiText{"かな/漢字", "かな/漢字"}, true, true},
		[this](const WidgetState& state){
			this->kanaKanjiButton.set_label(get_display_text(state.text.value()).c_str());
			this->kanaKanjiButton.set_sensitive(state.is_sensitive.value());
			this->kanaKanjiButton.set_visible(state.is_visible.value());
		}
	}));

	// --- 「ログアウト」ボタン ---
	m_widget_states.insert(make_pair(&logoutButton, WidgetStateUpdater{
		{UiText{"ログアウト", "ログアウト"}, false, true},
		[this](const WidgetState& state){
			this->logoutButton.set_label(get_display_text(state.text.value()).c_str());
			this->logoutButton.set_sensitive(state.is_sensitive.value());
			this->logoutButton.set_visible(state.is_visible.value());
		}
	}));

	// --- ユーザーIDエントリ ---
	m_widget_states.insert(make_pair(&userIdEntry, WidgetStateUpdater{
		{UiText{"", ""}, true, true},
		[this](const WidgetState& state){
			this->userIdEntry.set_text(get_display_text(state.text.value()).c_str());
			this->userIdEntry.set_sensitive(state.is_sensitive.value());
			this->userIdEntry.set_visible(state.is_visible.value());
		}
	}));

	// --- 「ライン番号」ラベル ---
	m_widget_states.insert(make_pair(&lineLabel, WidgetStateUpdater{
		{UiText{"ライン番号：", "ラインばんごう："}, true, true},
		[this](const WidgetState& state){
			this->lineLabel.set_label(get_display_text(state.text.value()).c_str());
			// ラベルのsensitiveは通常操作しない
			this->lineLabel.set_visible(state.is_visible.value());
		}
	}));

	// --- 「プログラム番号」ラベル ---
	m_widget_states.insert(make_pair(&programLabel, WidgetStateUpdater{
		{UiText{"プログラム番号：", "プログラムばんごう："}, true, true},
		[this](const WidgetState& state){
			this->programLabel.set_label(get_display_text(state.text.value()).c_str());
			this->programLabel.set_visible(state.is_visible.value());
		}
	}));

	// --- 「絞り込み」ボタン ---
	m_widget_states.insert(make_pair(&filterButton, WidgetStateUpdater{
		{UiText{"絞り込み", "しぼりこみ"}, true, true},
		[this](const WidgetState& state){
			this->filterButton.set_label(get_display_text(state.text.value()).c_str());
			this->filterButton.set_sensitive(state.is_sensitive.value());
			this->filterButton.set_visible(state.is_visible.value());
		}
	}));

	// --- 「コンテナ番号」ラベル ---
	m_widget_states.insert(make_pair(&containerLabel, WidgetStateUpdater{
		{UiText{"コンテナ番号：", "コンテナばんごう："}, true, true},
		[this](const WidgetState& state){
			this->containerLabel.set_label(get_display_text(state.text.value()).c_str());
			this->containerLabel.set_visible(state.is_visible.value());
		}
	}));

	// --- メッセージラベル ---
	m_widget_states.insert(make_pair(&messageLabel, WidgetStateUpdater{
		{UiText{"", ""}, true, true},
		[this](const WidgetState& state){
			this->messageLabel.set_label(get_display_text(state.text.value()).c_str());
			this->messageLabel.set_visible(state.is_visible.value());
		}
	}));

	// --- 3列表示テーブルのボックス (1) ---
	m_widget_states.insert(make_pair(&table3column1stBox, WidgetStateUpdater{
		{nullopt, nullopt, false},
		[this](const WidgetState& state){
			this->table3column1stBox.set_visible(state.is_visible.value());
		}
	}));

	// --- 3列表示テーブルのボックス (2) ---
	m_widget_states.insert(make_pair(&table3column2ndBox, WidgetStateUpdater{
		{nullopt, nullopt, false},
		[this](const WidgetState& state){
			this->table3column2ndBox.set_visible(state.is_visible.value());
		}
	}));

	// --- 3列表示テーブルのボックス (3) ---
	m_widget_states.insert(make_pair(&table3column3rdBox, WidgetStateUpdater{
		{nullopt, nullopt, false},
		[this](const WidgetState& state){
			this->table3column3rdBox.set_visible(state.is_visible.value());
		}
	}));

		// --- 2列表示テーブルのボックス (1) ---
	m_widget_states.insert(make_pair(&table2column1stBox, WidgetStateUpdater{
		{nullopt, nullopt, false},
		[this](const WidgetState& state){
			this->table2column1stBox.set_visible(state.is_visible.value());
		}
	}));

	// --- 2列表示テーブルのボックス (2) ---
	m_widget_states.insert(make_pair(&table2column2ndBox, WidgetStateUpdater{
		{nullopt, nullopt, false},
		[this](const WidgetState& state){
			this->table2column2ndBox.set_visible(state.is_visible.value());
		}
	}));

	// --- QRコード受付UIのボックス ---
	m_widget_states.insert(make_pair(&qrCodeEntryBox, WidgetStateUpdater{
		{nullopt, nullopt, false},
		[this](const WidgetState& state){
			this->qrCodeEntryBox.set_visible(state.is_visible.value());
		}
	}));

	// --- 「QRコード受付」ラベル ---
	m_widget_states.insert(make_pair(&qrCodeEntryLabel, WidgetStateUpdater{
		{UiText{"QRコード受付：", "QRコード うけつけ："}, true, true},
		[this](const WidgetState& state){
			this->qrCodeEntryLabel.set_label(get_display_text(state.text.value()).c_str());
			this->qrCodeEntryLabel.set_visible(state.is_visible.value());
		}
	}));

	// --- true/falseボタン（ラベルは動的に変わるため空） ---
	m_widget_states.insert(make_pair(&falseButton, WidgetStateUpdater{
		{UiText{"", ""}, true, true},
		[this](const WidgetState& state){
			this->falseButton.set_label(get_display_text(state.text.value()).c_str());
			this->falseButton.set_sensitive(state.is_sensitive.value());
			this->falseButton.set_visible(state.is_visible.value());
		}
	}));
	m_widget_states.insert(make_pair(&trueButton, WidgetStateUpdater{
		{UiText{"", ""}, true, true},
		[this](const WidgetState& state){
			this->trueButton.set_label(get_display_text(state.text.value()).c_str());
			this->trueButton.set_sensitive(state.is_sensitive.value());
			this->trueButton.set_visible(state.is_visible.value());
		}
	}));

	// --- インサート枚数確認ダイアログ ---
	m_widget_states.insert(make_pair(&confirmInsertCountLabel, WidgetStateUpdater{
		{UiText{"使用するインサートの枚数を\n入力してください", "しようする インサートの まいすうを\nにゅうりょく してください"}, true, true},
		[this](const WidgetState& state){
			this->confirmInsertCountLabel.set_label(get_display_text(state.text.value()).c_str());
			this->confirmInsertCountLabel.set_visible(state.is_visible.value());
		}
	}));
	m_widget_states.insert(make_pair(&insertCountOkButton, WidgetStateUpdater{
		{UiText{"OK", "OK"}, true, true},
		[this](const WidgetState& state){
			this->insertCountOkButton.set_label(get_display_text(state.text.value()).c_str());
			this->insertCountOkButton.set_sensitive(state.is_sensitive.value());
			this->insertCountOkButton.set_visible(state.is_visible.value());
		}
	}));

	// --- 再研磨確認ダイアログ ---
	m_widget_states.insert(make_pair(&confirmRegrindLabel, WidgetStateUpdater{
		{UiText{"一度取り出された工具を検出しました。\n再研磨された工具ですか？", "いちど とりだされた こうぐを けんしゅつしました。\nさいけんま された こうぐですか？"}, true, true},
		[this](const WidgetState& state){
			this->confirmRegrindLabel.set_label(get_display_text(state.text.value()).c_str());
			this->confirmRegrindLabel.set_visible(state.is_visible.value());
		}
	}));
	m_widget_states.insert(make_pair(&confirmRegrindOkButton, WidgetStateUpdater{
		{UiText{"OK", "OK"}, true, true},
		[this](const WidgetState& state){
			this->confirmRegrindOkButton.set_label(get_display_text(state.text.value()).c_str());
			this->confirmRegrindOkButton.set_sensitive(state.is_sensitive.value());
			this->confirmRegrindOkButton.set_visible(state.is_visible.value());
		}
	}));
	m_widget_states.insert(make_pair(&notRegrindButton, WidgetStateUpdater{
		{UiText{"いいえ", "いいえ"}, true, true},
		[this](const WidgetState& state){
			this->notRegrindButton.set_label(get_display_text(state.text.value()).c_str());
			this->notRegrindButton.set_sensitive(state.is_sensitive.value());
			this->notRegrindButton.set_visible(state.is_visible.value());
		}
	}));
	m_widget_states.insert(make_pair(&regrindButton, WidgetStateUpdater{
		{UiText{"はい", "はい"}, true, true},
		[this](const WidgetState& state){
			this->regrindButton.set_label(get_display_text(state.text.value()).c_str());
			this->regrindButton.set_sensitive(state.is_sensitive.value());
			this->regrindButton.set_visible(state.is_visible.value());
		}
	}));

	// --- ログアウト確認ダイアログ ---
	m_widget_states.insert(make_pair(&logoutNoButton, WidgetStateUpdater{
		{UiText{"いいえ", "いいえ"}, true, true},
		[this](const WidgetState& state){
			this->logoutNoButton.set_label(get_display_text(state.text.value()).c_str());
			this->logoutNoButton.set_sensitive(state.is_sensitive.value());
			this->logoutNoButton.set_visible(state.is_visible.value());
		}
	}));
	m_widget_states.insert(make_pair(&logoutYesButton, WidgetStateUpdater{
		{UiText{"はい", "はい"}, true, true},
		[this](const WidgetState& state){
			this->logoutYesButton.set_label(get_display_text(state.text.value()).c_str());
			this->logoutYesButton.set_sensitive(state.is_sensitive.value());
			this->logoutYesButton.set_visible(state.is_visible.value());
		}
	}));

	// --- パスワード入力ダイアログ ---
	m_widget_states.insert(make_pair(&passwordEntryLabel, WidgetStateUpdater{
		{UiText{"パスワードを\n入力してください", "パスワードを\nにゅうりょく してください"}, true, true},
		[this](const WidgetState& state){
			this->passwordEntryLabel.set_label(get_display_text(state.text.value()).c_str());
			this->passwordEntryLabel.set_visible(state.is_visible.value());
		}
	}));
	m_widget_states.insert(make_pair(&passwordCancelButton, WidgetStateUpdater{
		{UiText{"キャンセル", "キャンセル"}, true, true},
		[this](const WidgetState& state){
			this->passwordCancelButton.set_label(get_display_text(state.text.value()).c_str());
			this->passwordCancelButton.set_sensitive(state.is_sensitive.value());
			this->passwordCancelButton.set_visible(state.is_visible.value());
		}
	}));
	m_widget_states.insert(make_pair(&passwordEnterButton, WidgetStateUpdater{
		{UiText{"確定", "かくてい"}, true, true},
		[this](const WidgetState& state){
			this->passwordEnterButton.set_label(get_display_text(state.text.value()).c_str());
			this->passwordEnterButton.set_sensitive(state.is_sensitive.value());
			this->passwordEnterButton.set_visible(state.is_visible.value());
		}
	}));

	// --- 棚卸し絞り込みダイアログ ---
	m_widget_states.insert(make_pair(&reconcileToolTypeFilterLabel, WidgetStateUpdater{
		{UiText{"分類", "ぶんるい"}, true, true},
		[this](const WidgetState& state){
			this->reconcileToolTypeFilterLabel.set_label(get_display_text(state.text.value()).c_str());
			this->reconcileToolTypeFilterLabel.set_visible(state.is_visible.value());
		}
	}));
	m_widget_states.insert(make_pair(&reconcilePeriodFilterLabel, WidgetStateUpdater{
		{UiText{"最終棚卸し日時", "さいしゅうたなおろしにちじ"}, true, true},
		[this](const WidgetState& state){
			this->reconcilePeriodFilterLabel.set_label(get_display_text(state.text.value()).c_str());
			this->reconcilePeriodFilterLabel.set_visible(state.is_visible.value());
		}
	}));
	m_widget_states.insert(make_pair(&reconcileFilterEnterButton, WidgetStateUpdater{
		{UiText{"決定", "けってい"}, true, true},
		[this](const WidgetState& state){
			this->reconcileFilterEnterButton.set_label(get_display_text(state.text.value()).c_str());
			this->reconcileFilterEnterButton.set_sensitive(state.is_sensitive.value());
			this->reconcileFilterEnterButton.set_visible(state.is_visible.value());
		}
	}));
	m_widget_states.insert(make_pair(&reconcileFilterResetButton, WidgetStateUpdater{
		{UiText{"期間リセット", "きかんリセット"}, true, true},
		[this](const WidgetState& state){
			this->reconcileFilterResetButton.set_label(get_display_text(state.text.value()).c_str());
			this->reconcileFilterResetButton.set_sensitive(state.is_sensitive.value());
			this->reconcileFilterResetButton.set_visible(state.is_visible.value());
		}
	}));

	// --- 警告ダイアログ（ラベルは動的に変わるため空） ---
	m_widget_states.insert(make_pair(&warningLabel, WidgetStateUpdater{
		{UiText{"", ""}, true, true},
		[this](const WidgetState& state){
			this->warningLabel.set_label(get_display_text(state.text.value()).c_str());
			this->warningLabel.set_visible(state.is_visible.value());
		}
	}));
	// TODO: 他のかな/漢字切替や、状態遷移での表示切替が必要なウィジェットも同様に追加

	//=========================================================================
	// 各テーブルのカラムヘッダー表示テキストを保持するマップの構築
	//=========================================================================

	// --- 補充する工具の情報を表示するテーブル ---
	m_table_states.insert(make_pair(&storingToolTable, TableState{
		std::vector<UiText>{
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"}
		}, false, true
	}));

	// --- 3列表示テーブル ---
	m_table_states.insert(make_pair(&table3column1st, TableState{
		std::vector<UiText>{
			{"ツール\n  番号", "  ツール\nばんごう"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"在庫数", "ざいこ\n  すう"}
		}, true, true
	}));
	m_table_states.insert(make_pair(&table3column2nd, TableState{
		std::vector<UiText>{
			{"ツール\n  番号", "  ツール\nばんごう"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"在庫数", "ざいこ\n  すう"}
		}, true, true
	}));
	m_table_states.insert(make_pair(&table3column3rd, TableState{
		std::vector<UiText>{
			{"ツール\n  番号", "  ツール\nばんごう"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"在庫数", "ざいこ\n  すう"}
		}, true, true
	}));

	// --- 2列表示テーブル ---
	m_table_states.insert(make_pair(&table2column1st, TableState{
		std::vector<UiText>{
			{"ID", "ID"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"再研磨\n  回数", "さいけんま\n  かいすう"}
		}, true, true
	}));
	m_table_states.insert(make_pair(&table2column2nd, TableState{
		std::vector<UiText>{
			{"ID", "ID"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"再研磨\n  回数", "さいけんま\n  かいすう"}
		}, true, true
	}));

	//=========================================================================
	// アプリケーション状態とウィジェット設定を紐付けるマップの構築
	//=========================================================================

	// --- 状態：ユーザーログイン待ち - 移行 ---
	g_app_state_configs.insert(make_pair(AppState::NONE, WidgetStateConfig{
		.on_enter_state = [](MainWindow* thiz){ thiz->set_app_state(AppState::WAITING_FOR_USER_LOGIN); }
	}));

	// --- 状態：ユーザーログイン待ち ---
	g_app_state_configs.insert(make_pair(AppState::WAITING_FOR_USER_LOGIN, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		{
			{ {&retrieveToggleButton, &storeToggleButton, &reconcileToggleButton, &logoutButton}, {.is_sensitive = false, .is_visible = true}},
			{ {&userIdEntry}, {UiText{"", ""}, true, true} },
			{ {&messageLabel}, {UiText{"名札のQRコードを\nQRコードリーダーにかざしてください", "なふだの QRコードを\nQRコードリーダーに かざしてください"}, true, true} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		{
			{ {&table3column1st, &table3column2nd, &table3column3rd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		"pageNone", "pageNone", // 初期ページ
		// --- 状態の「振る舞い」を定義 ---
		[](MainWindow* thiz){ thiz->on_enter_waiting_for_user_login(); },
		nullptr, // Falseボタンアクション
		nullptr  // Trueボタンアクション
	}));

	// --- 状態：取り出し - 移行 ---
	g_app_state_configs.insert(make_pair(AppState::RETRIEVE, WidgetStateConfig{
		.on_enter_state = [](MainWindow* thiz){ thiz->set_app_state(AppState::RETRIEVE_WAITING_FOR_PROGRAM_QR); }
	}));

	// --- 状態：取り出し - プログラムQR待ち ---
	g_app_state_configs.insert(make_pair(AppState::RETRIEVE_WAITING_FOR_PROGRAM_QR, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {UiText{"プログラム割当一覧表のQRコードを\nQRコードリーダーにかざしてください", "プログラムわりあていちらんひょうの QRコードを\nQRコードリーダーに かざしてください"}, true, true} },
			// ★変更点：qrCodeEntryBoxとqrCodeEntryLabelの表示設定を削除。これにより非表示になります。
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		.table_states = {
			{ {&table3column1st, &table3column2nd, &table3column3rd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.operationStackPageName = "retrieveOperationPage",
		.tableStackPageName = "pageNone",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){
			// 入力欄は見えませんが、フォーカスを当てて入力を待機させます
			thiz->qrCodeEntry.set_text("");
			gtk_widget_grab_focus(GTK_WIDGET(thiz->qrCodeEntry.object));
		},
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_retrieve_waiting_for_program_qr(); },
		.on_falseButton_clicked		= nullptr,
		.on_trueButton_clicked		= nullptr
	}));

	// --- 状態：取り出し - 工具選択中 ---
	g_app_state_configs.insert(make_pair(AppState::RETRIEVE_SELECTING_TOOLS, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		{
			{ {&messageLabel}, {UiText{"取り出す工具を選択してください", "とりだす こうぐを せんたく してください"}, true, true} },
			{ {&table3column1stBox, &table3column2ndBox, &table3column3rdBox}, {.is_visible = true} },
			{ {&trueButton}, {UiText{"確定", "かくてい"}, true, true} },
			{ {&falseButton}, {UiText{"全選択解除", "ぜんせんたくかいじょ"}, true, true} }
		},
		{
			{ {&table3column1st, &table3column2nd, &table3column3rd}, {.is_sensitive = true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		"retrieveOperationPage", "table3column",
		// --- 状態の「振る舞い」を定義 ---
		[](MainWindow* thiz){ /* TODO: DBから取得した工具リストをテーブルに表示する */ },
		[](MainWindow* thiz){ thiz->set_app_state(AppState::RETRIEVE_CONFIRMING_SELECTION); },
		[](MainWindow* thiz){ /* TODO: テーブルの全選択を解除する処理 */ }
	}));

	// --- 状態：取り出し - 選択内容の確認中 ---
	g_app_state_configs.insert(make_pair(AppState::RETRIEVE_CONFIRMING_SELECTION, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		{
			{ {&messageLabel}, {UiText{"選択した工具は以下の通りです\nよろしければ「搬出」を押してください", "せんたくした こうぐは いかのとおりです..."}, true, true} },
			{ {&table2column1stBox}, {.is_visible = true} },
			{ {&table2column2ndBox}, {.is_visible = false} },
			{ {&trueButton}, {UiText{"搬出", "はんしゅつ"}, true, true} },
			{ {&falseButton}, {UiText{"キャンセル", "キャンセル"}, true, true} }
		},
		{
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} },
			// TODO: 選択された工具のみを表示するテーブルに設定
		},
		// --- 各スタックの「表示ページ」を定義 ---
		"retrieveOperationPage", "table2column",
		// --- 状態の「振る舞い」を定義 ---
		[](MainWindow* thiz){ /* TODO: 選択された工具のみをテーブルに表示する */ },
		[](MainWindow* thiz){ /* TODO: 搬出処理を開始し、次の状態へ */ },
		[](MainWindow* thiz){ thiz->set_app_state(AppState::RETRIEVE_SELECTING_TOOLS); } // 前の状態に戻る
	}));

	// --- 状態：補充 - 移行 ---
	g_app_state_configs.insert(make_pair(AppState::STORE, WidgetStateConfig{
		.on_enter_state = [](MainWindow* thiz){ thiz->set_app_state(AppState::STORE_WAITING_FOR_TOOL_QR); }
	}));

	// --- 状態：補充 - 工具QR待ち ---
	g_app_state_configs.insert(make_pair(AppState::STORE_WAITING_FOR_TOOL_QR, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {UiText{"補充する工具のQRコードを\nQRコードリーダーにかざしてください", "ほじゅうする こうぐの QRコードを\nQRコードリーダーに かざしてください"}, true, true} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		.table_states = {
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.operationStackPageName = "storeOperationPage",
		.tableStackPageName = "pageNone",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_waiting_for_tool_qr(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_store_waiting_for_tool_qr(); }
	}));

	// --- 状態：補充 - 読み取った工具の確認 ---
	g_app_state_configs.insert(make_pair(AppState::STORE_CONFIRMING_SELECTION, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {UiText{"補充する工具に間違いがなければ\n「搬出」を押してください", "ほじゅうする こうぐに まちがいが なければ\n「はんしゅつ」を おしてください"}, true, true} },
			{ {&trueButton}, {UiText{"搬出", "はんしゅつ"}, true, true} },
			{ {&falseButton}, {UiText{"キャンセル", "キャンセル"}, true, true} }
		},
		.table_states = {
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName = "pageNone",
		// --- 状態の「振る舞い」を定義 ---
		.on_falseButton_clicked		= [](MainWindow* thiz){ thiz->set_app_state(AppState::STORE_WAITING_FOR_TOOL_QR); },	// 前の状態に戻る
		.on_trueButton_clicked		= [](MainWindow* thiz){ thiz->set_app_state(AppState::STORE_CONTAINER_RETRIEVING); }	// コンテナ搬出処理へ
	}));

	// --- 状態：補充 - コンテナ搬出待ち ---
	g_app_state_configs.insert(make_pair(AppState::STORE_CONTAINER_RETRIEVING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {UiText{"コンテナを搬出しています\nしばらくお待ち下さい", "コンテナを はんしゅつ しています\nしばらく おまちください"}, true, true} },
			{ {&table2column1stBox}, {.is_visible = true} },
			{ {&table2column2ndBox}, {.is_visible = false} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		.table_states = {
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName = "table2column",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_container_retrieving(); }
	}));

	// --- 状態：補充 - コンテナ搬出完了 ---
	g_app_state_configs.insert(make_pair(AppState::STORE_CONTAINER_RETRIEVED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {UiText{"コンテナが届きました\n扉を開けてコンテナを取り出してください", "コンテナが とどきました\nとびらを あけて コンテナを とりだしてください"}, true, true} },
			{ {&table2column1stBox}, {.is_visible = true} },
			{ {&table2column2ndBox}, {.is_visible = false} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		.table_states = {
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName = "table2column",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_container_retrieved(); }
	}));

	// --- 状態：補充 - 実際に補充を行う(インサート以外) - 工具読み取り前 ---
	g_app_state_configs.insert(make_pair(AppState::STORE_READY_FOR_READING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {UiText{"工具のQRコードを QRコードリーダーで\n読み取ってからコンテナに収納してください", "こうぐの QRコードを QRコードリーダーで\nよみとってから コンテナに しゅうのうしてください"}, true, true} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		.table_states = {
			{ {&table2column1st, &table2column2nd}, {
				.column_headers = std::vector<UiText>{
					{"ID", "ID"},
					{"分類", "ぶんるい"},
					{"メーカー", "メーカー"},
					{"型番", "かたばん"},
					{"材質", "ざいしつ"},
					{"再研磨\n  回数", "さいけんま\n  かいすう"}
				}, .is_sensitive = false
			} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName = "pageNone",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_ready_for_reading(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_store_ready_for_reading(); }
	}));

	// --- 状態：補充 - 実際に補充を行う(インサート以外) - 工具読み取り中 ---
	g_app_state_configs.insert(make_pair(AppState::STORE_READING_TOOL_QR, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&table2column1stBox, &table2column2ndBox}, {.is_visible = true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName = "table2column",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_reading_tool_qr(); }
	}));

	// --- 状態：補充 - 実際に補充を行う(インサート以外) - 工具読み取り後 ---
	g_app_state_configs.insert(make_pair(AppState::STORE_READ_COMPLETED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {UiText{"補充が終わったらコンテナを戻して\n扉を閉めてください", "ほじゅうが おわったら コンテナを もどして\nとびらを しめてください"}, true, true} },
			{ {&table2column1stBox, &table2column2ndBox}, {.is_visible = true} },
			{ {&trueButton}, {UiText{"続ける", "つづける"}, true, true} },
			{ {&falseButton}, {UiText{"終了", "しゅうりょう"}, true, true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName = "table2column",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_read_completed(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_store_ready_for_reading(); },
		.on_falseButton_clicked		= [](MainWindow* thiz){ thiz->on_falseButton_store_read_completed(); },
		.on_trueButton_clicked		= [](MainWindow* thiz){ thiz->on_trueButton_store_read_completed(); }
	}));

	// --- 状態：補充 - 実際に補充を行う(インサート) - 工具読み取り前 ---
	g_app_state_configs.insert(make_pair(AppState::STORE_INSERT_READY_FOR_READING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {UiText{"工具のQRコードを QRコードリーダーで\n読み取ってからコンテナに収納してください", "こうぐの QRコードを QRコードリーダーで\nよみとってから コンテナに しゅうのうしてください"}, true, true} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		.table_states = {
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName = "pageNone",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_insert_ready_for_reading(); }
	}));

	// --- 状態：補充 - 実際に補充を行う(インサート) - 工具読み取り中 ---
	g_app_state_configs.insert(make_pair(AppState::STORE_INSERT_READING_TOOL_QR, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&table2column1stBox, &table2column2ndBox}, {.is_visible = true} }
		},
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_insert_reading_tool_qr(); }
	}));

	// --- 状態：補充 - 実際に補充を行う(インサート) - 工具読み取り後 ---
	g_app_state_configs.insert(make_pair(AppState::STORE_INSERT_READ_COMPLETED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {UiText{"補充が終わったらコンテナを戻して\n扉を閉めてください", "ほじゅうが おわったら コンテナを もどして\nとびらを しめてください"}, true, true} },
			{ {&table2column1stBox, &table2column2ndBox}, {.is_visible = true} },
			{ {&trueButton}, {UiText{"続ける", "つづける"}, true, true} },
			{ {&falseButton}, {UiText{"終了", "しゅうりょう"}, true, true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName = "table2column",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_insert_read_completed(); },
		.on_falseButton_clicked		= [](MainWindow* thiz){ /* TODO: 終了ボタンがクリックされた時の処理 */ },
		.on_trueButton_clicked		= [](MainWindow* thiz){ /* TODO: 続けるボタンがクリックされた時の処理 */ }
	}));

	// --- 状態：棚卸し - 移行 ---
	g_app_state_configs.insert(make_pair(AppState::RECONCILE, WidgetStateConfig{
		.on_enter_state = [](MainWindow* thiz){ thiz->set_app_state(AppState::RECONCILE_WAITING_FOR_TOOL_SELECTION); }
	}));

	// --- 状態：棚卸し - 工具選択待ち ---
	g_app_state_configs.insert(make_pair(AppState::RECONCILE_WAITING_FOR_TOOL_SELECTION, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		{
			{ {&messageLabel}, {UiText{"棚卸しを行うコンテナを選択してください", "たなおろしをおこなうコンテナをせんたくしてください"}, true, true} },
			{ {&table2column1stBox, &table2column2ndBox}, {.is_visible = true} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		{
			{ {&table2column1st, &table2column2nd}, {
				.column_headers = std::vector<UiText>{
					{"コンテナ\n  番号", "コンテナ\nばんごう"},
					{"分類", "ぶんるい"},
					{"メーカー", "メーカー"},
					{"型番", "かたばん"},
					{"材質", "ざいしつ"},
					{"  　最終\n棚卸し日時", "  　さいしゅう\nたなおろしにちじ"}
				}, .is_sensitive = true
			} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		"reconcileOperationPage", "table2column",
		// --- 状態の「振る舞い」を定義 ---
		[](MainWindow* thiz){ /* TODO: DBから取得したコンテナリストをテーブルに表示する */ },
		nullptr,
		nullptr
	}));

	// TODO: 他のすべての状態(RETRIEVE_WAITING_FOR_CONTAINERなど)もここに追加していく
}

//=============================================================================
// 状態遷移関数
//=============================================================================

void MainWindow::set_app_state(AppState state) {
	// 1. メンバ変数の状態を更新
	m_app_state = state;

	// 2. m_widget_statesを更新する
	const auto& state_config = g_app_state_configs.at(state);
	for (const auto& widget_state : state_config.widget_states) {
		for (const auto& widget : widget_state.widgets) {
			update_widget_state(widget, widget_state.state);
		}
	}

	// 3. m_table_statesを更新する
	for (const auto& table_state : state_config.table_states) {
		for (const auto& table : table_state.tables) {
			update_table_state(table, table_state.state);
		}
	}

	// 4. 各スタックの表示ページを更新
	if (!state_config.operationStackPageName.empty()) {
		operationStack.set_visible_child_name(state_config.operationStackPageName.c_str());
	}
	if (!state_config.tableStackPageName.empty()) {
		table_stack.set_visible_child_name(state_config.tableStackPageName.c_str());
	}

	// 5. UIを再描画
	refresh_ui(false);

	// 6. 状態遷移直後に実行するアクションがあれば実行
	if (state_config.on_enter_state) {
		state_config.on_enter_state(this);
	}
}

//=============================================================================
// ヘルパー関数実装
//=============================================================================

void MainWindow::update_widget_state(sg::Widget* widget, WidgetState state) {
	// m_widget_statesから現在のWidgetStateUpdaterのコピーを取得
	auto updater = m_widget_states.at(widget);

	// stateメンバーだけを新しい状態に更新
	if (updater.state.merge(state)) {
		// 変更があった場合、更新したWidgetStateUpdaterをマップに書き戻す
		m_widget_states.insert_or_assign(widget, updater);
	}
}

void MainWindow::update_table_state(tool_io::TreeView* table, TableState state) {
	// m_table_statesから現在のTableStateのコピーを取得
	auto current_state = m_table_states.at(table);

	// stateメンバーだけを新しい状態に更新
	if (current_state.merge(state)) {
		// 変更があった場合、更新したTableStateをマップに書き戻す
		m_table_states.insert_or_assign(table, current_state);
	}
}

void MainWindow::set_mode(GuiMode mode) {
	if (mode != m_gui_mode && (mode == GuiMode::STORE || mode == GuiMode::RECONCILE)) {
		// モードが変わる場合のみ認証を行う
		if (!authenticate_with_password()) {
			// 認証失敗またはキャンセルされた場合、モード変更を中止
			// トグルボタンの状態を元に戻す
			m_isProgrammaticallyChangingToggle = true;
			retrieveToggleButton.set_active(m_gui_mode == GuiMode::RETRIEVE);
			storeToggleButton.set_active(m_gui_mode == GuiMode::STORE);
			reconcileToggleButton.set_active(m_gui_mode == GuiMode::RECONCILE);
			m_isProgrammaticallyChangingToggle = false;
			return;
		}
	}

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
	case GuiMode::NONE:
		fmt::print("ログイン待ちモード\n");
		set_app_state(AppState::NONE);
		break;
	case GuiMode::RETRIEVE:
		fmt::print("取り出しモード\n");
		set_app_state(AppState::RETRIEVE);
		break;
	case GuiMode::STORE:
		fmt::print("補充モード\n");
		set_app_state(AppState::STORE);
		break;
	case GuiMode::RECONCILE:
		fmt::print("棚卸しモード\n");
		set_app_state(AppState::RECONCILE);
		break;
	}
}

bool MainWindow::authenticate_with_password() {
	// ダイアログ表示前にパスワード欄をクリア
	passwordEntry.set_text("");

	GtkResponseType response;

	// 既に認証済みならば、再度認証を求めない
	if (!g_password_authenticated) {
		// パスワード認証が成功するか、キャンセルされるまで繰り返す
		update_widget_state(&passwordEntryLabel, WidgetState{
			.text = UiText{"パスワードを\n入力してください", "ぱすわーどを\nにゅうりょく してください"}
		});
		refresh_ui(false); // ラベルのテキストを更新
		do {
			// run()でダイアログをモーダル表示し、ユーザーの応答を待つ
			response = passwordEntryDialog.run(this);
			if (response == GTK_RESPONSE_OK) {
				// TODO: ここで実際のパスワード検証ロジックを実装します
				// 今回はダミーとして "1234" を正しいパスワードとします
				fmt::print("認証開始\n");
				if (std::string(passwordEntry.get_text()) == "1234") {
					fmt::print("認証成功\n");
					g_password_authenticated = true;
				} else {
					fmt::print("認証失敗：パスワードが違います\n");
					update_widget_state(&passwordEntryLabel, WidgetState{
						.text = UiText{"パスワードが違います\nもう一度入力してください", "ぱすわーどが ちがいます\nもういちど にゅうりょく してください"}
					});
					refresh_ui(false); // ラベルのテキストを更新
				}
			} else if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT) {
				// キャンセルボタンが押されたか、ダイアログが閉じられた場合
				g_password_authenticated = false;
				fmt::print("認証キャンセル\n");
			}
		} while (response == GTK_RESPONSE_OK && !g_password_authenticated);

		// ダイアログを非表示にする
		passwordEntryDialog.hide();
	}
	return g_password_authenticated;
}

void MainWindow::refresh_ui(bool force_redraw_all) {
	// 1. 更新対象となるウィジェットのリストを取得
	auto widgets_to_update = force_redraw_all
		? m_widget_states.get_all(true)	// 全ウィジェットを取得
		: m_widget_states.get_updated_elements(true); // 更新されたウィジェットのみ取得し、取得後に更新リストをクリア

	// 2. 対象ウィジェットの表示を更新
	for (const auto& [widget, state_updater] : widgets_to_update) {
		// セッター関数が登録されていれば呼び出す
		if (state_updater.setter) {
			// セッター関数を呼び出して、テキストと状態を更新
			state_updater.setter(state_updater.state);
		}
	}

	// 3. 更新対象となるテーブルのリストを取得
	auto tables_to_update = force_redraw_all
		? m_table_states.get_all(true)	// 全テーブルを取得
		: m_table_states.get_updated_elements(true); // 更新されたテーブルのみ取得し、取得後に更新リストをクリア

	// 4. 対象テーブルのカラムヘッダー表示を更新
	for (const auto& [table, headers] : tables_to_update) {
		for (guint i = 0; i < headers.column_headers.value().size(); ++i) {
			GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(table->object), i);
			if (column) {
				const auto& header = headers.column_headers.value()[i];
				const std::string& title = (m_text_mode == TextMode::KANA) ? header.kana : header.kanji;
				gtk_tree_view_column_set_title(column, title.c_str());
			}
		}
		table->set_sensitive(headers.is_sensitive.value());
		table->set_visible(headers.is_visible.value());
	}
}

//=============================================================================
// イベントハンドラ実装
//=============================================================================

void MainWindow::on_window_show() {
	if (m_is_first_show) {
		// 初回表示の時だけ、UI全体を初期状態に合わせて完全に再描画し、初期設定を行う
		refresh_ui(true);
		set_mode(GuiMode::NONE);	// 最初は取り出しモード
		set_app_state(AppState::WAITING_FOR_USER_LOGIN);	// 最初の状態を設定
		m_is_first_show = false; // フラグを倒して次回以降は実行しない
	}
}

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

	// このボタンに対する操作は、常に「補充」モードへの設定を試みる
	// これにより、ユーザーがアクティブなこのボタンを押してオフにしようとしても、
	// set_modeが再度このボタンをオンにし、状態が維持される
	set_mode(GuiMode::STORE);
}

void MainWindow::reconcileToggleButton_on_toggled() {
	fmt::print("[棚卸し]\n");
	if (m_isProgrammaticallyChangingToggle) {
		fmt::print("無視\n");
		return;	// プログラムによる変更は無視
	}

	// このボタンに対する操作は、常に「棚卸し」モードへの設定を試みる
	// これにより、ユーザーがアクティブなこのボタンを押してオフにしようとしても、
	// set_modeが再度このボタンをオンにし、状態が維持される
	set_mode(GuiMode::RECONCILE);
}

void MainWindow::kanaKanjiButton_on_clicked() {
	m_text_mode = (m_text_mode == TextMode::KANJI) ? TextMode::KANA : TextMode::KANJI;
	refresh_ui(true); // 現在の状態のままUIテキストを更新
}

void MainWindow::logoutButton_on_clicked() {
	fmt::print("ログアウトボタンがクリックされました\n");
	if (logoutDialog.run(this) == GTK_RESPONSE_YES) {
		fmt::print("ログアウトします\n");
		set_mode(GuiMode::NONE);
	} else {
		fmt::print("ログアウトはキャンセルされました\n");
	}
}

void MainWindow::userIdEntry_on_activate() {
	fmt::print("ユーザーIDエントリでEnterキーが押されました\n");

	if (user_authenticate(stoi(std::string(userIdEntry.get_text())))) {
		fmt::print("ユーザー認証成功\n");

		// 権限に応じてトグルボタンの操作可否を設定
		// 取り出しトグルボタン
		update_widget_state(&retrieveToggleButton, WidgetState{
			.is_sensitive = g_logged_in_user_info.value().per_out,
		});
		// 補充トグルボタン
		update_widget_state(&storeToggleButton, WidgetState{
			.is_sensitive = g_logged_in_user_info.value().per_add,
		});
		// 棚卸しトグルボタン
		update_widget_state(&reconcileToggleButton, WidgetState{
			.is_sensitive = g_logged_in_user_info.value().per_reconcile,
		});
		// ユーザーIDエントリにログインユーザー名を表示して固定
		update_widget_state(&userIdEntry, WidgetState{
			.text = UiText{g_logged_in_user_info.value().user_name, g_logged_in_user_info.value().user_name},
			.is_sensitive = false
		});
		// ログアウトボタンを操作可能に
		update_widget_state(&logoutButton, WidgetState{
			.is_sensitive = true
		});
		refresh_ui(false); // UIを更新

		if (g_logged_in_user_info.value().per_out) {
			// 取り出し権限があれば取り出しモードへ
			set_mode(GuiMode::RETRIEVE);
		} else if (g_logged_in_user_info.value().per_add) {
			// 補充権限があれば補充モードへ
			set_mode(GuiMode::STORE);
		} else if (g_logged_in_user_info.value().per_reconcile) {
			// 棚卸し権限があれば棚卸しモードへ
			set_mode(GuiMode::RECONCILE);
		} else {
			// どの権限もなければ認証画面に戻る
			set_mode(GuiMode::NONE);
		}
	} else {
		fmt::print("ユーザー認証失敗\n");
		// TODO: エラーメッセージを表示する処理
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

void MainWindow::qrCodeEntry_on_activate() {
	try {
		auto state_config = g_app_state_configs.at(m_app_state);
		if (state_config.on_qrCodeEntry_activate) {
			state_config.on_qrCodeEntry_activate(this);
		}
	} catch (const std::out_of_range& e) {
		fmt::print("Error: 未定義の状態でqrCodeEntryでEnterが押されました (AppState={})\n", static_cast<int>(m_app_state));
		// 例外を無視して処理を続行
	}
}

void MainWindow::falseButton_on_clicked() {
	try {
		auto state_config = g_app_state_configs.at(m_app_state);
		if (state_config.on_falseButton_clicked) {
			state_config.on_falseButton_clicked(this);
		}
	} catch (const std::out_of_range& e) {
		fmt::print("Error: 未定義の状態でFalseボタンがクリックされました (AppState={})\n", static_cast<int>(m_app_state));
		// 例外を無視して処理を続行
	}
}

void MainWindow::trueButton_on_clicked() {
	try {
		auto state_config = g_app_state_configs.at(m_app_state);
		if (state_config.on_trueButton_clicked) {
			state_config.on_trueButton_clicked(this);
		}
	} catch (const std::out_of_range& e) {
		fmt::print("Error: 未定義の状態でTrueボタンがクリックされました (AppState={})\n", static_cast<int>(m_app_state));
		// 例外を無視して処理を続行
	}
}

void MainWindow::logoutNoButton_on_clicked() {
	gtk_dialog_response(GTK_DIALOG(this->logoutDialog.object), GTK_RESPONSE_NO);
}

void MainWindow::logoutYesButton_on_clicked() {
	gtk_dialog_response(GTK_DIALOG(this->logoutDialog.object), GTK_RESPONSE_YES);
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

//=========================================================================
// 状態毎の関数 実装
//=========================================================================

void MainWindow::on_enter_waiting_for_user_login() {
	gtk_widget_grab_focus(GTK_WIDGET(this->userIdEntry.object));
}

void MainWindow::on_enter_store_waiting_for_tool_qr() {
	storingToolListstore.clear();
	storingToolListstore.append(0, "", 1, "", 2, "", 3, "", -1);
	qrCodeEntry.set_text("");
	gtk_widget_grab_focus(GTK_WIDGET(this->qrCodeEntry.object));
}

void MainWindow::on_qrCodeEntry_store_waiting_for_tool_qr() {
	ToolBasicInfo basic_info;
	std::string text(qrCodeEntry.get_text());
	fmt::print("補充モード - 工具QRコード入力: '{}'\n", text);
	auto individual_info = get_tool_data(stoi(text));

	if (!individual_info.has_value()) {
		basic_info = individual_info.value().basic_info;
		storingToolListstore.clear();
		storingToolListstore.append(
			0, basic_info.class_name.c_str(),
			1, basic_info.maker.c_str(),
			2, basic_info.tool_name.c_str(),
			3, basic_info.material.c_str(),
			-1
		);
		set_app_state(AppState::STORE_CONFIRMING_SELECTION);
	} else {
		// TODO: 該当データが無い場合の処理
		update_widget_state(&messageLabel, WidgetState{
			.text = UiText{"該当する工具データが見つかりません\nもう一度QRコードを読み取ってください", "がいとうする こうぐデータが みつかりません\nもういちど QRコードを よみとってください"}
		});
		refresh_ui(false); // ラベルのテキストを更新
	}
}

// tool_io_gui.cpp の末尾に追加
/**
 * @brief 取り出しモードでプログラムQRが入力された際の処理
 */
void MainWindow::on_qrCodeEntry_retrieve_waiting_for_program_qr() {
    std::string qr_text(qrCodeEntry.get_text());
    fmt::print("取り出しモード - プログラムQRコード入力: '{}'\n", qr_text);

    // 1. カンマで文字列を分割
    size_t comma_pos = qr_text.find(',');
    if (comma_pos == std::string::npos) {
        // エラー処理: カンマが見つからない場合
        update_widget_state(&messageLabel, WidgetState{
            .text = UiText{"QRコードの形式が不正です\nもう一度読み取ってください", "QRコードの けいしきが ふせいです\nもういちど よみとってください"}
        });
        refresh_ui(false);
        qrCodeEntry.set_text(""); // 入力欄をクリア
        return;
    }

    // 2. ライン番号とプログラム番号を抽出
    std::string line_str = qr_text.substr(0, comma_pos);
    std::string program_str = qr_text.substr(comma_pos + 1);

    // 3. 抽出した情報をUIに設定
    lineEntry.set_text(line_str.c_str());
    programEntry.set_text(program_str.c_str());

    // 4. 次の状態（工具選択）へ遷移
    set_app_state(AppState::RETRIEVE_SELECTING_TOOLS);
}

void MainWindow::on_enter_store_container_retrieving() {
}

void MainWindow::on_enter_store_container_retrieved() {
}

void MainWindow::on_enter_store_ready_for_reading() {
}

void MainWindow::on_qrCodeEntry_store_ready_for_reading() {
}

void MainWindow::on_enter_store_reading_tool_qr() {
}

void MainWindow::on_enter_store_read_completed() {
}

void MainWindow::on_falseButton_store_read_completed() {
}

void MainWindow::on_trueButton_store_read_completed() {
}

void MainWindow::on_enter_store_insert_ready_for_reading() {
}

void MainWindow::on_enter_store_insert_reading_tool_qr() {
}

void MainWindow::on_enter_store_insert_read_completed() {
}