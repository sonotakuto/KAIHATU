/**
 * @file tool_io_gui.cpp
 * @brief tool_io_gui.hppで宣言されたクラスのメンバ関数の実装ファイル。
 */
#include "./includes.hpp"
#include "./SimpleGtk_extension.hpp"

//=============================================================================
// MainWindowのメンバクラスの関数
//=============================================================================

void MainWindow::WidgetState::merge(const WidgetState& other) {
	if (other.text.has_value()) {
		this->text = other.text;
	}
	if (other.is_sensitive.has_value()) {
		this->is_sensitive = other.is_sensitive;
	}
	if (other.is_visible.has_value()) {
		this->is_visible = other.is_visible;
	}
}

void MainWindow::TableState::merge(const TableState& other) {
	if (other.column_headers.has_value()) {
		this->column_headers = other.column_headers;
	}
	if (other.is_sensitive.has_value()) {
		this->is_sensitive = other.is_sensitive;
	}
	if (other.is_visible.has_value()) {
		this->is_visible = other.is_visible;
	}
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
	// ウィジェットの状態を保持するマップの構築
	//=========================================================================

	// --- 「取り出し」トグルボタン ---
	m_widget_states.insert(make_pair(&retrieveToggleButton, WidgetStateUpdater{
		{UiText{"取り出し", "とりだし"}, true, true},
		[this](const WidgetState& state){
			this->retrieveToggleButton.set_label(get_display_text(state.text.value()).c_str());
			this->retrieveToggleButton.set_sensitive(state.is_sensitive.value());
			this->retrieveToggleButton.set_visible(state.is_visible.value());
		}
	}));

	// --- 「補充」トグルボタン ---
	m_widget_states.insert(make_pair(&storeToggleButton, WidgetStateUpdater{
		{UiText{"補充", "ほじゅう"}, true, true},
		[this](const WidgetState& state){
			this->storeToggleButton.set_label(get_display_text(state.text.value()).c_str());
			this->storeToggleButton.set_sensitive(state.is_sensitive.value());
			this->storeToggleButton.set_visible(state.is_visible.value());
		}
	}));

	// --- 「棚卸し」トグルボタン ---
	m_widget_states.insert(make_pair(&reconcileToggleButton, WidgetStateUpdater{
		{UiText{"棚卸し", "たなおろし"}, true, true},
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
		{UiText{"ログアウト", "ログアウト"}, true, true},
		[this](const WidgetState& state){
			this->logoutButton.set_label(get_display_text(state.text.value()).c_str());
			this->logoutButton.set_sensitive(state.is_sensitive.value());
			this->logoutButton.set_visible(state.is_visible.value());
		}
	}));

	// --- 「ライン番号」ラベル ---
	m_widget_states.insert(make_pair(&lineLabel, WidgetStateUpdater{
		{UiText{"ライン番号：", "らいんばんごう："}, true, true},
		[this](const WidgetState& state){
			this->lineLabel.set_label(get_display_text(state.text.value()).c_str());
			// ラベルのsensitiveは通常操作しない
			this->lineLabel.set_visible(state.is_visible.value());
		}
	}));

	// --- 「プログラム番号」ラベル ---
	m_widget_states.insert(make_pair(&programLabel, WidgetStateUpdater{
		{UiText{"プログラム番号：", "ぷろぐらむばんごう："}, true, true},
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
		{UiText{"コンテナ番号：", "こんてなばんごう："}, true, true},
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

	// --- true/falseボタン（ラベルは動的に変わるため空） ---
	m_widget_states.insert(make_pair(&trueButton, WidgetStateUpdater{
		{UiText{"", ""}, true, true},
		[this](const WidgetState& state){
			this->trueButton.set_label(get_display_text(state.text.value()).c_str());
			this->trueButton.set_sensitive(state.is_sensitive.value());
			this->trueButton.set_visible(state.is_visible.value());
		}
	}));
	m_widget_states.insert(make_pair(&falseButton, WidgetStateUpdater{
		{UiText{"", ""}, true, true},
		[this](const WidgetState& state){
			this->falseButton.set_label(get_display_text(state.text.value()).c_str());
			this->falseButton.set_sensitive(state.is_sensitive.value());
			this->falseButton.set_visible(state.is_visible.value());
		}
	}));

	// --- インサート枚数確認ダイアログ ---
	m_widget_states.insert(make_pair(&confirmInsertCountLabel, WidgetStateUpdater{
		{UiText{"使用するインサートの枚数を\n入力してください", "しようする いんさーとの まいすうを\nにゅうりょく してください"}, true, true},
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
		{UiText{"パスワードを\n入力してください", "ぱすわーどを\nにゅうりょく してください"}, true, true},
		[this](const WidgetState& state){
			this->passwordEntryLabel.set_label(get_display_text(state.text.value()).c_str());
			this->passwordEntryLabel.set_visible(state.is_visible.value());
		}
	}));
	m_widget_states.insert(make_pair(&passwordCancelButton, WidgetStateUpdater{
		{UiText{"キャンセル", "きゃんせる"}, true, true},
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
		{UiText{"期間リセット", "きかんりせっと"}, true, true},
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

	// --- 状態：ユーザーログイン待ち ---
	g_app_state_configs.insert(make_pair(AppState::WAITING_FOR_USER_LOGIN, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		{
			{ {&messageLabel}, {UiText{"名札のQRコードを\nQRコードリーダーにかざしてください", "なふだの QRコードを\nQRコードリーダーに かざしてください"}, true, true} },
			{ {&table3column1stBox, &table3column2ndBox, &table3column3rdBox}, {.is_visible = false} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		{
			{ {&table3column1st, &table3column2nd, &table3column3rd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		"retrieveOperationPage", "table3column", // 初期ページ
		// --- 状態の「振る舞い」を定義 ---
		[](MainWindow* thiz){ gtk_widget_grab_focus(GTK_WIDGET(thiz->passwordEntry.object)); },
		nullptr, // Trueボタンアクション
		nullptr  // Falseボタンアクション
	}));

	// --- 状態：取り出し - 移行 ---
	g_app_state_configs.insert(make_pair(AppState::RETRIEVE, WidgetStateConfig{
		.on_enter_state = [](MainWindow* thiz){ thiz->set_app_state(AppState::RETRIEVE_WAITING_FOR_PROGRAM_QR); }
	}));

	// --- 状態：取り出し - プログラムQR待ち ---
	g_app_state_configs.insert(make_pair(AppState::RETRIEVE_WAITING_FOR_PROGRAM_QR, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		{
			{ {&messageLabel}, {UiText{"プログラム割当一覧表のQRコードを\nQRコードリーダーにかざしてください", "プログラムわりあていちらんひょうの QRコードを\nQRコードリーダーにかざしてください"}, true, true} },
			{ {&table3column1stBox, &table3column2ndBox, &table3column3rdBox}, {.is_visible = false} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		{
			{ {&table3column1st, &table3column2nd, &table3column3rd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		"retrieveOperationPage", "table3column",
		// --- 状態の「振る舞い」を定義 ---
		[](MainWindow* thiz){ /* TODO: テーブルをクリアするなどの初期化処理 */ },
		nullptr,
		nullptr
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
			{ {&falseButton}, {UiText{"キャンセル", "きゃんせる"}, true, true} }
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
		{
			{ {&messageLabel}, {UiText{"補充する工具のQRコードを\nQRコードリーダーにかざしてください", "ほじゅうする こうぐの QRコードを\nQRコードリーダーに かざしてください"}, true, true} },
			{ {&table2column1stBox, &table2column2ndBox}, {.is_visible = false} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		{
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		"storeOperationPage", "table2column",
		// --- 状態の「振る舞い」を定義 ---
		[](MainWindow* thiz){ /* TODO: テーブルをクリアするなどの初期化処理 */ },
		nullptr,
		nullptr
	}));

	// --- 状態：補充 - 読み取った工具の確認 ---
	g_app_state_configs.insert(make_pair(AppState::STORE_CONFIRMING_SELECTION, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		{
			{ {&messageLabel}, {UiText{"補充する工具に間違いがなければ\n「搬出」を押してください", "ほじゅうする こうぐに まちがいが なければ\n「はんしゅつ」を おしてください"}, true, true} },
			{ {&table2column1stBox, &table2column2ndBox}, {.is_visible = false} },
			{ {&trueButton}, {UiText{"搬出", "はんしゅつ"}, true, true} },
			{ {&falseButton}, {UiText{"キャンセル", "きゃんせる"}, true, true} }
		},
		{
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		"storeOperationPage", "table2column",
		// --- 状態の「振る舞い」を定義 ---
		[](MainWindow* thiz){ /* TODO: 読み取った工具情報をテーブルに表示する */ },
		[](MainWindow* thiz){ /* TODO: 搬出処理を開始し、次の状態へ */ },
		[](MainWindow* thiz){ thiz->set_app_state(AppState::STORE_WAITING_FOR_TOOL_QR); } // 前の状態に戻る
	}));

	// --- 状態：補充 - コンテナ搬出待ち ---
	g_app_state_configs.insert(make_pair(AppState::STORE_CONTAINER_RETRIEVING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		{
			{ {&messageLabel}, {UiText{"コンテナを搬出しています\nしばらくお待ち下さい", "コンテナを はんしゅつ しています\nしばらく おまちください"}, true, true} },
			{ {&table2column1stBox, &table2column2ndBox}, {.is_visible = false} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		{
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		"storeOperationPage", "table2column",
		// --- 状態の「振る舞い」を定義 ---
		[](MainWindow* thiz){ /* TODO: 搬出完了まで待機 */ },
		nullptr,
		nullptr
	}));

	// --- 状態：補充 - コンテナ搬出完了 ---
	g_app_state_configs.insert(make_pair(AppState::STORE_CONTAINER_RETRIEVED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		{
			{ {&messageLabel}, {UiText{"コンテナが届きました\n扉を開けてコンテナを取り出してください", "コンテナが とどきました\nとびらを あけて コンテナを とりだしてください"}, true, true} },
			{ {&table2column1stBox, &table2column2ndBox}, {.is_visible = false} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		{
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		"storeOperationPage", "table2column",
		// --- 状態の「振る舞い」を定義 ---
		[](MainWindow* thiz){ /* TODO: コンテナ取り出し完了まで待機 */ },
		nullptr,
		nullptr
	}));

	// --- 状態：補充 - 実際に補充を行う(インサート以外) - 工具読み取り前 ---
	g_app_state_configs.insert(make_pair(AppState::STORE_READY_FOR_READING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		{
			{ {&messageLabel}, {UiText{"工具のQRコードを QRコードリーダーで\n読み取ってからコンテナに収納してください", "こうぐの QRコードを QRコードリーダーで\nよみとってから コンテナに しゅうのうしてください"}, true, true} },
			{ {&table2column1stBox, &table2column2ndBox}, {.is_visible = false} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		{
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		"storeOperationPage", "table2column",
		// --- 状態の「振る舞い」を定義 ---
		[](MainWindow* thiz){ /* TODO: QRコード読み取り処理を開始 */ },
		nullptr,
		nullptr
	}));

	// --- 状態：棚卸し - 移行 ---
	g_app_state_configs.insert(make_pair(AppState::RECONCILE, WidgetStateConfig{
		.on_enter_state = [](MainWindow* thiz){ thiz->set_app_state(AppState::RECONCILE_WAITING_FOR_TOOL_SELECTION); }
	}));

	// --- 状態：棚卸し - 工具選択待ち ---
	g_app_state_configs.insert(make_pair(AppState::RECONCILE_WAITING_FOR_TOOL_SELECTION, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		{
			{ {&messageLabel}, {UiText{"棚卸しを行うコンテナを選択してください", "たなおろしをおこなうこんてなをせんたくしてください"}, true, true} },
			{ {&table2column1stBox, &table2column2ndBox}, {.is_visible = true} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		{
			{ {&table2column1st, &table2column2nd}, {
				.column_headers = std::vector<UiText>{
					{"コンテナ\n  番号", "こんてな\nばんごう"},
					{"分類", "ぶんるい"},
					{"メーカー", "メーカー"},
					{"型番", "かたばん"},
					{"材質", "ざいしつ"},
					{"　最終\n棚卸し日時", "  　さいしゅう\nたなおろしにちじ"}
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

	//=========================================================================
	// 初期状態の設定
	//=========================================================================
	set_mode(GuiMode::RETRIEVE);	// 最初は取り出しモード
	set_app_state(AppState::WAITING_FOR_USER_LOGIN);	// 最初の状態を設定
	refresh_ui(true);	// 最初は全ウィジェットを強制的に再描画
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
			// m_widget_statesから既存のWidgetStateUpdaterのコピーを取得
			auto updater = m_widget_states.at(widget);
			// stateメンバーだけを新しい状態に更新
			updater.state.merge(widget_state.state);
			// 更新したWidgetStateUpdaterをマップに書き戻す
			m_widget_states.insert_or_assign(widget, updater);
		}
	}

	// 3. m_table_statesを更新する
	for (const auto& table_state : state_config.table_states) {
		for (const auto& table : table_state.tables) {
			// m_table_statesから現在のTableStateのコピーを取得
			auto current_state = m_table_states.at(table);
			// 新しい状態に更新
			current_state.merge(table_state.state);
			// 更新したTableStateをマップに書き戻す
			m_table_states.insert_or_assign(table, current_state);
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
	refresh_ui(true); // 現在の状態のままUIテキストを更新
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
	try {
		auto state_config = g_app_state_configs.at(m_app_state);
		if (state_config.on_true_button_clicked) {
			state_config.on_true_button_clicked(this);
		}
	} catch (const std::out_of_range& e) {
		fmt::print("Error: 未定義の状態でTrueボタンがクリックされました (AppState={})\n", static_cast<int>(m_app_state));
		// 例外を無視して処理を続行
	}
}

void MainWindow::falseButton_on_clicked() {
	try {
		auto state_config = g_app_state_configs.at(m_app_state);
		if (state_config.on_false_button_clicked) {
			state_config.on_false_button_clicked(this);
		}
	} catch (const std::out_of_range& e) {
		fmt::print("Error: 未定義の状態でFalseボタンがクリックされました (AppState={})\n", static_cast<int>(m_app_state));
		// 例外を無視して処理を続行
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
