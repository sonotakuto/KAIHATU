/**
 * @file tool_io_gui.cpp
 * @brief tool_io_gui.hppで宣言されたクラスのメンバ関数の実装ファイル。
 */
#include "./SimpleGtk_extension.hpp"
#include "./includes.hpp"
#include "./db_client.hpp"
#include "./tool_io_gui.hpp"

namespace gui {

//=============================================================================
// 静的メンバ変数の定義
//=============================================================================

const std::map<MainWindow::TableCellState, GdkRGBA> MainWindow::g_table_row_colors = {
	{ TableCellState::NONE					, TABLE_BGCOLOR_NORMAL				},	// NORMAL と同色
	{ TableCellState::NORMAL				, TABLE_BGCOLOR_NORMAL				},	// 通常背景色
	{ TableCellState::HIGHLIGHT				, TABLE_BGCOLOR_HIGHLIGHT			},	// 強調表示背景色
	{ TableCellState::FADED					, TABLE_BGCOLOR_FADED				},	// 薄く表示された背景色
	{ TableCellState::SELECTED				, TABLE_BGCOLOR_HIGHLIGHT			},	// HIGHLIGHT と同色
	{ TableCellState::RECONCILE_MATCH		, TABLE_BGCOLOR_RECONCILE_MATCH		},	// 棚卸し一致行背景色
	{ TableCellState::RECONCILE_MISMATCH	, TABLE_BGCOLOR_RECONCILE_MISMATCH	},	// 棚卸し不一致行背景色
	{ TableCellState::RECONCILE_UNREAD		, TABLE_BGCOLOR_RECONCILE_UNREAD	}	// 棚卸し未読行背景色
};

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
	if (other.text.has_value() && this->text != other.text) {
		this->text = other.text;
		changed = true;
	}
	if (other.is_sensitive.has_value() && this->is_sensitive != other.is_sensitive) {
		this->is_sensitive = other.is_sensitive;
		changed = true;
	}
	if (other.is_visible.has_value() && this->is_visible != other.is_visible) {
		this->is_visible = other.is_visible;
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
	if (other.column_headers.has_value() && this->column_headers != other.column_headers) {
		this->column_headers = other.column_headers;
		changed = true;
	}
	if (other.is_sensitive.has_value() && this->is_sensitive != other.is_sensitive) {
		this->is_sensitive = other.is_sensitive;
		changed = true;
	}
	if (other.is_visible.has_value() && this->is_visible != other.is_visible) {
		this->is_visible = other.is_visible;
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
	storingToolListstore(
		storingToolTable,
		{ G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA },
		[](sgex::ListStore* store, GtkTreeIter* iter, const MstToolInfo& row) {
			store->set(iter,
				0, row.category.c_str(),
				1, row.maker.c_str(),
				2, row.name.c_str(),
				3, row.material.c_str(),
				4, &TABLE_BGCOLOR_NORMAL	// TODO: コンパイルできる所まで進めたら動作確認
			);
		}
	),
	filterButton(glade, "filterButton"),
	containerLabel(glade, "containerLabel"),
	containerEntry(glade, "containerEntry"),
	// テーブル部
	table_stack(glade, "table_stack"),
	table3column1stBox(glade, "table3column1stBox"),
	table3column1st(glade, "table3column1st"),
	table3column1stSelection(table3column1st.get_selection()),
	liststore3column1st(
		table3column1st,
		{ G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA, GDK_TYPE_RGBA },
		[this](sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) { this->main_ListStore_setter(store, iter, row); }
	),
	table3column2ndBox(glade, "table3column2ndBox"),
	table3column2nd(glade, "table3column2nd"),
	table3column2ndSelection(table3column2nd.get_selection()),
	liststore3column2nd(
		table3column2nd,
		{ G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA, GDK_TYPE_RGBA },
		[this](sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) { this->main_ListStore_setter(store, iter, row); }
	),
	table3column3rdBox(glade, "table3column3rdBox"),
	table3column3rd(glade, "table3column3rd"),
	table3column3rdSelection(table3column3rd.get_selection()),
	liststore3column3rd(
		table3column3rd,
		{ G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA, GDK_TYPE_RGBA },
		[this](sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) { this->main_ListStore_setter(store, iter, row); }
	),
	liststore3columnAll(DISPLAY_ROWS_PER_COLUMN),
	table2column1stBox(glade, "table2column1stBox"),
	table2column1st(glade, "table2column1st"),
	table2column1stSelection(table2column1st.get_selection()),
	liststore2column1st(
		table2column1st,
		{ G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA, GDK_TYPE_RGBA },
		[this](sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) { this->main_ListStore_setter(store, iter, row); }
	),
	table2column2ndBox(glade, "table2column2ndBox"),
	table2column2nd(glade, "table2column2nd"),
	table2column2ndSelection(table2column2nd.get_selection()),
	liststore2column2nd(
		table2column2nd,
		{ G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA, GDK_TYPE_RGBA },
		[this](sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) { this->main_ListStore_setter(store, iter, row); }
	),
	liststore2columnAll(DISPLAY_ROWS_PER_COLUMN),
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
	confirmRegroundDialog(glade, "confirmRegroundDialog"),
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
	confirmRegroundLabel(glade, "confirmRegroundLabel"),
	notRegroundButton(glade, "notRegroundButton"),
	regroundButton(glade, "regroundButton"),
	confirmRegroundOkButton(glade, "confirmRegroundOkButton"),
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
	m_password_authenticated(false),
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

	this->fullscreen();	// フルスクリーンで表示

	this->liststore3columnAll.add_model({&liststore3column1st, &liststore3column2nd, &liststore3column3rd});
	this->liststore2columnAll.add_model({&liststore2column1st, &liststore2column2nd});

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
	table3column1stSelection.on_changed([this]() { this->mainTableSelection_on_changed(); });
	table3column2ndSelection.on_changed([this]() { this->mainTableSelection_on_changed(); });
	table3column3rdSelection.on_changed([this]() { this->mainTableSelection_on_changed(); });
	table2column1stSelection.on_changed([this]() { this->mainTableSelection_on_changed(); });
	table2column2ndSelection.on_changed([this]() { this->mainTableSelection_on_changed(); });
	prevButton.on_clicked([this]() { this->prevButton_on_clicked(); });
	nextButton.on_clicked([this]() { this->nextButton_on_clicked(); });
	qrCodeEntry.on_activate([this]() { this->qrCodeEntry_on_activate(); });
	falseButton.on_clicked([this]() { this->falseButton_on_clicked(); });
	trueButton.on_clicked([this]() { this->trueButton_on_clicked(); });
	logoutNoButton.on_clicked([this]() { this->logoutDialog.response(GTK_RESPONSE_NO); });
	logoutYesButton.on_clicked([this]() { this->logoutDialog.response(GTK_RESPONSE_YES); });
	passwordEntryDialog.on_show([this]() { this->passwordEntryDialog_on_show(); });
	passwordCancelButton.on_clicked([this]() { passwordEntryDialog.response(GTK_RESPONSE_CANCEL); });
	passwordEnterButton.on_clicked([this]() { passwordEntryDialog.response(GTK_RESPONSE_OK); });
	passwordEntry.on_activate([this]() { passwordEntryDialog.response(GTK_RESPONSE_OK); });	// 確定ボタンがクリックされた時と同様
	regroundButton.on_toggled([this]() { this->regroundButton_on_toggled(); });
	notRegroundButton.on_toggled([this]() { this->notRegroundButton_on_toggled(); });
	confirmRegroundOkButton.on_clicked([this]() { confirmRegroundDialog.response(GTK_RESPONSE_OK); });

	//=========================================================================
	// ウィジェットの状態を保持するマップの構築
	//=========================================================================

	// --- 「取り出し」トグルボタン ---
	m_widget_states.try_emplace(&retrieveToggleButton, WidgetStateUpdater{
		{UiText{"取り出し", "とりだし"}, false, true},
		[this](const WidgetState& state){
			this->retrieveToggleButton.set_label(get_display_text(state.text.value()).c_str());
			this->retrieveToggleButton.set_sensitive(state.is_sensitive.value());
			this->retrieveToggleButton.set_visible(state.is_visible.value());
		}
	});

	// --- 「補充」トグルボタン ---
	m_widget_states.try_emplace(&storeToggleButton, WidgetStateUpdater{
		{UiText{"補充", "ほじゅう"}, false, true},
		[this](const WidgetState& state){
			this->storeToggleButton.set_label(get_display_text(state.text.value()).c_str());
			this->storeToggleButton.set_sensitive(state.is_sensitive.value());
			this->storeToggleButton.set_visible(state.is_visible.value());
		}
	});

	// --- 「棚卸し」トグルボタン ---
	m_widget_states.try_emplace(&reconcileToggleButton, WidgetStateUpdater{
		{UiText{"棚卸し", "たなおろし"}, false, true},
		[this](const WidgetState& state){
			this->reconcileToggleButton.set_label(get_display_text(state.text.value()).c_str());
			this->reconcileToggleButton.set_sensitive(state.is_sensitive.value());
			this->reconcileToggleButton.set_visible(state.is_visible.value());
		}
	});

	// --- 「かな/漢字」ボタン ---
	m_widget_states.try_emplace(&kanaKanjiButton, WidgetStateUpdater{
		{UiText{"かな/漢字", "かな/漢字"}, true, true},
		[this](const WidgetState& state){
			this->kanaKanjiButton.set_label(get_display_text(state.text.value()).c_str());
			this->kanaKanjiButton.set_sensitive(state.is_sensitive.value());
			this->kanaKanjiButton.set_visible(state.is_visible.value());
		}
	});

	// --- 「ログアウト」ボタン ---
	m_widget_states.try_emplace(&logoutButton, WidgetStateUpdater{
		{UiText{"ログアウト", "ログアウト"}, false, true},
		[this](const WidgetState& state){
			this->logoutButton.set_label(get_display_text(state.text.value()).c_str());
			this->logoutButton.set_sensitive(state.is_sensitive.value());
			this->logoutButton.set_visible(state.is_visible.value());
		}
	});

	// --- ユーザーIDエントリ ---
	m_widget_states.try_emplace(&userIdEntry, WidgetStateUpdater{
		{nullopt, true, true},
		[this](const WidgetState& state){
			this->userIdEntry.set_sensitive(state.is_sensitive.value());
			this->userIdEntry.set_visible(state.is_visible.value());
		}
	});

	// --- 「ライン番号」ラベル ---
	m_widget_states.try_emplace(&lineLabel, WidgetStateUpdater{
		{UiText{"ライン番号：", "ラインばんごう："}, true, true},
		[this](const WidgetState& state){
			this->lineLabel.set_label(get_display_text(state.text.value()).c_str());
			// ラベルのsensitiveは通常操作しない
			this->lineLabel.set_visible(state.is_visible.value());
		}
	});

	// --- 「プログラム番号」ラベル ---
	m_widget_states.try_emplace(&programLabel, WidgetStateUpdater{
		{UiText{"プログラム番号：", "プログラムばんごう："}, true, true},
		[this](const WidgetState& state){
			this->programLabel.set_label(get_display_text(state.text.value()).c_str());
			this->programLabel.set_visible(state.is_visible.value());
		}
	});

	// --- 「絞り込み」ボタン ---
	m_widget_states.try_emplace(&filterButton, WidgetStateUpdater{
		{UiText{"絞り込み", "しぼりこみ"}, true, true},
		[this](const WidgetState& state){
			this->filterButton.set_label(get_display_text(state.text.value()).c_str());
			this->filterButton.set_sensitive(state.is_sensitive.value());
			this->filterButton.set_visible(state.is_visible.value());
		}
	});

	// --- 「コンテナ番号」ラベル ---
	m_widget_states.try_emplace(&containerLabel, WidgetStateUpdater{
		{UiText{"コンテナ番号：", "コンテナばんごう："}, true, true},
		[this](const WidgetState& state){
			this->containerLabel.set_label(get_display_text(state.text.value()).c_str());
			this->containerLabel.set_visible(state.is_visible.value());
		}
	});

	// --- メッセージラベル ---
	m_widget_states.try_emplace(&messageLabel, WidgetStateUpdater{
		{UiText{"", ""}, true, true},
		[this](const WidgetState& state){
			this->messageLabel.set_label(get_display_text(state.text.value()).c_str());
			this->messageLabel.set_visible(state.is_visible.value());
		}
	});

	// --- 3列表示テーブルのボックス (1) ---
	m_widget_states.try_emplace(&table3column1stBox, WidgetStateUpdater{
		{nullopt, nullopt, false},
		[this](const WidgetState& state){
			this->table3column1stBox.set_visible(state.is_visible.value());
		}
	});

	// --- 3列表示テーブルのボックス (2) ---
	m_widget_states.try_emplace(&table3column2ndBox, WidgetStateUpdater{
		{nullopt, nullopt, false},
		[this](const WidgetState& state){
			this->table3column2ndBox.set_visible(state.is_visible.value());
		}
	});

	// --- 3列表示テーブルのボックス (3) ---
	m_widget_states.try_emplace(&table3column3rdBox, WidgetStateUpdater{
		{nullopt, nullopt, false},
		[this](const WidgetState& state){
			this->table3column3rdBox.set_visible(state.is_visible.value());
		}
	});

	// --- 2列表示テーブルのボックス (1) ---
	m_widget_states.try_emplace(&table2column1stBox, WidgetStateUpdater{
		{nullopt, nullopt, false},
		[this](const WidgetState& state){
			this->table2column1stBox.set_visible(state.is_visible.value());
		}
	});

	// --- 2列表示テーブルのボックス (2) ---
	m_widget_states.try_emplace(&table2column2ndBox, WidgetStateUpdater{
		{nullopt, nullopt, false},
		[this](const WidgetState& state){
			this->table2column2ndBox.set_visible(state.is_visible.value());
		}
	});

	// --- テーブルのページネーション(ページ送り)UIを格納するボックス ---
	m_widget_states.try_emplace(&paginationBox, WidgetStateUpdater{
		{nullopt, nullopt, false},
		[this](const WidgetState& state){
			this->paginationBox.set_visible(state.is_visible.value());
		}
	});

	// --- QRコード受付UIのボックス ---
	m_widget_states.try_emplace(&qrCodeEntryBox, WidgetStateUpdater{
		{nullopt, nullopt, false},
		[this](const WidgetState& state){
			this->qrCodeEntryBox.set_visible(state.is_visible.value());
		}
	});

	// --- 「QRコード受付」ラベル ---
	m_widget_states.try_emplace(&qrCodeEntryLabel, WidgetStateUpdater{
		{UiText{"QRコード受付：", "QRコード うけつけ："}, true, true},
		[this](const WidgetState& state){
			this->qrCodeEntryLabel.set_label(get_display_text(state.text.value()).c_str());
			this->qrCodeEntryLabel.set_visible(state.is_visible.value());
		}
	});

	// --- QRコードを入力するエントリ ---
	m_widget_states.try_emplace(&qrCodeEntry, WidgetStateUpdater{
		{nullopt, false, true},
		[this](const WidgetState& state){
			this->qrCodeEntry.set_sensitive(state.is_sensitive.value());
			this->qrCodeEntry.set_visible(state.is_visible.value());
		}
	});

	// --- true/falseボタン（ラベルは動的に変わるため空） ---
	m_widget_states.try_emplace(&falseButton, WidgetStateUpdater{
		{UiText{"", ""}, true, true},
		[this](const WidgetState& state){
			this->falseButton.set_label(get_display_text(state.text.value()).c_str());
			this->falseButton.set_sensitive(state.is_sensitive.value());
			this->falseButton.set_visible(state.is_visible.value());
		}
	});
	m_widget_states.try_emplace(&trueButton, WidgetStateUpdater{
		{UiText{"", ""}, true, true},
		[this](const WidgetState& state){
			this->trueButton.set_label(get_display_text(state.text.value()).c_str());
			this->trueButton.set_sensitive(state.is_sensitive.value());
			this->trueButton.set_visible(state.is_visible.value());
		}
	});

	// --- インサート枚数確認ダイアログ ---
	m_widget_states.try_emplace(&confirmInsertCountLabel, WidgetStateUpdater{
		{UiText{"使用するインサートの枚数を\n入力してください", "しようする インサートの まいすうを\nにゅうりょく してください"}, true, true},
		[this](const WidgetState& state){
			this->confirmInsertCountLabel.set_label(get_display_text(state.text.value()).c_str());
			this->confirmInsertCountLabel.set_visible(state.is_visible.value());
		}
	});
	m_widget_states.try_emplace(&insertCountOkButton, WidgetStateUpdater{
		{UiText{"OK", "OK"}, true, true},
		[this](const WidgetState& state){
			this->insertCountOkButton.set_label(get_display_text(state.text.value()).c_str());
			this->insertCountOkButton.set_sensitive(state.is_sensitive.value());
			this->insertCountOkButton.set_visible(state.is_visible.value());
		}
	});

	// --- 再研磨確認ダイアログ ---
	m_widget_states.try_emplace(&confirmRegroundLabel, WidgetStateUpdater{
		{UiText{"一度取り出された工具を検出しました。\n再研磨された工具ですか？", "いちど とりだされた こうぐを けんしゅつしました。\nさいけんま された こうぐですか？"}, true, true},
		[this](const WidgetState& state){
			this->confirmRegroundLabel.set_label(get_display_text(state.text.value()).c_str());
			this->confirmRegroundLabel.set_visible(state.is_visible.value());
		}
	});
	m_widget_states.try_emplace(&confirmRegroundOkButton, WidgetStateUpdater{
		{UiText{"OK", "OK"}, true, true},
		[this](const WidgetState& state){
			this->confirmRegroundOkButton.set_label(get_display_text(state.text.value()).c_str());
			this->confirmRegroundOkButton.set_sensitive(state.is_sensitive.value());
			this->confirmRegroundOkButton.set_visible(state.is_visible.value());
		}
	});
	m_widget_states.try_emplace(&notRegroundButton, WidgetStateUpdater{
		{UiText{"いいえ", "いいえ"}, true, true},
		[this](const WidgetState& state){
			this->notRegroundButton.set_label(get_display_text(state.text.value()).c_str());
			this->notRegroundButton.set_sensitive(state.is_sensitive.value());
			this->notRegroundButton.set_visible(state.is_visible.value());
		}
	});
	m_widget_states.try_emplace(&regroundButton, WidgetStateUpdater{
		{UiText{"はい", "はい"}, true, true},
		[this](const WidgetState& state){
			this->regroundButton.set_label(get_display_text(state.text.value()).c_str());
			this->regroundButton.set_sensitive(state.is_sensitive.value());
			this->regroundButton.set_visible(state.is_visible.value());
		}
	});

	// --- ログアウト確認ダイアログ ---
	m_widget_states.try_emplace(&logoutNoButton, WidgetStateUpdater{
		{UiText{"いいえ", "いいえ"}, true, true},
		[this](const WidgetState& state){
			this->logoutNoButton.set_label(get_display_text(state.text.value()).c_str());
			this->logoutNoButton.set_sensitive(state.is_sensitive.value());
			this->logoutNoButton.set_visible(state.is_visible.value());
		}
	});
	m_widget_states.try_emplace(&logoutYesButton, WidgetStateUpdater{
		{UiText{"はい", "はい"}, true, true},
		[this](const WidgetState& state){
			this->logoutYesButton.set_label(get_display_text(state.text.value()).c_str());
			this->logoutYesButton.set_sensitive(state.is_sensitive.value());
			this->logoutYesButton.set_visible(state.is_visible.value());
		}
	});

	// --- パスワード入力ダイアログ ---
	m_widget_states.try_emplace(&passwordEntryLabel, WidgetStateUpdater{
		{UiText{"パスワードを\n入力してください", "パスワードを\nにゅうりょく してください"}, true, true},
		[this](const WidgetState& state){
			this->passwordEntryLabel.set_label(get_display_text(state.text.value()).c_str());
			this->passwordEntryLabel.set_visible(state.is_visible.value());
		}
	});
	m_widget_states.try_emplace(&passwordCancelButton, WidgetStateUpdater{
		{UiText{"キャンセル", "キャンセル"}, true, true},
		[this](const WidgetState& state){
			this->passwordCancelButton.set_label(get_display_text(state.text.value()).c_str());
			this->passwordCancelButton.set_sensitive(state.is_sensitive.value());
			this->passwordCancelButton.set_visible(state.is_visible.value());
		}
	});
	m_widget_states.try_emplace(&passwordEnterButton, WidgetStateUpdater{
		{UiText{"確定", "かくてい"}, true, true},
		[this](const WidgetState& state){
			this->passwordEnterButton.set_label(get_display_text(state.text.value()).c_str());
			this->passwordEnterButton.set_sensitive(state.is_sensitive.value());
			this->passwordEnterButton.set_visible(state.is_visible.value());
		}
	});

	// --- 棚卸し絞り込みダイアログ ---
	m_widget_states.try_emplace(&reconcileToolTypeFilterLabel, WidgetStateUpdater{
		{UiText{"分類", "ぶんるい"}, true, true},
		[this](const WidgetState& state){
			this->reconcileToolTypeFilterLabel.set_label(get_display_text(state.text.value()).c_str());
			this->reconcileToolTypeFilterLabel.set_visible(state.is_visible.value());
		}
	});
	m_widget_states.try_emplace(&reconcilePeriodFilterLabel, WidgetStateUpdater{
		{UiText{"最終棚卸し日時", "さいしゅうたなおろしにちじ"}, true, true},
		[this](const WidgetState& state){
			this->reconcilePeriodFilterLabel.set_label(get_display_text(state.text.value()).c_str());
			this->reconcilePeriodFilterLabel.set_visible(state.is_visible.value());
		}
	});
	m_widget_states.try_emplace(&reconcileFilterEnterButton, WidgetStateUpdater{
		{UiText{"決定", "けってい"}, true, true},
		[this](const WidgetState& state){
			this->reconcileFilterEnterButton.set_label(get_display_text(state.text.value()).c_str());
			this->reconcileFilterEnterButton.set_sensitive(state.is_sensitive.value());
			this->reconcileFilterEnterButton.set_visible(state.is_visible.value());
		}
	});
	m_widget_states.try_emplace(&reconcileFilterResetButton, WidgetStateUpdater{
		{UiText{"期間リセット", "きかんリセット"}, true, true},
		[this](const WidgetState& state){
			this->reconcileFilterResetButton.set_label(get_display_text(state.text.value()).c_str());
			this->reconcileFilterResetButton.set_sensitive(state.is_sensitive.value());
			this->reconcileFilterResetButton.set_visible(state.is_visible.value());
		}
	});

	// --- 警告ダイアログ（ラベルは動的に変わるため空） ---
	m_widget_states.try_emplace(&warningLabel, WidgetStateUpdater{
		{UiText{"", ""}, true, true},
		[this](const WidgetState& state){
			this->warningLabel.set_label(get_display_text(state.text.value()).c_str());
			this->warningLabel.set_visible(state.is_visible.value());
		}
	});
	// TODO: 他のかな/漢字切替や、状態遷移での表示切替が必要なウィジェットも同様に追加

	//=========================================================================
	// 各テーブルのカラムヘッダー表示テキストを保持するマップの構築
	//=========================================================================

	// --- 補充する工具の情報を表示するテーブル ---
	m_table_states.try_emplace(&storingToolTable, TableState{
		std::vector<UiText>{
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"}
		}, false, true
	});

	// --- 3列表示テーブル ---
	m_table_states.try_emplace(&table3column1st, TableState{
		std::vector<UiText>{
			{"ツール\n  番号", "  ツール\nばんごう"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"在庫数", "ざいこ\n  すう"}
		}, true, true
	});
	m_table_states.try_emplace(&table3column2nd, TableState{
		std::vector<UiText>{
			{"ツール\n  番号", "  ツール\nばんごう"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"在庫数", "ざいこ\n  すう"}
		}, true, true
	});
	m_table_states.try_emplace(&table3column3rd, TableState{
		std::vector<UiText>{
			{"ツール\n  番号", "  ツール\nばんごう"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"在庫数", "ざいこ\n  すう"}
		}, true, true
	});

	// --- 2列表示テーブル ---
	m_table_states.try_emplace(&table2column1st, TableState{
		std::vector<UiText>{
			{"ID", "ID"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"再研磨\n  回数", "さいけんま\n  かいすう"}
		}, true, true
	});
	m_table_states.try_emplace(&table2column2nd, TableState{
		std::vector<UiText>{
			{"ID", "ID"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"再研磨\n  回数", "さいけんま\n  かいすう"}
		}, true, true
	});

	//=========================================================================
	// アプリケーション状態とウィジェット設定を紐付けるマップの構築
	//=========================================================================

	// --- 状態：ユーザーログイン待ち - 移行 ---
	g_app_state_configs.try_emplace(AppState::NONE, WidgetStateConfig{
		.on_enter_state	= [](MainWindow* thiz){ thiz->set_app_state(AppState::WAITING_FOR_USER_LOGIN); }
	});

	// --- 状態：ユーザーログイン待ち ---
	g_app_state_configs.try_emplace(AppState::WAITING_FOR_USER_LOGIN, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&retrieveToggleButton, &storeToggleButton, &reconcileToggleButton, &logoutButton}, {.is_sensitive = false, .is_visible = true}},
			{ {&userIdEntry}, {.is_sensitive = true, .is_visible = true} },
			{ {&messageLabel}, {.text = UiText{"名札のQRコードを\nQRコードリーダーにかざしてください", "なふだの QRコードを\nQRコードリーダーに かざしてください"}} },
			{ {&paginationBox}, {.is_visible = false} },
			{ {&falseButton, &trueButton}, {UiText{"", ""}, false, true} }
		},
		.table_states = {
			{ {&table3column1st, &table3column2nd, &table3column3rd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.operationStackPageName	= "pageNone",	// 初期ページ
		.tableStackPageName		= "pageNone",	// 初期ページ
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state	= [](MainWindow* thiz){ thiz->on_enter_waiting_for_user_login(); }
	});

	// --- 状態：取り出し - 移行 ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE, WidgetStateConfig{
		.on_enter_state	= [](MainWindow* thiz){ thiz->set_app_state(AppState::RETRIEVE_WAITING_FOR_PROGRAM_QR); }
	});

	// --- 状態：取り出し - プログラムQR待ち ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE_WAITING_FOR_PROGRAM_QR, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&logoutButton}, {.is_sensitive = true} },
			{ {&messageLabel}, {.text = UiText{"プログラム割当一覧表のQRコードを\nQRコードリーダーにかざしてください", "プログラムわりあていちらんひょうの QRコードを\nQRコードリーダーにかざしてください"}} },
			{ {&paginationBox}, {.is_visible = false} },
			{ {&trueButton, &falseButton}, {UiText{"", ""}, false, true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.operationStackPageName = "retrieveOperationPage",
		.tableStackPageName		= "pageNone",	// 初期ページ
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_retrieve_waiting_for_program_qr(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_retrieve_waiting_for_program_qr(); }
	});

	// --- 状態：取り出し - 工具選択中 ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE_SELECTING_TOOLS, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"取り出す工具を選択してください", "とりだす こうぐを せんたく してください"}} },
			{ {&paginationBox}, {.is_visible = true} },
			{ {&trueButton}, {UiText{"確定", "かくてい"}, true, true} },
			{ {&falseButton}, {UiText{"全選択解除", "ぜんせんたくかいじょ"}, true, true} }
		},
		.table_states = {
			{ {&table2column1st, &table2column2nd}, {
				.column_headers = std::vector<UiText>{
					{"ツール\n  番号", "  ツール\nばんごう"},
					{"分類", "ぶんるい"},
					{"メーカー", "メーカー"},
					{"型番", "かたばん"},
					{"材質", "ざいしつ"},
					{"在庫数", "ざいこ\n  すう"}
				}, .is_sensitive = true
			} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName		= "table2column",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state			= [](MainWindow* thiz){ /* TODO: DBから取得した工具リストをテーブルに表示する */ },
		.on_falseButton_clicked	= [](MainWindow* thiz){ /* TODO: テーブルの全選択を解除する処理 */ },
		.on_trueButton_clicked	= [](MainWindow* thiz){ thiz->set_app_state(AppState::RETRIEVE_CONFIRMING_SELECTION); }
	});

	// --- 状態：取り出し - 選択内容の確認中 ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE_CONFIRMING_SELECTION, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"選択した工具は以下の通りです\nよろしければ「搬出」を押してください", "せんたくした こうぐは いかのとおりです..."}} },
			{ {&trueButton}, {UiText{"搬出", "はんしゅつ"}, true, true} },
			{ {&falseButton}, {UiText{"キャンセル", "キャンセル"}, true, true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName		= "table2column",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state			= [](MainWindow* thiz){ /* TODO: 選択された工具のみをテーブルに表示する */ },
		.on_falseButton_clicked	= [](MainWindow* thiz){ thiz->set_app_state(AppState::RETRIEVE_SELECTING_TOOLS); },	// 前の状態に戻る
		.on_trueButton_clicked	= [](MainWindow* thiz){ /* TODO: 搬出処理を開始し、次の状態へ */ }
	});

	// --- 状態：取り出し - コンテナ搬出待ち ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE_CONTAINER_RETRIEVING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&retrieveToggleButton, &reconcileToggleButton, &logoutButton}, {.is_sensitive = false} },
			{ {&messageLabel}, {.text = UiText{"コンテナを搬出しています\nしばらくお待ち下さい", "コンテナを はんしゅつ しています\nしばらく おまちください"}} },
			{ {&falseButton, &trueButton}, {UiText{"", ""}, false, true} }
		},
		.table_states = {
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName		= "table2column",
		//TODO --- 状態の「振る舞い」を定義 ---
	});

	// --- 状態：取り出し - コンテナ搬出完了 ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE_CONTAINER_RETRIEVED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"コンテナが届きました\n扉を開けてコンテナを取り出してください", "コンテナが とどきました\nとびらを あけて コンテナを とりだしてください"}} },
		},
		//TODO --- 状態の「振る舞い」を定義 ---
	});

	// --- 状態：取り出し - 実際に補充を行う - 工具読み取り前 ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE_READY_FOR_READING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"コンテナから工具を取り出して、工具のQRコードを\nQRコードリーダーで読み取ってください", "コンテナから こうぐを とりだして、こうぐの QRコードを\nQRコードリーダーで よみとってください"}} },
			{ {&qrCodeEntry}, {.is_sensitive = true} },
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName	= "pageNone",
		// --- 状態の「振る舞い」を定義 ---
		.on_prepare_state	= [](MainWindow* thiz){
			thiz->update_widget_state(&(thiz->falseButton), {
				.text = ((true)	// TODO: 取り出す工具の中で最後だった場合とそうでない場合の分岐処理を追加
					? (UiText{"", ""})
					: (UiText{"終了", "しゅうりょう"})
				),
				.is_sensitive = false,
				.is_visible = true
			});
			thiz->update_widget_state(&(thiz->trueButton), {
				.text = ((true)	// TODO: 取り出す工具の中で最後だった場合とそうでない場合の分岐処理を追加
					? (UiText{"次へ", "つぎへ"})
					: (UiText{"続ける", "つづける"})
				),
				.is_sensitive = false,
				.is_visible = true
			});
			thiz->update_table_state({&(thiz->table2column1st), &(thiz->table2column2nd)}, {
				.column_headers = std::vector<UiText>{
					{"ID", "ID"},
					{"分類", "ぶんるい"},
					{"メーカー", "メーカー"},
					{"型番", "かたばん"},
					{"材質", "ざいしつ"},
					((thiz->m_is_insert_operation)
						? (UiText{"ケース内\n　個数", "ケースない\n　こすう"})
						: (UiText{"再研磨\n  回数", "さいけんま\n  かいすう"})
					)
				}, .is_sensitive = true
			});
		},
		//TODO 他の振る舞いを定義
	});

	// --- 状態：取り出し - 実際に補充を行う - 工具読み取り後 ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE_READ_COMPLETED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&paginationBox}, {.is_visible = true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName	= "table2column",
		// --- 状態の「振る舞い」を定義 ---
		.on_prepare_state	= [](MainWindow* thiz){
			thiz->update_widget_state(&(thiz->messageLabel), {
				.text = ((true)	// TODO: 取り出す工具の中で最後だった場合とそうでない場合の分岐処理を追加
					? (UiText{"取り出し終わったらコンテナを戻して\n扉を閉めてください", "とりだしおわったら コンテナを もどして\nとびらを しめてください"})
					: (UiText{"取り出し終わったら扉を閉めて\n「次へ」を押してください", "とりだしおわったら とびらを しめて\n「つぎへ」を おしてください"})
				)
			});
		},
		//TODO 他の振る舞いを定義
	});

	// --- 状態：補充 - 移行 ---
	g_app_state_configs.try_emplace(AppState::STORE, WidgetStateConfig{
		.on_enter_state	= [](MainWindow* thiz){ thiz->set_app_state(AppState::STORE_WAITING_FOR_TOOL_QR); }
	});

	// --- 状態：補充 - 移行 ---
	g_app_state_configs.try_emplace(AppState::STORE, WidgetStateConfig{
		.on_enter_state	= [](MainWindow* thiz){ thiz->set_app_state(AppState::STORE_WAITING_FOR_TOOL_QR); }
	});

	// --- 状態：補充 - 工具QR待ち ---
	g_app_state_configs.try_emplace(AppState::STORE_WAITING_FOR_TOOL_QR, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&logoutButton}, {.is_sensitive = true} },
			{ {&messageLabel}, {.text = UiText{"補充する工具のQRコードを\nQRコードリーダーにかざしてください", "ほじゅうする こうぐの QRコードを\nQRコードリーダーに かざしてください"}} },
			{ {&paginationBox}, {.is_visible = false} },
			{ {&qrCodeEntry}, {.is_sensitive = true} },
			{ {&falseButton, &trueButton}, {UiText{"", ""}, false, true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.operationStackPageName	= "storeOperationPage",
		.tableStackPageName		= "pageNone",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_waiting_for_tool_qr(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_store_waiting_for_tool_qr(); }
	});

	// --- 状態：補充 - 読み取った工具の確認 ---
	g_app_state_configs.try_emplace(AppState::STORE_CONFIRMING_SELECTION, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"補充する工具に間違いがなければ\n「搬出」を押してください", "ほじゅうする こうぐに まちがいが なければ\n「はんしゅつ」を おしてください"}} },
			{ {&qrCodeEntry}, {.is_sensitive = false} },
			{ {&falseButton}, {UiText{"キャンセル", "キャンセル"}, true, true} },
			{ {&trueButton}, {UiText{"搬出", "はんしゅつ"}, true, true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName	= "pageNone",
		// --- 状態の「振る舞い」を定義 ---
		.on_falseButton_clicked	= [](MainWindow* thiz){ thiz->set_app_state(AppState::STORE_WAITING_FOR_TOOL_QR); },	// 前の状態に戻る
		.on_trueButton_clicked	= [](MainWindow* thiz){ thiz->set_app_state(AppState::STORE_CONTAINER_RETRIEVING); }	// コンテナ搬出処理へ
	});

	// --- 状態：補充 - コンテナ搬出待ち ---
	g_app_state_configs.try_emplace(AppState::STORE_CONTAINER_RETRIEVING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&retrieveToggleButton, &reconcileToggleButton, &logoutButton}, {.is_sensitive = false} },
			{ {&messageLabel}, {.text = UiText{"コンテナを搬出しています\nしばらくお待ち下さい", "コンテナを はんしゅつ しています\nしばらく おまちください"}} },
			{ {&falseButton, &trueButton}, {UiText{"", ""}, false, true} }
		},
		.table_states = {
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName	= "pageNone",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state		= [](MainWindow* thiz){ thiz->on_enter_store_container_retrieving(); },
		.on_plc_notified	= [](MainWindow* thiz, plc::Notification notification){ thiz->on_plc_store_container_retrieving(notification); }
	});

	// --- 状態：補充 - コンテナ搬出完了 ---
	g_app_state_configs.try_emplace(AppState::STORE_CONTAINER_RETRIEVED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"コンテナが届きました\n扉を開けてコンテナを取り出してください", "コンテナが とどきました\nとびらを あけて コンテナを とりだしてください"}} },
		},
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state		= [](MainWindow* thiz){ thiz->on_enter_store_container_retrieved(); },
		.on_plc_notified	= [](MainWindow* thiz, plc::Notification notification){ thiz->on_plc_store_container_retrieved(notification); }
	});

	// --- 状態：補充 - 実際に補充を行う - 工具読み取り前 ---
	g_app_state_configs.try_emplace(AppState::STORE_READY_FOR_READING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"工具のQRコードを QRコードリーダーで\n読み取ってからコンテナに収納してください", "こうぐの QRコードを QRコードリーダーで\nよみとってから コンテナに しゅうのうしてください"}} },
			{ {&qrCodeEntry}, {.is_sensitive = true} },
			{ {&falseButton}, {UiText{"終了", "しゅうりょう"}, false, true} },
			{ {&trueButton}, {UiText{"続ける", "つづける"}, false, true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName	= "pageNone",
		// --- 状態の「振る舞い」を定義 ---
		.on_prepare_state	= [](MainWindow* thiz){
			thiz->update_table_state({&(thiz->table2column1st), &(thiz->table2column2nd)}, {
				.column_headers = std::vector<UiText>{
					{"ID", "ID"},
					{"分類", "ぶんるい"},
					{"メーカー", "メーカー"},
					{"型番", "かたばん"},
					{"材質", "ざいしつ"},
					((thiz->m_is_insert_operation)
						? (UiText{"ケース内\n　個数", "ケースない\n　こすう"})
						: (UiText{"再研磨\n  回数", "さいけんま\n  かいすう"})
					)
				}, .is_sensitive = true
			});
		},
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_ready_for_reading(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_store_ready_for_reading(); }
	});

	// --- 状態：補充 - 実際に補充を行う - 工具読み取り後 ---
	g_app_state_configs.try_emplace(AppState::STORE_READ_COMPLETED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"補充が終わったらコンテナを戻して\n扉を閉めてください", "ほじゅうが おわったら コンテナを もどして\nとびらを しめてください"}} },
			{ {&paginationBox}, {.is_visible = true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName	= "table2column",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_read_completed(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_store_read_completed(); },
		.on_falseButton_clicked		= [](MainWindow* thiz){ thiz->on_falseButton_store_read_completed(); },
		.on_trueButton_clicked		= [](MainWindow* thiz){ thiz->on_trueButton_store_read_completed(); }
		// TODO: 扉の開閉に関するPLC連携処理を実装 (扉が閉められている時のみtrue/falseボタンを操作できるようにする)
	});

	// --- 状態：棚卸し - 移行 ---
	g_app_state_configs.try_emplace(AppState::RECONCILE, WidgetStateConfig{
		.on_enter_state	= [](MainWindow* thiz){ thiz->set_app_state(AppState::RECONCILE_WAITING_FOR_CONTAINER_SELECTION); }
	});

	// --- 状態：棚卸し - コンテナ選択待ち ---
	g_app_state_configs.try_emplace(AppState::RECONCILE_WAITING_FOR_CONTAINER_SELECTION, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"棚卸しを行うコンテナを選択してください", "たなおろしを おこなう コンテナを\nせんたく してください"}} },
			{ {&table2column1stBox, &table2column2ndBox}, {.is_visible = true} },
			{ {&falseButton}, {UiText{"", ""}, false, true} },
			{ {&trueButton}, {UiText{"搬出", "はんしゅつ"}, false, true} }
		},
		.table_states = {
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
		.operationStackPageName	= "reconcileOperationPage",
		.tableStackPageName		= "table2column",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state					= [](MainWindow* thiz){ thiz->on_enter_reconcile_waiting_for_container_selection(); },
		.on_mainTableSelection_changed	= [](MainWindow* thiz){ thiz->on_mainTable_reconcile_waiting_for_container_selection(); },
		.on_trueButton_clicked			= [](MainWindow* thiz){ thiz->set_app_state(AppState::RECONCILE_CONTAINER_RETRIEVING); }
	});

	// --- 状態：棚卸し - コンテナ搬出待ち ---
	g_app_state_configs.try_emplace(AppState::RECONCILE_CONTAINER_RETRIEVING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&retrieveToggleButton, &storeToggleButton, &logoutButton}, {.is_sensitive = false} },
			{ {&messageLabel}, {.text = UiText{"コンテナを搬出しています\nしばらくお待ち下さい", "コンテナを はんしゅつ しています\nしばらく おまちください"}} },
			{ {&falseButton, &trueButton}, {UiText{"", ""}, false, true} }
		},
		.table_states = {
			{ {&table2column1st, &table2column2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.operationStackPageName	= "pageNone",
		.tableStackPageName		= "table2column",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state		= [](MainWindow* thiz){ thiz->on_enter_reconcile_container_retrieving(); },
		.on_plc_notified	= [](MainWindow* thiz, plc::Notification notification){ thiz->on_plc_reconcile_container_retrieving(notification); }
	});

	// --- 状態：棚卸し - コンテナ搬出完了 ---
	g_app_state_configs.try_emplace(AppState::RECONCILE_CONTAINER_RETRIEVED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"コンテナが届きました\n扉を開けてコンテナを取り出してください", "コンテナが とどきました\nとびらを あけて コンテナを とりだしてください"}} }
		},
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state		= [](MainWindow* thiz){ thiz->on_enter_reconcile_container_retrieving(); },
		.on_plc_notified	= [](MainWindow* thiz, plc::Notification notification){ thiz->on_plc_reconcile_container_retrieving(notification); }
	});

	// --- 状態：棚卸し - スキャン待ち ---
	g_app_state_configs.try_emplace(AppState::RECONCILE_READY_FOR_READING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"コンテナ内の工具をすべてQRコードリーダーで読み取ってください", "コンテナないの こうぐを すべて QRコードリーダーで よみとってください"}} },
			{ {&qrCodeEntryBox}, {.is_visible = true} },
			{ {&falseButton}, {UiText{"終了", "しゅうりょう"}, false, true} },
			{ {&trueButton}, {UiText{"続ける", "つづける"}, false, true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.operationStackPageName	= "reconcileInfoPage",
		.tableStackPageName		= "table2column",
		// --- 状態の「振る舞い」を定義 ---
		.on_prepare_state	= [](MainWindow* thiz){
			thiz->update_table_state({&(thiz->table2column1st), &(thiz->table2column2nd)}, {
				.column_headers = std::vector<UiText>{
					{"ID", "ID"},
					{"分類", "ぶんるい"},
					{"メーカー", "メーカー"},
					{"型番", "かたばん"},
					{"材質", "ざいしつ"},
					((thiz->m_is_insert_operation)
						? (UiText{"ケース内\n　個数", "ケースない\n　こすう"})
						: (UiText{"再研磨\n  回数", "さいけんま\n  かいすう"})
					)
				}, .is_sensitive = true
			});
		},
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_reconcile_ready_for_reading(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_reconcile_ready_for_reading(); }
	});

	// --- 状態：棚卸し - 工具読み取り後 ---
	g_app_state_configs.try_emplace(AppState::RECONCILE_READ_COMPLETED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"工具を全てスキャンできたら\n工具とコンテナを戻して扉を閉めてください", "こうぐを すべてスキャンできたら\nこうぐと コンテナを もどして とびらを しめてください"}} },
			{ {&table2column1stBox, &table2column2ndBox}, {.is_visible = true} },
			{ {&paginationBox}, {.is_visible = true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName	= "table2column",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_reconcile_read_completed(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_reconcile_read_completed(); },
		.on_falseButton_clicked		= [](MainWindow* thiz){ thiz->on_falseButton_reconcile_read_completed(); },
		.on_trueButton_clicked		= [](MainWindow* thiz){ thiz->on_trueButton_reconcile_read_completed(); }
		// TODO: 扉の開閉に関するPLC連携処理を実装 (扉が閉められている時のみtrue/falseボタンを操作できるようにする)
	});

	// TODO: 他のすべての状態(RETRIEVE_WAITING_FOR_CONTAINERなど)もここに追加していく
}

//=============================================================================
// 公開メンバ関数実装
//=============================================================================

gboolean MainWindow::dispatch_to_gui_thread(gpointer user_data) {
	// 渡されたポインタを元の std::function オブジェクトにキャスト
	auto* func = static_cast<std::function<void()>*>(user_data);
	// 関数を実行
	(*func)();
	// オブジェクトを解放
	delete func;
	// このコールバックは一度きりなので解除
	return G_SOURCE_REMOVE;
}

void MainWindow::on_plc_notified(plc::Notification notification) {
	DEV_PRINT("GUIスレッド: PLCから通知を受け取りました。通知タイプ: {}\n", (int)notification.type);
	switch (notification.type) {
	case plc::NotificationType::CONNECTION_ESTABLISHED :
		// TODO: CONNECTION_ESTABLISHED 通知を受け取ったときの処理
		break;
	case plc::NotificationType::CONNECTION_LOST :
		// TODO: CONNECTION_LOST 通知を受け取ったときの処理
		break;
	case plc::NotificationType::MONITOR :
		if (notification.monitor_result.has_value()) {
			switch (notification.monitor_result.value().addr) {
			case plc::M_DEVICE_MAINTENANCE_MODE :
				// TODO: メンテナンスモードに関する処理を追加
				if (notification.monitor_result.value().status) {
					update_widget_state(&warningLabel, WidgetState{.text = UiText{
						"ステータスモニタでの操作により、画面がロックされました。\n詳細はステータスモニタをご覧ください。",
						"ステータスモニタでの そうさにより、がめんが ロックされました。\nしょうさいは ステータスモニタを ごらんください。"
					}});
					refresh_ui(false);
					warningDialog.show_all();
				} else {
					warningDialog.destroy();
				}
				break;

			// TODO: 他にも全画面共通の処理があれば追加

			default :	// 全画面共通の処理を行わないモニター通知は、状態毎の処理に投げる
				try {
					auto state_config = g_app_state_configs.at(m_app_state);
					if (state_config.on_plc_notified) {
						state_config.on_plc_notified(this, notification);
					}
				} catch (const std::out_of_range& e) {
					DEV_PRINT(stderr, "Error: 未定義の状態でPLC通知を受け取りました (AppState={})\n", static_cast<int>(m_app_state));
					// 例外を無視して処理を続行
				}
				break;
			}
		}
		break;
	default :
		DEV_PRINT(stderr, "Error: 未知のPLC通知を受け取りました\n");
	}
}

//=============================================================================
// プライベートメンバ関数 (ヘルパー関数) 実装
//=============================================================================

void MainWindow::reset_variables() {
	m_is_insert_operation = false;
}

void MainWindow::reset_header_buttons() {
	// 権限に応じてトグルボタンの操作可否を設定
	MstUserInfo logged_in_user_info = g_logged_in_user_info.value_or(MstUserInfo{0, "", false, false, false});
	// 取り出しトグルボタン
	update_widget_state(&retrieveToggleButton, WidgetState{
		.is_sensitive = logged_in_user_info.per_out,
	});
	// 補充トグルボタン
	update_widget_state(&storeToggleButton, WidgetState{
		.is_sensitive = logged_in_user_info.per_add,
	});
	// 棚卸しトグルボタン
	update_widget_state(&reconcileToggleButton, WidgetState{
		.is_sensitive = logged_in_user_info.per_inventory,
	});
	// ログアウトボタンを操作可能に
	update_widget_state(&logoutButton, WidgetState{
		.is_sensitive = true
	});
}

void MainWindow::update_widget_state(sg::Widget* widget, WidgetState state) {
	// m_widget_statesから現在のWidgetStateUpdaterのコピーを取得
	auto updater = m_widget_states.at(widget);

	// stateメンバーだけを新しい状態に更新
	if (updater.state.merge(state)) {
		// 変更があった場合、更新したWidgetStateUpdaterをマップに書き戻す
		m_widget_states.insert_or_assign(widget, updater);
	}
}

void MainWindow::update_widget_state(std::vector<sg::Widget*> widgets, WidgetState state) {
	for (auto* widget : widgets) {
		update_widget_state(widget, state);
	}
}

void MainWindow::update_table_state(sgex::TreeView* table, TableState state) {
	// m_table_statesから現在のTableStateのコピーを取得
	auto current_state = m_table_states.at(table);

	// stateメンバーだけを新しい状態に更新
	if (current_state.merge(state)) {
		// 変更があった場合、更新したTableStateをマップに書き戻す
		m_table_states.insert_or_assign(table, current_state);
	}
}

void MainWindow::update_table_state(std::vector<sgex::TreeView*> tables, TableState state) {
	for (auto* table : tables) {
		update_table_state(table, state);
	}
}

void MainWindow::refresh_ui(bool force_redraw_all) {
	// 1. 更新対象となるウィジェットのリストを取得
	auto widgets_to_update = ((force_redraw_all)
		? (m_widget_states.get_all(true))	// 全ウィジェットを取得
		: (m_widget_states.get_updated_elements(true))); // 更新されたウィジェットのみ取得し、取得後に更新リストをクリア

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
		? (m_table_states.get_all(true))	// 全テーブルを取得
		: (m_table_states.get_updated_elements(true)); // 更新されたテーブルのみ取得し、取得後に更新リストをクリア

	// 4. 対象テーブルのカラムヘッダー表示を更新
	for (const auto& [table, headers] : tables_to_update) {
		for (guint i = 0; i < headers.column_headers.value().size(); ++i) {
			sgex::TreeViewColumn column(table->get_column(i));
			if ((GtkTreeViewColumn*)column) {
				const auto& header = headers.column_headers.value()[i];
				const std::string& title = (m_text_mode == TextMode::KANA) ? header.kana : header.kanji;
				column.set_title(title.c_str());
			}
		}
		table->set_sensitive(headers.is_sensitive.value());
		table->set_visible(headers.is_visible.value());
	}
}

void MainWindow::set_app_state(AppState state) {
	// メンバ変数の状態を更新
	m_app_state = state;

	// m_widget_statesを更新する
	const auto& state_config = g_app_state_configs.at(state);
	for (const auto& widget_state : state_config.widget_states) {
		for (const auto& widget : widget_state.widgets) {
			update_widget_state(widget, widget_state.state);
		}
	}

	// m_table_statesを更新する
	for (const auto& table_state : state_config.table_states) {
		for (const auto& table : table_state.tables) {
			update_table_state(table, table_state.state);
		}
	}

	// 各スタックの表示ページを更新
	if (!state_config.operationStackPageName.empty() && operationStack.get_visible_child_name() != state_config.operationStackPageName) {
		operationStack.set_visible_child_name(state_config.operationStackPageName.c_str());
	}
	if (!state_config.tableStackPageName.empty() && table_stack.get_visible_child_name() != state_config.tableStackPageName) {
		table_stack.set_visible_child_name(state_config.tableStackPageName.c_str());
		update_and_handle_pagination(PaginationAction::UPDATE_UI_ONLY);
	}

	// UI更新前のカスタム処理を実行
	if (state_config.on_prepare_state) {
		state_config.on_prepare_state(this);
	}

	// UIを再描画
	refresh_ui(false);

	// 状態遷移直後に実行するアクションがあれば実行
	if (state_config.on_enter_state) {
		state_config.on_enter_state(this);
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

	// 1. トグルボタンの状態を排他的に更新
	m_isProgrammaticallyChangingToggle = true;
	retrieveToggleButton.set_active(mode == GuiMode::RETRIEVE);
	storeToggleButton.set_active(mode == GuiMode::STORE);
	reconcileToggleButton.set_active(mode == GuiMode::RECONCILE);
	m_isProgrammaticallyChangingToggle = false;

	// 2. モードが変化する場合、モードに応じて現在の状態を切り替え
	if (m_gui_mode != mode) {
		reset_variables();
		reset_header_buttons();

		switch (mode) {
		case GuiMode::NONE:
			DEV_PRINT("ログイン待ちモード\n");
			set_app_state(AppState::NONE);
			break;
		case GuiMode::RETRIEVE:
			DEV_PRINT("取り出しモード\n");
			set_app_state(AppState::RETRIEVE);
			break;
		case GuiMode::STORE:
			DEV_PRINT("補充モード\n");
			set_app_state(AppState::STORE);
			break;
		case GuiMode::RECONCILE:
			DEV_PRINT("棚卸しモード\n");
			set_app_state(AppState::RECONCILE);
			break;
		}
	}

	// 3. メンバ変数を更新
	m_gui_mode = mode;
}

void MainWindow::main_ListStore_setter(sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) {
	GdkRGBA bgcolor_0to4;
	GdkRGBA bgcolor_5;
	try {
		bgcolor_0to4 = MainWindow::g_table_row_colors.at(row.cell_state_0to4);
		bgcolor_5 = ((row.cell_state_5 == TableCellState::NONE)
			? (bgcolor_0to4)
			: (MainWindow::g_table_row_colors.at(row.cell_state_5))
		);
	} catch (const std::out_of_range& e) {
		DEV_PRINT(stderr, "Error: TableCellState enum value out of range: {}\n", static_cast<int>(row.cell_state_0to4));
		throw; // 再スローして呼び出し元で処理させる
	}
	store->set(iter,
		0, row.data_columns[0].c_str(),
		1, row.data_columns[1].c_str(),
		2, row.data_columns[2].c_str(),
		3, row.data_columns[3].c_str(),
		4, row.data_columns[4].c_str(),
		5, row.data_columns[5].c_str(),
		6, &bgcolor_0to4,
		7, &bgcolor_5
	);
}

void MainWindow::update_and_handle_pagination(PaginationAction action) {
	// 1. 現在の状態設定から、アクティブなテーブルページ名を取得
	const auto& state_config = g_app_state_configs.at(m_app_state);
	const auto& page_name = state_config.tableStackPageName;

	// 2. ページ名に応じて、操作対象となるページネーションモデルを選択
	sgex::PaginatedListStoreModel<MainTableData>* active_paginator = nullptr;
	if (page_name == "table3column") {
		active_paginator = &liststore3columnAll;
	} else if (page_name == "table2column") {
		active_paginator = &liststore2columnAll;
	}

	// 3. 対象のページネーションモデルが存在しない場合、UIを非表示にして終了
	if (!active_paginator) {
		update_widget_state(&paginationBox, WidgetState{ .is_visible = false });
		refresh_ui(false);
		return;
	}
	update_widget_state(&paginationBox, WidgetState{ .is_visible = true });
	refresh_ui(false);

	// 4. 引数に応じてページ送り操作を実行
	if (action == PaginationAction::GO_TO_PREV) {
		active_paginator->prev_page();
	} else if (action == PaginationAction::GO_TO_NEXT) {
		active_paginator->next_page();
	}
	// actionがUPDATE_UI_ONLYの場合は、ページ送りは行わない

	// 5. モデルの現在の状態に基づいて、UIを更新
	pageNumberLabel.set_label(fmt::format("ページ {:2d} / {:2d}",
		active_paginator->get_current_page(),
		active_paginator->get_total_pages()
	).c_str());
	prevButton.set_sensitive(active_paginator->has_prev_page());
	nextButton.set_sensitive(active_paginator->has_next_page());
}

bool MainWindow::authenticate_with_password() {
	// ダイアログ表示前にパスワード欄をクリア
	passwordEntry.set_text("");

	GtkResponseType response;

	// 既に認証済みならば、再度認証を求めない
	if (!m_password_authenticated) {
		// パスワード認証が成功するか、キャンセルされるまで繰り返す
		update_widget_state(&passwordEntryLabel, WidgetState{
			.text = UiText{"パスワードを\n入力してください", "パスワードを\nにゅうりょく してください"}
		});
		refresh_ui(false); // ラベルのテキストを更新
		do {
			// run()でダイアログをモーダル表示し、ユーザーの応答を待つ
			response = passwordEntryDialog.run(this);
			if (response == GTK_RESPONSE_OK) {
				// TODO: ここで実際のパスワード検証ロジックを実装します
				// 今回はダミーとして "1234" を正しいパスワードとします
				DEV_PRINT("認証開始\n");
				if (std::string(passwordEntry.get_text()) == "1234") {
					DEV_PRINT("認証成功\n");
					m_password_authenticated = true;
				} else {
					DEV_PRINT("認証失敗：パスワードが違います\n");
					update_widget_state(&passwordEntryLabel, WidgetState{
						.text = UiText{"パスワードが違います\nもう一度入力してください", "パスワードが ちがいます\nもういちど にゅうりょく してください"}
					});
					refresh_ui(false); // ラベルのテキストを更新
				}
			} else if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT) {
				// キャンセルボタンが押されたか、ダイアログが閉じられた場合
				m_password_authenticated = false;
				DEV_PRINT("認証キャンセル\n");
			}
		} while (response == GTK_RESPONSE_OK && !m_password_authenticated);

		// ダイアログを非表示にする
		passwordEntryDialog.hide();
	}
	return m_password_authenticated;
}

bool MainWindow::confirm_if_reground() {
	// 1. ダイアログの初期状態を設定（デフォルトは「いいえ」を選択）
	m_isProgrammaticallyChangingToggle = true;
	notRegroundButton.set_active(false);
	regroundButton.set_active(true);
	m_isProgrammaticallyChangingToggle = false;

	// 2. ダイアログをモーダル表示し、ユーザーの応答を待つ
	confirmRegroundDialog.run(this);
	confirmRegroundDialog.hide();

	// 3. OKボタンが押されたら、トグルボタンの状態を確認して返す
	return regroundButton.get_active();
}

void MainWindow::save_store_history() {
	db::save_history(RecOperationTable{});	// TODO: 後で詳細を書く
}

//=============================================================================
// プライベートメンバ関数 (イベントハンドラ) 実装
//=============================================================================

void MainWindow::on_window_show() {
	if (m_is_first_show) {
		// 初回表示の時だけ、UI全体を初期状態に合わせて完全に再描画し、初期設定を行う
		storingToolTable.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH); // テーブルのグリッド線を表示
		table2column1st.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH); // テーブルのグリッド線を表示
		table2column2nd.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH); // テーブルのグリッド線を表示
		table3column1st.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH); // テーブルのグリッド線を表示
		table3column2nd.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH); // テーブルのグリッド線を表示
		table3column3rd.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH); // テーブルのグリッド線を表示

		set_mode(GuiMode::NONE);	// 最初は取り出しモード
		set_app_state(AppState::WAITING_FOR_USER_LOGIN);	// 最初の状態を設定
		refresh_ui(true);
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
	DEV_PRINT("[取り出し]\n");
	if (m_isProgrammaticallyChangingToggle) {
		DEV_PRINT("無視\n");
		return;	// プログラムによる変更は無視
	}

	// このボタンに対する操作は、常に「取り出し」モードへの設定を試みる
	// これにより、ユーザーがアクティブなこのボタンを押してオフにしようとしても、
	// set_modeが再度このボタンをオンにし、状態が維持される
	set_mode(GuiMode::RETRIEVE);
}

void MainWindow::storeToggleButton_on_toggled() {
	DEV_PRINT("[補充]\n");
	if (m_isProgrammaticallyChangingToggle) {
		DEV_PRINT("無視\n");
		return;	// プログラムによる変更は無視
	}

	// このボタンに対する操作は、常に「補充」モードへの設定を試みる
	// これにより、ユーザーがアクティブなこのボタンを押してオフにしようとしても、
	// set_modeが再度このボタンをオンにし、状態が維持される
	set_mode(GuiMode::STORE);
}

void MainWindow::reconcileToggleButton_on_toggled() {
	DEV_PRINT("[棚卸し]\n");
	if (m_isProgrammaticallyChangingToggle) {
		DEV_PRINT("無視\n");
		return;	// プログラムによる変更は無視
	}

	// このボタンに対する操作は、常に「棚卸し」モードへの設定を試みる
	// これにより、ユーザーがアクティブなこのボタンを押してオフにしようとしても、
	// set_modeが再度このボタンをオンにし、状態が維持される
	set_mode(GuiMode::RECONCILE);
}

void MainWindow::kanaKanjiButton_on_clicked() {
	m_text_mode = ((m_text_mode == TextMode::KANJI) ? (TextMode::KANA) : (TextMode::KANJI));
	refresh_ui(true); // 現在の状態のままUIテキストを更新
}

void MainWindow::logoutButton_on_clicked() {
	DEV_PRINT("ログアウトボタンがクリックされました\n");
	if (logoutDialog.run(this) == GTK_RESPONSE_YES) {
		DEV_PRINT("ログアウトします\n");
		m_password_authenticated = false;
		g_logged_in_user_info = nullopt;
		set_mode(GuiMode::NONE);
	} else {
		DEV_PRINT("ログアウトはキャンセルされました\n");
	}
}

void MainWindow::userIdEntry_on_activate() {
	DEV_PRINT("ユーザーIDエントリでEnterキーが押されました\n");

	if (db::user_authenticate(userIdEntry.get_text())) {
		DEV_PRINT("ユーザー認証成功\n");

		// ユーザーIDエントリにログインユーザー名を表示して固定
		update_widget_state(&userIdEntry, WidgetState{.is_sensitive = false});

		reset_header_buttons();

		refresh_ui(false); // UIを更新
		userIdEntry.set_text(g_logged_in_user_info.value().name.c_str());

		if (g_logged_in_user_info.value().per_out) {
			// 取り出し権限があれば取り出しモードへ
			set_mode(GuiMode::RETRIEVE);
		} else if (g_logged_in_user_info.value().per_add) {
			// 補充権限があれば補充モードへ
			set_mode(GuiMode::STORE);
		} else if (g_logged_in_user_info.value().per_inventory) {
			// 棚卸し権限があれば棚卸しモードへ
			set_mode(GuiMode::RECONCILE);
		} else {
			// どの権限もなければ認証画面に戻る
			set_mode(GuiMode::NONE);
		}
	} else {
		DEV_PRINT("ユーザー認証失敗\n");
		update_widget_state(&messageLabel, WidgetState{
			.text = UiText{"該当するユーザーが見つかりません\nもう一度QRコードを読み取ってください", "がいとうする ユーザーが みつかりません\nもういちど QRコードを よみとってください"}
		});
		refresh_ui(false); // ラベルのテキストを更新
	}
}

void MainWindow::mainTableSelection_on_changed(){
	try {
		auto state_config = g_app_state_configs.at(m_app_state);
		if (state_config.on_mainTableSelection_changed) {
			state_config.on_mainTableSelection_changed(this);
		}
	} catch (const std::out_of_range& e) {
		DEV_PRINT(stderr, "Error: 未定義の状態でmainTableの選択が変更されました (AppState={})\n", static_cast<int>(m_app_state));
		// 例外を無視して処理を続行
	}
}

void MainWindow::prevButton_on_clicked() {
	DEV_PRINT("前のページへ\n");
	update_and_handle_pagination(PaginationAction::GO_TO_PREV);
}

void MainWindow::nextButton_on_clicked() {
	DEV_PRINT("次のページへ\n");
	update_and_handle_pagination(PaginationAction::GO_TO_NEXT);
}

void MainWindow::qrCodeEntry_on_activate() {
	try {
		auto state_config = g_app_state_configs.at(m_app_state);
		if (state_config.on_qrCodeEntry_activate) {
			state_config.on_qrCodeEntry_activate(this);
		}
	} catch (const std::out_of_range& e) {
		DEV_PRINT(stderr, "Error: 未定義の状態でqrCodeEntryでEnterが押されました (AppState={})\n", static_cast<int>(m_app_state));
		// 例外を無視して処理を続行
	}
	qrCodeEntry.set_text("");
}

void MainWindow::falseButton_on_clicked() {
	try {
		auto state_config = g_app_state_configs.at(m_app_state);
		if (state_config.on_falseButton_clicked) {
			state_config.on_falseButton_clicked(this);
		}
	} catch (const std::out_of_range& e) {
		DEV_PRINT(stderr, "Error: 未定義の状態でFalseボタンがクリックされました (AppState={})\n", static_cast<int>(m_app_state));
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
		DEV_PRINT(stderr, "Error: 未定義の状態でTrueボタンがクリックされました (AppState={})\n", static_cast<int>(m_app_state));
		// 例外を無視して処理を続行
	}
}

void MainWindow::passwordEntryDialog_on_show() {
	// Gtkウィジェットにキーボードフォーカスを強制的に設定する
	passwordEntry.grab_focus();
}

void MainWindow::regroundButton_on_toggled() {
	if (m_isProgrammaticallyChangingToggle) return;	// プログラムによる変更は無視

	if (regroundButton.get_active()) {
		m_isProgrammaticallyChangingToggle = true;
		notRegroundButton.set_active(false);	// 「いいえ」をオフにする
		m_isProgrammaticallyChangingToggle = false;
	}
}

void MainWindow::notRegroundButton_on_toggled() {
	if (m_isProgrammaticallyChangingToggle) return;	// プログラムによる変更は無視

	if (notRegroundButton.get_active()) {
		m_isProgrammaticallyChangingToggle = true;
		regroundButton.set_active(false); // 「はい」をオフにする
		m_isProgrammaticallyChangingToggle = false;
	}
}

//=========================================================================
// プライベートメンバ関数 (状態毎の関数) 実装
//=========================================================================

void MainWindow::on_enter_waiting_for_user_login() {
	userIdEntry.set_text("");
	userIdEntry.grab_focus();
}

//取り出し

void MainWindow::on_mainTableSelection_retrieve_selecting_tools(){

	/*----------------------------------------------------------------------------------------*/
						//QRコードで読み取ったプログラム割当一覧の受け取り(始め)
	/*----------------------------------------------------------------------------------------*/
	
	auto tooling_list_info_opt = db::get_ToolingListInfo(qrCodeEntry.get_text());

	if (tooling_list_info_opt.has_value()){
		ToolingListInfo tooling_list = tooling_list_info_opt.value();

		// 2. ToolAssignmentInfo のリストを MainTableData のリストに変換
		std::vector<gui::MainWindow::MainTableData> table_data_list;
		table_data_list.reserve(tooling_list.tool_assignments.size()); // 事前にメモリ確保

		for (const auto& tool_assignment : tooling_list.tool_assignments) {
			gui::MainWindow::MainTableData row; // 1行分のデータ構造

			// ToolAssignmentInfo と MstToolInfo から MainTableData に値をコピー
			row.id = tool_assignment.id; // 行の識別に使う (今回は tool_assignment_id を流用)
			row.data_columns[0] = tool_assignment.tool_num;              // ツール番号
			row.data_columns[1] = tool_assignment.mst_tool_info.category; // 分類
			row.data_columns[2] = tool_assignment.mst_tool_info.maker;    // メーカー
			row.data_columns[3] = tool_assignment.mst_tool_info.name;     // 型番
			row.data_columns[4] = tool_assignment.mst_tool_info.material; // 材質
			row.data_columns[5] = std::to_string(tool_assignment.mst_tool_info.stc); // 在庫数 (文字列に変換)
			row.cell_state_0to4 = gui::MainWindow::TableCellState::NORMAL; // 初期状態は通常色
			// row.cell_state_5 はデフォルト(NONE)のまま

			table_data_list.push_back(row); // リストに追加
		}

		// 3. 変換したデータをページネーションモデルにセット (これで表示が更新される)
		liststore2columnAll.set_master_data(table_data_list); //

		// 4. 次の状態へ遷移
		set_app_state(AppState::RETRIEVE_SELECTING_TOOLS); //

	} else {
		// 取得失敗時の処理 (エラーメッセージ表示など)
		DEV_PRINT("ツーリングリストの取得に失敗しました。\n");
		update_widget_state(&messageLabel, WidgetState{
			.text = UiText{"有効なプログラム割当一覧表QRコードではありません。", "ゆうこうな プログラムわりあていちらんひょう QRコードではありません。"}
		});
		refresh_ui(false); //
	}

	/*----------------------------------------------------------------------------------------*/
						//QRコードで読み取ったプログラム割当一覧の受け取り(終わり)
	/*----------------------------------------------------------------------------------------*/	

	
	/*----------------------------------------------------------------------------------------*/
						////テーブル選択時の背景色変更(始め)
	/*----------------------------------------------------------------------------------------*/	
	// 現在のアプリケーション状態を取得
    const auto& state_config = g_app_state_configs.at(m_app_state);

    // 現在の状態に対応するコールバック関数が登録されていれば実行
    if (state_config.on_mainTableSelection_changed) {
        state_config.on_mainTableSelection_changed(this);
        return; // 状態固有の処理があればそれを優先
    }

    // --- 以下は状態固有の処理がない場合のデフォルト動作 ---

    // 現在アクティブなページネーションモデルを取得
    sgex::PaginatedListStoreModel<MainTableData>* active_paginator = nullptr;
    const auto& page_name = state_config.tableStackPageName;
    if (page_name == "table3column") {
        active_paginator = &liststore3columnAll;
    } else if (page_name == "table2column") {
        active_paginator = &liststore2columnAll;
    }

    if (!active_paginator) {
        return; // 対象モデルがなければ何もしない
    }

    // 選択された行のグローバルインデックスを取得
    std::optional<size_type> selected_global_index_opt = active_paginator->get_selected_index(); //

    if (selected_global_index_opt.has_value()) {
        size_type selected_global_index = selected_global_index_opt.value();

        // 選択された行の現在のデータを取得
        MainTableData current_row_data = active_paginator->get_row(selected_global_index); //

        // 選択状態をトグルする
        if (m_selected_tool_indices.count(selected_global_index)) {
            // --- すでに選択されていた場合：選択解除 ---
            m_selected_tool_indices.erase(selected_global_index); // セットから削除
            // 背景色を通常に戻す
            current_row_data.cell_state_0to4 = TableCellState::NORMAL; //
            DEV_PRINT("行 {} を選択解除しました。\n", selected_global_index);

        } else {
            // --- まだ選択されていなかった場合：選択 ---
            m_selected_tool_indices.insert(selected_global_index); // セットに追加
            // 背景色を選択状態にする
            current_row_data.cell_state_0to4 = TableCellState::SELECTED; //
            DEV_PRINT("行 {} を選択しました。\n", selected_global_index);
        }

        // 変更をモデルに反映させて表示を更新
        active_paginator->update(selected_global_index, current_row_data); //

        // (オプション) trueButton (確定ボタンなど) の有効/無効状態を更新
        // 例: 選択されている行が1つ以上あれば有効にする
        update_widget_state(&trueButton, WidgetState{ .is_sensitive = !m_selected_tool_indices.empty() }); //
        refresh_ui(false); //

    } else {
        // 何も選択されていない状態になった場合 (通常はあまり発生しない)
        DEV_PRINT("選択が解除されました (選択なし)。\n");
        // (オプション) trueButton を無効化するなど
        update_widget_state(&trueButton, WidgetState{ .is_sensitive = false }); //
        refresh_ui(false); //
    }

	/*----------------------------------------------------------------------------------------*/
						////テーブル選択時の背景色変更(終わり)
	/*----------------------------------------------------------------------------------------*/
	/*----------------------------------------------------------------------------------------*/
						////確定ボタンの処理(始め)
	/*----------------------------------------------------------------------------------------*/

	// tool_io_gui.hpp の MainWindow クラス内に以下を追加
	// private:
	//     // ...
	//     /// @brief 確認画面用に選択された工具データを保持するリスト
	//     std::vector<MainTableData> m_confirmed_selection;
	// tool_io_gui.cpp 内の g_app_state_configs の RETRIEVE_SELECTING_TOOLS の on_trueButton_clicked ラムダ内
	.on_trueButton_clicked = [](MainWindow* thiz){
		// 1. アクティブなページネーションモデルを取得 (エラーチェックは省略)
		sgex::PaginatedListStoreModel<MainTableData>* active_paginator = nullptr;
		if (thiz->table_stack.get_visible_child_name() == "table2column") { //
			active_paginator = &(thiz->liststore2columnAll); //
		} else if (thiz->table_stack.get_visible_child_name() == "table3column") {
			active_paginator = &(thiz->liststore3columnAll); //
		} else {
			return; // 想定外
		}

		// 2. 選択されたインデックスに基づいてマスターデータからデータを抽出
		thiz->m_confirmed_selection.clear(); // 前回のデータをクリア
		thiz->m_confirmed_selection.reserve(thiz->m_selected_tool_indices.size()); // メモリ確保

		for (const auto& index : thiz->m_selected_tool_indices) {
			if (index < active_paginator->size()) { // インデックスが有効か確認
				// get_row でマスターデータからコピーを取得
				thiz->m_confirmed_selection.push_back(active_paginator->get_row(index));
			}
		}

		// 3. 抽出したデータが空でないか確認 (選択なしで確定は押せないはずだが念のため)
		if (thiz->m_confirmed_selection.empty()) {
			DEV_PRINT("エラー: 選択された工具がありません。\n");
			// 必要ならメッセージ表示
			return;
		}

		DEV_PRINT("{} 件の工具データを選択しました。\n", thiz->m_confirmed_selection.size());

		// 4. 次の状態 (選択確認画面) へ遷移
		thiz->set_app_state(AppState::RETRIEVE_CONFIRMING_SELECTION); //
	},

	/*----------------------------------------------------------------------------------------*/
						////確定ボタンの処理(終わり)
	/*----------------------------------------------------------------------------------------*/
	/*----------------------------------------------------------------------------------------*/
						////全選択解除ボタンの処理(始め)
	/*----------------------------------------------------------------------------------------*/
	
		// tool_io_gui.cpp 内の g_app_state_configs の RETRIEVE_SELECTING_TOOLS の on_falseButton_clicked ラムダ内
		.on_falseButton_clicked = [](MainWindow* thiz){
			DEV_PRINT("全選択解除ボタンがクリックされました。\n");

			// 1. アクティブなページネーションモデルを取得 (エラーチェックは省略)
			sgex::PaginatedListStoreModel<MainTableData>* active_paginator = nullptr;
			if (thiz->table_stack.get_visible_child_name() == "table2column") { //
				active_paginator = &(thiz->liststore2columnAll); //
			} else if (thiz->table_stack.get_visible_child_name() == "table3column") {
				active_paginator = &(thiz->liststore3columnAll); //
			} else {
				return; // 想定外
			}

			// 2. 以前に選択されていた行の背景色を通常に戻す
			for (const auto& index : thiz->m_selected_tool_indices) {
				if (index < active_paginator->size()) { // インデックスが有効か確認
					MainTableData row_data = active_paginator->get_row(index); // データ取得
					if (row_data.cell_state_0to4 == TableCellState::SELECTED) { // 選択状態なら
						row_data.cell_state_0to4 = TableCellState::NORMAL; // 通常状態に戻す
						active_paginator->update(index, row_data); // モデルと表示を更新
					}
				}
			}

			// 3. 選択インデックスのセットをクリア
			thiz->m_selected_tool_indices.clear();
			DEV_PRINT("選択をクリアしました。\n");

			// 4. 確定ボタンを無効化
			thiz->update_widget_state(&(thiz->trueButton), WidgetState{ .is_sensitive = false }); //
			thiz->refresh_ui(false); //
		},
	/*----------------------------------------------------------------------------------------*/
						////全選択解除ボタンの処理(終わり)
	/*----------------------------------------------------------------------------------------*/
}


void MainWindow::on_enter_store_waiting_for_tool_qr() {
	storingToolListstore.clear();
	storingToolListstore.append(MstToolInfo{});
	qrCodeEntry.set_text("");
	qrCodeEntry.grab_focus();
}

void MainWindow::on_qrCodeEntry_store_waiting_for_tool_qr() {
	MstToolInfo mst_tool;
	auto unique_info = db::get_UniqueToolInfo(qrCodeEntry.get_text());

	if (unique_info.has_value()) {
		mst_tool = unique_info.value().mst_tool_info;
		storingToolListstore.update(0, mst_tool);

		// 再研磨回数が設定されていなければインサート操作とみなす
		m_is_insert_operation = !unique_info.value().regrind_count.has_value();

		set_app_state(AppState::STORE_CONFIRMING_SELECTION);
	} else {
		// TODO: 該当データが無い場合の処理
		update_widget_state(&messageLabel, WidgetState{
			.text = UiText{"該当する工具データが見つかりません\nもう一度QRコードを読み取ってください", "がいとうする こうぐデータが みつかりません\nもういちど QRコードを よみとってください"}
		});
		refresh_ui(false); // ラベルのテキストを更新
	}
}

void MainWindow::on_enter_store_container_retrieving() {
	plc::retrieve_container(0);	// TODO: 搬出するコンテナIDを指定
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_RETRIEVE_COMPLETED,
		.flags = plc::WatchFlags{
			.enable_rise_notification = true,
			.rise_is_one_shot = true
		}
	});
}

void MainWindow::on_plc_store_container_retrieving(plc::Notification notification) {
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {
		auto result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_RETRIEVE_COMPLETED && result.status) {
			set_app_state(AppState::STORE_CONTAINER_RETRIEVED);
		} else {}	// TODO: エラー処理
	}
}

void MainWindow::on_enter_store_container_retrieved() {
	// TODO: 扉を開けるまで待機する処理を実装
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_DOOR_OPEND,
		.flags = plc::WatchFlags{
			.enable_rise_notification = true,
			.rise_is_one_shot = true
		}
	});
}

void MainWindow::on_plc_store_container_retrieved(plc::Notification notification) {
	// TODO: 扉を開けるまで待機する処理を実装
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {
		auto result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_DOOR_OPEND && result.status) {
			set_app_state(AppState::STORE_CONTAINER_RETRIEVED);
		} else {}	// TODO: エラー処理
	}
}

void MainWindow::on_enter_store_ready_for_reading() {
	liststore2column1st.clear();
	liststore2column2nd.clear();
	qrCodeEntry.set_text("");
	qrCodeEntry.grab_focus();
}

void MainWindow::on_qrCodeEntry_store_ready_for_reading() {
	if (on_qrCodeEntry_store_read_completed()) {
		set_app_state(AppState::STORE_READ_COMPLETED);
	}
}

void MainWindow::on_enter_store_read_completed() {
	// TODO: 扉を閉めるまで待機する処理を実装
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_DOOR_OPEND,
		.flags = plc::WatchFlags{
			.enable_fall_notification = true,
			.enable_rise_notification = true,
			.fall_is_one_shot = false,
			.rise_is_one_shot = false
		}
	});
}

bool MainWindow::on_qrCodeEntry_store_read_completed() {
	bool result = false;
	bool is_reground = false;
	MstToolInfo mst_tool;
	auto unique_info = db::get_UniqueToolInfo(qrCodeEntry.get_text());

	if (unique_info.has_value()) {
		if (!m_is_insert_operation && unique_info.value().storage_condition == StorageCondition::TAKEN_OUT) {
			if ((is_reground = confirm_if_reground())) {
				// 再研磨操作の場合、再研磨回数を1増やす
				unique_info.value().regrind_count = (unique_info.value().regrind_count.value_or(0) + 1);
			}
		}

		liststore2columnAll.append(MainTableData{
			.id = unique_info.value().id,
			.data_columns = {
				to_string(unique_info.value().id),
				unique_info.value().mst_tool_info.category,
				unique_info.value().mst_tool_info.maker,
				unique_info.value().mst_tool_info.name,
				unique_info.value().mst_tool_info.material,
				to_string(((m_is_insert_operation)
					? (unique_info.value().case_pack_num)
					: (unique_info.value().regrind_count)
				).value())
			}
		});
		update_and_handle_pagination(PaginationAction::UPDATE_UI_ONLY);
		result = true;
	} else {
		// TODO: 該当データが無い場合の処理
		update_widget_state(&messageLabel, WidgetState{
			.text = UiText{"該当する工具データが見つかりません\nもう一度QRコードを読み取ってください", "がいとうする こうぐデータが みつかりません\nもういちど QRコードを よみとってください"}
		});
		refresh_ui(false); // ラベルのテキストを更新
		result = false;
	}
	qrCodeEntry.set_text("");
	qrCodeEntry.grab_focus();
	return result;
}

void MainWindow::on_falseButton_store_read_completed() {
	save_store_history();
	set_mode(GuiMode::NONE);
}

void MainWindow::on_trueButton_store_read_completed() {
	save_store_history();
	set_mode(GuiMode::STORE);
}

void MainWindow::on_plc_store_read_completed(plc::Notification notification) {
	// TODO: 扉を閉めるまで待機する処理を実装
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {
		auto result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_DOOR_OPEND) {
			update_widget_state({&falseButton, &trueButton}, WidgetState{.is_sensitive = !result.status});
		} else {}	// TODO: エラー処理
	}
}

void MainWindow::on_enter_reconcile_waiting_for_container_selection() {

}

void MainWindow::on_mainTable_reconcile_waiting_for_container_selection() {

}

void MainWindow::on_enter_reconcile_container_retrieving() {

}

void MainWindow::on_plc_reconcile_container_retrieving(plc::Notification notification) {

}

void MainWindow::on_enter_reconcile_container_retrieved() {

}

void MainWindow::on_plc_reconcile_container_retrieved(plc::Notification notification) {

}

void MainWindow::on_enter_reconcile_ready_for_reading() {
	// TODO: DBからこのコンテナの在庫リストを取得し、テーブルに「未読」状態で表示


	qrCodeEntry.set_text("");
	qrCodeEntry.grab_focus();
}

void MainWindow::on_qrCodeEntry_reconcile_ready_for_reading() {
	// TODO: スキャンされたQRを処理するロジック
	// 1. QRから工具ID取得
	// 2. テーブル内の該当工具を検索
	// 3. 見つかれば「一致」状態に更新
	// 4. 見つからなければ「不一致」として新規行を追加
	// x. このとき、インサートの場合は個数確認ダイアログを表示
}

void MainWindow::on_enter_reconcile_read_completed() {

}

bool MainWindow::on_qrCodeEntry_reconcile_read_completed() {
	return true;
}

void MainWindow::on_falseButton_reconcile_read_completed() {
	// TODO: 棚卸し結果をDBに保存
	set_mode(GuiMode::NONE);
}

void MainWindow::on_trueButton_reconcile_read_completed() {
	// TODO: 棚卸し結果をDBに保存
	set_mode(GuiMode::RECONCILE);
}

} // namespace gui