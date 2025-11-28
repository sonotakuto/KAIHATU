/**
 * @file tool_io_gui.cpp
 * @brief tool_io_gui.hppで宣言されたクラスのメンバ関数の実装ファイル。
 */
#include "./SimpleGtk_extension.hpp"
#include "./includes.hpp"
#include "./db_client.hpp"
#include "./tracking_communicator.hpp"
#include "./utility.hpp"
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
	if (other.can_focus.has_value() && this->can_focus != other.can_focus) {
		this->can_focus = other.can_focus;
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
	if (other.is_editable.has_value() && this->is_editable != other.is_editable) {
		this->is_editable = other.is_editable;
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
	listChangeButton(glade, "listChangeButton"),
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
				4, &TABLE_BGCOLOR_NORMAL
			);
		}
	),
	filterButton(glade, "filterButton"),
	containerLabel(glade, "containerLabel"),
	containerEntry(glade, "containerEntry"),
	// テーブル部
	tableStack(glade, "tableStack"),
	table3columns1stBox(glade, "table3columns1stBox"),
	table3columns1st(glade, "table3columns1st"),
	liststore3columns1st(
		table3columns1st,
		{ G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA, GDK_TYPE_RGBA },
		[this](sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) { this->main_ListStore_setter(store, iter, row); }
	),
	table3columns2ndBox(glade, "table3columns2ndBox"),
	table3columns2nd(glade, "table3columns2nd"),
	liststore3columns2nd(
		table3columns2nd,
		{ G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA, GDK_TYPE_RGBA },
		[this](sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) { this->main_ListStore_setter(store, iter, row); }
	),
	table3columns3rdBox(glade, "table3columns3rdBox"),
	table3columns3rd(glade, "table3columns3rd"),
	liststore3columns3rd(
		table3columns3rd,
		{ G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA, GDK_TYPE_RGBA },
		[this](sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) { this->main_ListStore_setter(store, iter, row); }
	),
	liststore3columnsAll(DISPLAY_ROWS_PER_COLUMN),
	paginationBox3columns(glade, "paginationBox3columns"),
	prevButton3columns(glade, "prevButton3columns"),
	nextButton3columns(glade, "nextButton3columns"),
	pageLabel3columns(glade, "pageLabel3columns"),
	table2columns1stBox(glade, "table2columns1stBox"),
	table2columns1st(glade, "table2columns1st"),
	liststore2columns1st(
		table2columns1st,
		{ G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA, GDK_TYPE_RGBA },
		[this](sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) { this->main_ListStore_setter(store, iter, row); }
	),
	table2columns2ndBox(glade, "table2columns2ndBox"),
	table2columns2nd(glade, "table2columns2nd"),
	liststore2columns2nd(
		table2columns2nd,
		{ G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA, GDK_TYPE_RGBA },
		[this](sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) { this->main_ListStore_setter(store, iter, row); }
	),
	liststore2columnsAll(DISPLAY_ROWS_PER_COLUMN),
	paginationBox2columns(glade, "paginationBox2columns"),
	prevButton2columns(glade, "prevButton2columns"),
	nextButton2columns(glade, "nextButton2columns"),
	pageLabel2columns(glade, "pageLabel2columns"),
	table2types1stBox(glade, "table2types1stBox"),
	table2types1st(glade, "table2types1st"),
	liststore2types1st(
		table2types1st,
		{ G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA, GDK_TYPE_RGBA },
		[this](sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) { this->main_ListStore_setter(store, iter, row); }
	),
	paginatedListstore2types1st(DISPLAY_ROWS_PER_COLUMN),
	paginationBox2types1st(glade, "paginationBox2types1st"),
	prevButton2types1st(glade, "prevButton2types1st"),
	nextButton2types1st(glade, "nextButton2types1st"),
	pageLabel2types1st(glade, "pageLabel2types1st"),
	table2types2ndBox(glade, "table2types2ndBox"),
	table2types2nd(glade, "table2types2nd"),
	liststore2types2nd(
		table2types2nd,
		{ G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA, GDK_TYPE_RGBA },
		[this](sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) { this->main_ListStore_setter(store, iter, row); }
	),
	paginatedListstore2types2nd(DISPLAY_ROWS_PER_COLUMN),
	paginationBox2types2nd(glade, "paginationBox2types2nd"),
	prevButton2types2nd(glade, "prevButton2types2nd"),
	nextButton2types2nd(glade, "nextButton2types2nd"),
	pageLabel2types2nd(glade, "pageLabel2types2nd"),
	// フッター部
	qrCodeEntryBox(glade, "qrCodeEntryBox"),
	qrCodeEntryLabel(glade, "qrCodeEntryLabel"),
	qrCodeEntry(glade, "qrCodeEntry"),
	falseButton(glade, "falseButton"),
	trueButton(glade, "trueButton"),
	// ダイアログの親
	confirmInsertCountDialog(glade, "confirmInsertCountDialog"),
	confirmRegroundDialog(glade, "confirmRegroundDialog"),
	logoutDialog(glade, "logoutDialog"),
	messageDialog(glade, "messageDialog"),
	passwordEntryDialog(glade, "passwordEntryDialog"),
	warningDialog(glade, "warningDialog"),
	// インサート枚数確認ダイアログ内
	confirmInsertCountLabel(glade, "confirmInsertCountLabel"),
	insertCountIncrementButton(glade, "insertCountIncrementButton"),
	insertCountDecrementButton(glade, "insertCountDecrementButton"),
	insertCountEntry(glade, "insertCountEntry"),
	insertCountDetectButton(glade, "insertCountDetectButton"),
	insertCountOkButton(glade, "insertCountOkButton"),
	// 再研磨確認ダイアログ内
	confirmRegroundLabel(glade, "confirmRegroundLabel"),
	notRegroundButton(glade, "notRegroundButton"),
	regroundButton(glade, "regroundButton"),
	confirmRegroundOkButton(glade, "confirmRegroundOkButton"),
	// ログアウト確認ダイアログ内
	logoutNoButton(glade, "logoutNoButton"),
	logoutYesButton(glade, "logoutYesButton"),
	// メッセージダイアログ内
	messageDialogLabel(glade, "messageDialogLabel"),
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
	m_is_programmatically_changing_toggle(false),
	m_password_authenticated(false),
	m_gui_mode(GuiMode::RETRIEVE),
	m_app_state(AppState::WAITING_FOR_USER_LOGIN),
	m_text_mode(TextMode::KANJI),
	m_plc_ready_for_action(true),		// 最初は扉が閉まっているので true
	m_tracker_ready_for_action(true)	// 最初は不正なしなので true
{
	// --- コンストラクタ本体 ---

	// 1. Gladeファイルの "mainWindow" の定義からウィンドウを初期化
	this->init(glade, "mainWindow");

	// 2. ウィンドウの閉じるボタンでアプリケーションが終了するように設定
	this->setDefaultCloseOperation(sgex::Window::CloseOperation::DESTROY);

	// ダイアログは破棄せず、非表示にするだけ
	confirmInsertCountDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);
	confirmRegroundDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);
	logoutDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);
	messageDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);
	passwordEntryDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);
	warningDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);

	this->set_timeout(100);

	this->fullscreen();	// フルスクリーンで表示

	liststore3columnsAll.add_model({&liststore3columns1st, &liststore3columns2nd, &liststore3columns3rd});
	liststore2columnsAll.add_model({&liststore2columns1st, &liststore2columns2nd});
	paginatedListstore2types1st.add_model(&liststore2types1st);
	paginatedListstore2types2nd.add_model(&liststore2types2nd);

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
	listChangeButton.on_clicked([this]() { this->set_app_state(AppState::RETRIEVE_WAITING_FOR_PROGRAM_QR); });
	liststore3columnsAll.on_user_selection_changed([this](std::optional<size_t> selected_index_opt) { this->mainTableSelection_on_changed(selected_index_opt); });
	prevButton3columns.on_clicked([this]() { this->prevButton3columns_on_clicked(); });
	nextButton3columns.on_clicked([this]() { this->nextButton3columns_on_clicked(); });
	liststore2columnsAll.on_user_selection_changed([this](std::optional<size_t> selected_index_opt) { this->mainTableSelection_on_changed(selected_index_opt); });
	prevButton2columns.on_clicked([this]() { this->prevButton2columns_on_clicked(); });
	nextButton2columns.on_clicked([this]() { this->nextButton2columns_on_clicked(); });
	prevButton2types1st.on_clicked([this]() { this->prevButton2types1st_on_clicked(); });
	nextButton2types1st.on_clicked([this]() { this->nextButton2types1st_on_clicked(); });
	prevButton2types2nd.on_clicked([this]() { this->prevButton2types2nd_on_clicked(); });
	nextButton2types2nd.on_clicked([this]() { this->nextButton2types2nd_on_clicked(); });
	qrCodeEntry.on_activate([this]() { this->qrCodeEntry_on_activate(); });
	falseButton.on_clicked([this]() { this->falseButton_on_clicked(); });
	trueButton.on_clicked([this]() { this->trueButton_on_clicked(); });
	logoutNoButton.on_clicked([this]() { this->logoutDialog.response(GTK_RESPONSE_NO); });
	logoutYesButton.on_clicked([this]() { this->logoutDialog.response(GTK_RESPONSE_YES); });
	confirmInsertCountDialog.on_show([this]() { this->confirmInsertCountDialog_on_show(); });
	passwordEntryDialog.on_show([this]() { this->passwordEntryDialog_on_show(); });
	warningDialog.on_show([this]() { this->refresh_ui(false); });
	insertCountIncrementButton.on_clicked([this]() { this->insertCountIncrementButton_on_clicked(); });
	insertCountDecrementButton.on_clicked([this]() { this->insertCountDecrementButton_on_clicked(); });
	insertCountDetectButton.on_clicked([this]() { this->insertCountDetectButton_on_clicked(); });
	insertCountOkButton.on_clicked([this]() { this->insertCountOkButton_on_clicked(); });
	regroundButton.on_toggled([this]() { this->regroundButton_on_toggled(); });
	notRegroundButton.on_toggled([this]() { this->notRegroundButton_on_toggled(); });
	confirmRegroundOkButton.on_clicked([this]() { confirmRegroundDialog.response(GTK_RESPONSE_OK); });
	passwordCancelButton.on_clicked([this]() { passwordEntryDialog.response(GTK_RESPONSE_CANCEL); });
	passwordEnterButton.on_clicked([this]() { passwordEntryDialog.response(GTK_RESPONSE_OK); });
	passwordEntry.on_activate([this]() { passwordEntryDialog.response(GTK_RESPONSE_OK); });	// 確定ボタンがクリックされた時と同様

	//=========================================================================
	// ウィジェットの状態を保持するマップの構築
	//=========================================================================

	// --- 「取り出し」トグルボタン ---
	m_widget_states.try_emplace(&retrieveToggleButton, WidgetStateUpdater{
		{
			.text = UiText{"取り出し", "とりだし"},
			.can_focus		= false,
			.is_sensitive	= false,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->retrieveToggleButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->retrieveToggleButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->retrieveToggleButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->retrieveToggleButton.set_visible(state.is_visible.value()); }
		}
	});

	// --- 「補充」トグルボタン ---
	m_widget_states.try_emplace(&storeToggleButton, WidgetStateUpdater{
		{
			.text = UiText{"補充", "ほじゅう"},
			.can_focus		= false,
			.is_sensitive	= false,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->storeToggleButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->storeToggleButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->storeToggleButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->storeToggleButton.set_visible(state.is_visible.value()); }
		}
	});

	// --- 「棚卸し」トグルボタン ---
	m_widget_states.try_emplace(&reconcileToggleButton, WidgetStateUpdater{
		{
			.text = UiText{"棚卸し", "たなおろし"},
			.can_focus		= false,
			.is_sensitive	= false,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->reconcileToggleButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->reconcileToggleButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->reconcileToggleButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->reconcileToggleButton.set_visible(state.is_visible.value()); }
		}
	});

	// --- 「かな/漢字」ボタン ---
	m_widget_states.try_emplace(&kanaKanjiButton, WidgetStateUpdater{
		{
			.text = UiText{"かな/漢字", "かな/漢字"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->kanaKanjiButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->kanaKanjiButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->kanaKanjiButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->kanaKanjiButton.set_visible(state.is_visible.value()); }
		}
	});

	// --- 「ログアウト」ボタン ---
	m_widget_states.try_emplace(&logoutButton, WidgetStateUpdater{
		{
			.text = UiText{"ログアウト", "ログアウト"},
			.can_focus		= false,
			.is_sensitive	= false,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->logoutButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->logoutButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->logoutButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->logoutButton.set_visible(state.is_visible.value()); }
		}
	});

	// --- ユーザーIDエントリ ---
	m_widget_states.try_emplace(&userIdEntry, WidgetStateUpdater{
		{
			.can_focus		= true,
			.is_sensitive	= true,
			.is_visible		= true,
			.is_editable	= true
		},
		[this](const WidgetState& state){
			if (state.can_focus.has_value()		) { this->userIdEntry.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->userIdEntry.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->userIdEntry.set_visible(state.is_visible.value()); }
			if (state.is_editable.has_value()	) { this->userIdEntry.set_editable(state.is_editable.value()); }
		}
	});

	// --- 「ライン番号」ラベル ---
	m_widget_states.try_emplace(&lineLabel, WidgetStateUpdater{
		{
			.text = UiText{"ライン番号　　：", "ラインばんごう　　："},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->lineLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->lineLabel.set_visible(state.is_visible.value()); }
		}
	});

	// --- 「プログラム番号」ラベル ---
	m_widget_states.try_emplace(&programLabel, WidgetStateUpdater{
		{
			.text = UiText{"プログラム番号：", "プログラムばんごう："},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->programLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->programLabel.set_visible(state.is_visible.value()); }
		}
	});

	// --- 「ツーリングリスト変更」ボタン ---
	m_widget_states.try_emplace(&listChangeButton, WidgetStateUpdater{
		{
			.text = UiText{"ツーリングリスト\n　　　変更", "ツーリングリスト\n　　へんこう"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->listChangeButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->listChangeButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->listChangeButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->listChangeButton.set_visible(state.is_visible.value()); }
		}
	});

	// --- 「絞り込み」ボタン ---
	m_widget_states.try_emplace(&filterButton, WidgetStateUpdater{
		{
			.text = UiText{"絞り込み", "しぼりこみ"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->filterButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->filterButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->filterButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->filterButton.set_visible(state.is_visible.value()); }
		}
	});

	// --- 「コンテナ番号」ラベル ---
	m_widget_states.try_emplace(&containerLabel, WidgetStateUpdater{
		{
			.text = UiText{"コンテナ番号：", "コンテナばんごう："},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->containerLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->containerLabel.set_visible(state.is_visible.value()); }
		}
	});

	// --- メッセージラベル ---
	m_widget_states.try_emplace(&messageLabel, WidgetStateUpdater{
		{
			.text = UiText{"", ""},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->messageLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->messageLabel.set_visible(state.is_visible.value()); }
		}
	});

	// --- 3列表示テーブルのボックス (1) ---
	m_widget_states.try_emplace(&table3columns1stBox, WidgetStateUpdater{
		{
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.is_visible.has_value()	) { this->table3columns1stBox.set_visible(state.is_visible.value()); }
		}
	});

	// --- 3列表示テーブルのボックス (2) ---
	m_widget_states.try_emplace(&table3columns2ndBox, WidgetStateUpdater{
		{
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.is_visible.has_value()	) { this->table3columns2ndBox.set_visible(state.is_visible.value()); }
		}
	});

	// --- 3列表示テーブルのボックス (3) ---
	m_widget_states.try_emplace(&table3columns3rdBox, WidgetStateUpdater{
		{
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.is_visible.has_value()	) { this->table3columns3rdBox.set_visible(state.is_visible.value()); }
		}
	});

	// --- 3列表示テーブルのページネーション(ページ送り)UIを格納するボックス ---
	m_widget_states.try_emplace(&paginationBox3columns, WidgetStateUpdater{
		{
			.is_visible		= false
		},
		[this](const WidgetState& state){
			if (state.is_visible.has_value()	) { this->paginationBox3columns.set_visible(state.is_visible.value()); }
		}
	});

	// --- 3列表示テーブルのページ戻りボタン ---
	m_widget_states.try_emplace(&prevButton3columns, WidgetStateUpdater{
		{
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.can_focus.has_value()		) { this->prevButton3columns.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->prevButton3columns.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->prevButton3columns.set_visible(state.is_visible.value()); }
		}
	});

	// --- 3列表示テーブルのページ送りボタン ---
	m_widget_states.try_emplace(&nextButton3columns, WidgetStateUpdater{
		{
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.can_focus.has_value()		) { this->nextButton3columns.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->nextButton3columns.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->nextButton3columns.set_visible(state.is_visible.value()); }
		}
	});

	// --- 2列表示テーブルのボックス (1) ---
	m_widget_states.try_emplace(&table2columns1stBox, WidgetStateUpdater{
		{
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.is_visible.has_value()	) { this->table2columns1stBox.set_visible(state.is_visible.value()); }
		}
	});

	// --- 2列表示テーブルのボックス (2) ---
	m_widget_states.try_emplace(&table2columns2ndBox, WidgetStateUpdater{
		{
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.is_visible.has_value()	) { this->table2columns2ndBox.set_visible(state.is_visible.value()); }
		}
	});

	// --- 2種表示テーブルのボックス (1種目) ---
	m_widget_states.try_emplace(&table2columns1stBox, WidgetStateUpdater{
		{
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.is_visible.has_value()	) { this->table2columns1stBox.set_visible(state.is_visible.value()); }
		}
	});

	// --- 2種表示テーブルのページネーション(ページ送り)UIを格納するボックス (1種目) ---
	m_widget_states.try_emplace(&paginationBox2types1st, WidgetStateUpdater{
		{
			.is_visible		= false
		},
		[this](const WidgetState& state){
			if (state.is_visible.has_value()	) { this->paginationBox2types1st.set_visible(state.is_visible.value()); }
		}
	});

	// --- 2列表示テーブルのページ戻りボタン (1種目) ---
	m_widget_states.try_emplace(&prevButton2types1st, WidgetStateUpdater{
		{
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.can_focus.has_value()		) { this->prevButton2types1st.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->prevButton2types1st.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->prevButton2types1st.set_visible(state.is_visible.value()); }
		}
	});

	// --- 2列表示テーブルのページ送りボタン (1種目) ---
	m_widget_states.try_emplace(&nextButton2types1st, WidgetStateUpdater{
		{
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.can_focus.has_value()		) { this->nextButton2types1st.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->nextButton2types1st.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->nextButton2types1st.set_visible(state.is_visible.value()); }
		}
	});

	// --- 2種表示テーブルのボックス (2種目) ---
	m_widget_states.try_emplace(&table2columns2ndBox, WidgetStateUpdater{
		{
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.is_visible.has_value()	) { this->table2columns2ndBox.set_visible(state.is_visible.value()); }
		}
	});

	// --- 2種表示テーブルのページネーション(ページ送り)UIを格納するボックス (2種目) ---
	m_widget_states.try_emplace(&paginationBox2types2nd, WidgetStateUpdater{
		{
			.is_visible		= false
		},
		[this](const WidgetState& state){
			if (state.is_visible.has_value()	) { this->paginationBox2types2nd.set_visible(state.is_visible.value()); }
		}
	});

	// --- 2列表示テーブルのページ戻りボタン (2種目) ---
	m_widget_states.try_emplace(&prevButton2types2nd, WidgetStateUpdater{
		{
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.can_focus.has_value()		) { this->prevButton2types2nd.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->prevButton2types2nd.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->prevButton2types2nd.set_visible(state.is_visible.value()); }
		}
	});

	// --- 2列表示テーブルのページ送りボタン (2種目) ---
	m_widget_states.try_emplace(&nextButton2types2nd, WidgetStateUpdater{
		{
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.can_focus.has_value()		) { this->nextButton2types2nd.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->nextButton2types2nd.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->nextButton2types2nd.set_visible(state.is_visible.value()); }
		}
	});

	// --- QRコード受付UIのボックス ---
	m_widget_states.try_emplace(&qrCodeEntryBox, WidgetStateUpdater{
		{
			.is_visible		= false
		},
		[this](const WidgetState& state){
			if (state.is_visible.has_value()	) { this->qrCodeEntryBox.set_visible(state.is_visible.value()); }
		}
	});

	// --- 「QRコード受付」ラベル ---
	m_widget_states.try_emplace(&qrCodeEntryLabel, WidgetStateUpdater{
		{
			.text = UiText{"QRコード受付：", "QRコード うけつけ："},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->qrCodeEntryLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->qrCodeEntryLabel.set_visible(state.is_visible.value()); }
		}
	});

	// --- QRコードを入力するエントリ ---
	m_widget_states.try_emplace(&qrCodeEntry, WidgetStateUpdater{
		{
			.can_focus		= false,
			.is_sensitive	= false,
			.is_visible		= true,
			.is_editable	= false
		},
		[this](const WidgetState& state){
			if (state.can_focus.has_value()		) { this->qrCodeEntry.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->qrCodeEntry.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->qrCodeEntry.set_visible(state.is_visible.value()); }
			if (state.is_editable.has_value()	) { this->qrCodeEntry.set_editable(state.is_editable.value()); }
		}
	});

	// --- true/falseボタン（ラベルは動的に変わるため空） ---
	m_widget_states.try_emplace(&falseButton, WidgetStateUpdater{
		{
			.text = UiText{"", ""},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->falseButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->falseButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->falseButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->falseButton.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&trueButton, WidgetStateUpdater{
		{
			.text = UiText{"", ""},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->trueButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->trueButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->trueButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->trueButton.set_visible(state.is_visible.value()); }
		}
	});

	// --- インサート枚数確認ダイアログ ---
	m_widget_states.try_emplace(&confirmInsertCountLabel, WidgetStateUpdater{
		{
			.text = UiText{"使用するインサートの枚数を\n入力してください", "しようする インサートの まいすうを\nにゅうりょく してください"},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->confirmInsertCountLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->confirmInsertCountLabel.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&insertCountDetectButton, WidgetStateUpdater{
		{
			.text = UiText{"自動検出", "じどうけんしゅつ"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->insertCountDetectButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->insertCountDetectButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->insertCountDetectButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->insertCountDetectButton.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&insertCountOkButton, WidgetStateUpdater{
		{
			.text = UiText{"OK", "OK"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->insertCountOkButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->insertCountOkButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->insertCountOkButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->insertCountOkButton.set_visible(state.is_visible.value()); }
		}
	});

	// --- 再研磨確認ダイアログ ---
	m_widget_states.try_emplace(&confirmRegroundLabel, WidgetStateUpdater{
		{
			.text = UiText{"一度取り出された工具を検出しました。\n再研磨された工具ですか？", "いちど とりだされた こうぐを けんしゅつしました。\nさいけんま された こうぐですか？"},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->confirmRegroundLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->confirmRegroundLabel.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&confirmRegroundOkButton, WidgetStateUpdater{
		{
			.text = UiText{"OK", "OK"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->confirmRegroundOkButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->confirmRegroundOkButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->confirmRegroundOkButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->confirmRegroundOkButton.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&notRegroundButton, WidgetStateUpdater{
		{
			.text = UiText{"いいえ", "いいえ"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->notRegroundButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->notRegroundButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->notRegroundButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->notRegroundButton.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&regroundButton, WidgetStateUpdater{
		{
			.text = UiText{"はい", "はい"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->regroundButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->regroundButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->regroundButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->regroundButton.set_visible(state.is_visible.value()); }
		}
	});

	// --- ログアウト確認ダイアログ ---
	m_widget_states.try_emplace(&logoutNoButton, WidgetStateUpdater{
		{
			.text = UiText{"いいえ", "いいえ"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->logoutNoButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->logoutNoButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->logoutNoButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->logoutNoButton.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&logoutYesButton, WidgetStateUpdater{
		{
			.text = UiText{"はい", "はい"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->logoutYesButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->logoutYesButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->logoutYesButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->logoutYesButton.set_visible(state.is_visible.value()); }
		}
	});

	// --- メッセージダイアログ（ラベルは動的に変わるため空） ---
	m_widget_states.try_emplace(&messageDialogLabel, WidgetStateUpdater{
		{
			.text = UiText{"", ""},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->messageDialogLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->messageDialogLabel.set_visible(state.is_visible.value()); }
		}
	});

	// --- パスワード入力ダイアログ ---
	m_widget_states.try_emplace(&passwordEntryLabel, WidgetStateUpdater{
		{
			.text = UiText{"パスワードを\n入力してください", "パスワードを\nにゅうりょく してください"},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->passwordEntryLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->passwordEntryLabel.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&passwordCancelButton, WidgetStateUpdater{
		{
			.text = UiText{"キャンセル", "キャンセル"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->passwordCancelButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->passwordCancelButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->passwordCancelButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->passwordCancelButton.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&passwordEnterButton, WidgetStateUpdater{
		{
			.text = UiText{"確定", "かくてい"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->passwordEnterButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->passwordEnterButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->passwordEnterButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->passwordEnterButton.set_visible(state.is_visible.value()); }
		}
	});

	// --- 棚卸し絞り込みダイアログ ---
	m_widget_states.try_emplace(&reconcileToolTypeFilterLabel, WidgetStateUpdater{
		{
			.text = UiText{"分類", "ぶんるい"},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->reconcileToolTypeFilterLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->reconcileToolTypeFilterLabel.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&reconcilePeriodFilterLabel, WidgetStateUpdater{
		{
			.text = UiText{"最終棚卸し日時", "さいしゅうたなおろしにちじ"},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->reconcilePeriodFilterLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->reconcilePeriodFilterLabel.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&reconcileFilterEnterButton, WidgetStateUpdater{
		{
			.text = UiText{"決定", "けってい"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->reconcileFilterEnterButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->reconcileFilterEnterButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->reconcileFilterEnterButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->reconcileFilterEnterButton.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&reconcileFilterResetButton, WidgetStateUpdater{
		{
			.text = UiText{"期間リセット", "きかんリセット"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->reconcileFilterResetButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->reconcileFilterResetButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->reconcileFilterResetButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->reconcileFilterResetButton.set_visible(state.is_visible.value()); }
		}
	});

	// --- 警告ダイアログ（ラベルは動的に変わるため空） ---
	m_widget_states.try_emplace(&warningLabel, WidgetStateUpdater{
		{
			.text = UiText{"", ""},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->warningLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->warningLabel.set_visible(state.is_visible.value()); }
		}
	});

	//=========================================================================
	// 各テーブルのカラムヘッダー表示テキストを保持するマップの構築
	//=========================================================================

	// --- 補充する工具の情報を表示するテーブル ---
	m_table_states.try_emplace(&storingToolTable, TableState{
		.column_headers = std::vector<UiText>{
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"}
		},
		.is_sensitive	= false,
		.is_visible		= true
	});

	// --- 3列表示テーブル ---
	m_table_states.try_emplace(&table3columns1st, TableState{
		.column_headers = std::vector<UiText>{
			{"ツール\n  番号", "  ツール\nばんごう"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"在庫数", "ざいこ\n  すう"}
		},
		.is_sensitive	= true,
		.is_visible		= true
	});
	m_table_states.try_emplace(&table3columns2nd, TableState{
		.column_headers = std::vector<UiText>{
			{"ツール\n  番号", "  ツール\nばんごう"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"在庫数", "ざいこ\n  すう"}
		},
		.is_sensitive	= true,
		.is_visible		= true
	});
	m_table_states.try_emplace(&table3columns3rd, TableState{
		.column_headers = std::vector<UiText>{
			{"ツール\n  番号", "  ツール\nばんごう"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"在庫数", "ざいこ\n  すう"}
		},
		.is_sensitive	= true,
		.is_visible		= true
	});

	// --- 2列表示テーブル ---
	m_table_states.try_emplace(&table2columns1st, TableState{
		.column_headers = std::vector<UiText>{
			{"ID", "ID"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"再研磨\n  回数", "さいけんま\n  かいすう"}
		},
		.is_sensitive	= true,
		.is_visible		= true
	});
	m_table_states.try_emplace(&table2columns2nd, TableState{
		.column_headers = std::vector<UiText>{
			{"ID", "ID"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"再研磨\n  回数", "さいけんま\n  かいすう"}
		},
		.is_sensitive	= true,
		.is_visible		= true
	});

	// --- 2種表示テーブル ---
	m_table_states.try_emplace(&table2types1st, TableState{
		.column_headers = std::vector<UiText>{
			{"ID", "ID"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"再研磨\n  回数", "さいけんま\n  かいすう"}
		},
		.is_sensitive	= true,
		.is_visible		= true
	});
	m_table_states.try_emplace(&table2types2nd, TableState{
		.column_headers = std::vector<UiText>{
			{"ID", "ID"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"再研磨\n  回数", "さいけんま\n  かいすう"}
		},
		.is_sensitive	= true,
		.is_visible		= true
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
			{ {&retrieveToggleButton, &storeToggleButton, &reconcileToggleButton, &logoutButton, &listChangeButton}, {.is_sensitive = false, .is_visible = true}},
			{ {&userIdEntry}, {.can_focus = true, .is_visible = true, .is_editable = true} },
			{ {&messageLabel}, {.text = UiText{"名札のQRコードを\nQRコードリーダーにかざしてください", "なふだの QRコードを\nQRコードリーダーに かざしてください"}} },
			{ {&qrCodeEntry}, {.can_focus = false, .is_sensitive = false, .is_editable = false} },
			{ {&falseButton, &trueButton}, {.text = UiText{"", ""}, .is_sensitive = false} }
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
			{ {&listChangeButton}, {.is_sensitive = false} },
			{ {&qrCodeEntry}, {.can_focus = true, .is_sensitive = true, .is_editable = true} },
			{ {&falseButton, &trueButton}, {.text = UiText{"", ""}, .is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.operationStackPageName = "retrieveOperationPage",
		.tableStackPageName		= "pageNone",	// 初期ページ
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_retrieve_waiting_for_program_qr(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_retrieve_waiting_for_program_qr(); }
	});

	// --- 状態：取り出し - 工具選択中 ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE_SELECTING_TOOLS, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"取り出す工具を選択してください", "とりだす こうぐを せんたく してください"}} },
			{ {&listChangeButton}, {.is_sensitive = true} },
			{ {&qrCodeEntry}, {.can_focus = false, .is_sensitive = false, .is_editable = false} },
			{ {&falseButton}, {.text = UiText{"全選択解除", "ぜんせんたくかいじょ"}, .is_sensitive = true} },
			{ {&trueButton}, {.text = UiText{"確定", "かくてい"}, .is_sensitive = false} }
		},
		.table_states = {
			{ {&table2columns1st, &table2columns2nd}, {
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
		.tableStackPageName		= "table2columns",
		// --- 状態の「振る舞い」を定義 ---
		.on_prepare_state				= [](MainWindow* thiz){ thiz->reset_header_buttons(false); },
		.on_enter_state					= [](MainWindow* thiz){ thiz->on_enter_retrieve_selecting_tools(); },
		.on_mainTableSelection_changed	= [](MainWindow* thiz, std::optional<size_t> selected_index_opt){ thiz->on_mainTableSelection_retrieve_selecting_tools(selected_index_opt); },
		.on_falseButton_clicked			= [](MainWindow* thiz){ thiz->on_falseButton_retrieve_selecting_tools(); },
		.on_trueButton_clicked			= [](MainWindow* thiz){ thiz->set_app_state(AppState::RETRIEVE_CONFIRMING_SELECTION); }
	});

	// --- 状態：取り出し - 選択内容の確認中 ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE_CONFIRMING_SELECTION, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"選択した工具は以下の通りです\nよろしければ「搬出」を押してください", "せんたくした こうぐは いかのとおりです..."}} },
			{ {&listChangeButton}, {.is_sensitive = false} },
			{ {&falseButton}, {.text = UiText{"キャンセル", "キャンセル"}, .is_sensitive = true} },
			{ {&trueButton}, {.text = UiText{"搬出", "はんしゅつ"}, .is_sensitive = true} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName		= "table2columns",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state			= [](MainWindow* thiz){ thiz->on_enter_retrieve_confirming_selection(); },
		.on_falseButton_clicked	= [](MainWindow* thiz){ thiz->on_falseButton_retrieve_confirming_selection(); },
		.on_trueButton_clicked	= [](MainWindow* thiz){ thiz->on_trueButton_retrieve_confirming_selection(); }
	});

	// --- 状態：取り出し - コンテナ搬出待ち ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE_CONTAINER_RETRIEVING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&storeToggleButton, &reconcileToggleButton, &logoutButton}, {.is_sensitive = false} },
			{ {&messageLabel}, {.text = UiText{"コンテナを搬出しています\nしばらくお待ち下さい", "コンテナを はんしゅつ しています\nしばらく おまちください"}} },
			{ {&falseButton, &trueButton}, {.text = UiText{"", ""}, .is_sensitive = false} }
		},
		.table_states = {
			{ {&table2columns1st, &table2columns2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName		= "table2columns",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state		= [](MainWindow* thiz){ thiz->on_enter_retrieve_container_retrieving(); },
		.on_plc_notified	= [](MainWindow* thiz, const plc::Notification& notification){ thiz->on_plc_retrieve_container_retrieving(notification); }
	});

	// --- 状態：取り出し - コンテナ搬出完了 ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE_CONTAINER_RETRIEVED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"コンテナが届きました\n扉を開けてコンテナを取り出してください", "コンテナが とどきました\nとびらを あけて コンテナを とりだしてください"}} },
		},
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state		= [](MainWindow* thiz){ thiz->on_enter_retrieve_container_retrieved(); },
		.on_plc_notified	= [](MainWindow* thiz, const plc::Notification& notification){ thiz->on_plc_retrieve_container_retrieved(notification); }
	});

	// --- 状態：取り出し - 実際に補充を行う - 工具読み取り前 ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE_READY_FOR_READING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"コンテナから工具を取り出して、工具のQRコードを\nQRコードリーダーで読み取ってください", "コンテナから こうぐを とりだして、こうぐの QRコードを\nQRコードリーダーで よみとってください"}} },
			{ {&qrCodeEntry}, {.can_focus = true, .is_sensitive = true, .is_editable = true} },
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName	= "pageNone",
		// --- 状態の「振る舞い」を定義 ---
		.on_prepare_state	= [](MainWindow* thiz){
			bool is_last_tool = (!thiz->m_retrieving_index.has_value() || thiz->m_retrieving_index.value() + 1 >= thiz->m_retrieving_tools.size());
			thiz->update_widget_state(&(thiz->falseButton), {
				.text = ((is_last_tool)
					? (UiText{"終了", "しゅうりょう"})	// 最後の工具だった場合
					: (UiText{"", ""})					// 最後の工具で無かった場合
				),
				.is_sensitive = false,
				.is_visible = true
			}, false);
			thiz->update_widget_state(&(thiz->trueButton), {
				.text = ((is_last_tool)
					? (UiText{"続ける", "つづける"})	// 最後の工具だった場合
					: (UiText{"次へ", "つぎへ"})		// 最後の工具で無かった場合
				),
				.is_sensitive = false,
				.is_visible = true
			}, false);
			thiz->update_table_state({&(thiz->table2columns1st), &(thiz->table2columns2nd)}, {
				.column_headers = std::vector<UiText>{
					{"ID", "ID"},
					{"分類", "ぶんるい"},
					{"メーカー", "メーカー"},
					{"型番", "かたばん"},
					{"材質", "ざいしつ"},
					((thiz->m_is_insert_operation)
						? (UiText{"取り出し\n　個数", "とりだし\n  こすう"})	// インサートの操作だった場合
						: (UiText{"再研磨\n  回数", "さいけんま\n  かいすう"})	// インサートの操作以外の場合
					)
				}, .is_sensitive = true
			}, false);
		},
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_retrieve_ready_for_reading(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_retrieve_ready_for_reading(); },
		.on_tracker_notified		= [](MainWindow* thiz, const tracker::Notification& notification){ thiz->on_tracker_retrieve_ready_for_reading(notification); }
	});

	// --- 状態：取り出し - 実際に補充を行う - 工具読み取り後 ---
	g_app_state_configs.try_emplace(AppState::RETRIEVE_READ_COMPLETED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName	= "table2columns",
		// --- 状態の「振る舞い」を定義 ---
		.on_prepare_state	= [](MainWindow* thiz){
			thiz->update_widget_state(&(thiz->messageLabel), {
				.text = ((!thiz->m_retrieving_index.has_value() || thiz->m_retrieving_index.value() + 1 >= thiz->m_retrieving_tools.size())
					? (UiText{"取り出し終わったらコンテナを戻して\n扉を閉めてください", "とりだしおわったら コンテナを もどして\nとびらを しめてください"})
					: (UiText{"取り出し終わったら扉を閉めて\n「次へ」を押してください", "とりだしおわったら とびらを しめて\n「つぎへ」を おしてください"})
				)
			}, false);
		},
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_retrieve_read_completed(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_retrieve_ready_for_reading(); },
		.on_falseButton_clicked		= [](MainWindow* thiz){ thiz->on_falseButton_retrieve_read_completed(); },
		.on_trueButton_clicked		= [](MainWindow* thiz){ thiz->on_trueButton_retrieve_read_completed(); },
		.on_plc_notified			= [](MainWindow* thiz, const plc::Notification& notification){ thiz->on_plc_retrieve_read_completed(notification); },
		.on_tracker_notified		= [](MainWindow* thiz, const tracker::Notification& notification){ thiz->on_tracker_retrieve_ready_for_reading(notification); }
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
			{ {&qrCodeEntry}, {.can_focus = true, .is_sensitive = true, .is_editable = true} },
			{ {&falseButton, &trueButton}, {.text = UiText{"", ""}, .is_sensitive = false} }
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
			{ {&qrCodeEntry}, {.can_focus = false, .is_sensitive = false, .is_editable = false} },
			{ {&falseButton}, {.text = UiText{"キャンセル", "キャンセル"}, .is_sensitive = true} },
			{ {&trueButton}, {.text = UiText{"搬出", "はんしゅつ"}, .is_sensitive = true} }
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
			{ {&falseButton, &trueButton}, {.text = UiText{"", ""}, .is_sensitive = false} }
		},
		.table_states = {
			{ {&table2columns1st, &table2columns2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName	= "pageNone",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state		= [](MainWindow* thiz){ thiz->on_enter_store_container_retrieving(); },
		.on_plc_notified	= [](MainWindow* thiz, const plc::Notification& notification){ thiz->on_plc_store_container_retrieving(notification); }
	});

	// --- 状態：補充 - コンテナ搬出完了 ---
	g_app_state_configs.try_emplace(AppState::STORE_CONTAINER_RETRIEVED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"コンテナが届きました\n扉を開けてコンテナを取り出してください", "コンテナが とどきました\nとびらを あけて コンテナを とりだしてください"}} },
		},
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state		= [](MainWindow* thiz){ thiz->on_enter_store_container_retrieved(); },
		.on_plc_notified	= [](MainWindow* thiz, const plc::Notification& notification){ thiz->on_plc_store_container_retrieved(notification); }
	});

	// --- 状態：補充 - 実際に補充を行う - 工具読み取り前 ---
	g_app_state_configs.try_emplace(AppState::STORE_READY_FOR_READING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"工具のQRコードを QRコードリーダーで\n読み取ってからコンテナに収納してください", "こうぐの QRコードを QRコードリーダーで\nよみとってから コンテナに しゅうのうしてください"}} },
			{ {&qrCodeEntry}, {.can_focus = true, .is_sensitive = true, .is_editable = true} },
			{ {&falseButton}, {.text = UiText{"終了", "しゅうりょう"}, .is_sensitive = false} },
			{ {&trueButton}, {.text = UiText{"続ける", "つづける"}, .is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName	= "pageNone",
		// --- 状態の「振る舞い」を定義 ---
		.on_prepare_state	= [](MainWindow* thiz){
			thiz->update_table_state({&(thiz->table2columns1st), &(thiz->table2columns2nd)}, {
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
			}, false);
		},
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_ready_for_reading(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_store_ready_for_reading(); }
	});

	// --- 状態：補充 - 実際に補充を行う - 工具読み取り後 ---
	g_app_state_configs.try_emplace(AppState::STORE_READ_COMPLETED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"補充が終わったらコンテナを戻して\n扉を閉めてください", "ほじゅうが おわったら コンテナを もどして\nとびらを しめてください"}} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName	= "table2columns",
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
			{ {&qrCodeEntry}, {.can_focus = false, .is_sensitive = false, .is_editable = false} },
			{ {&falseButton}, {.text = UiText{"", ""}, .is_sensitive = false} },
			{ {&trueButton}, {.text = UiText{"搬出", "はんしゅつ"}, .is_sensitive = false} }
		},
		.table_states = {
			{ {&table2columns1st, &table2columns2nd}, {
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
		.tableStackPageName		= "table2columns",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state					= [](MainWindow* thiz){ thiz->on_enter_reconcile_waiting_for_container_selection(); },
		.on_mainTableSelection_changed	= [](MainWindow* thiz, std::optional<size_t> selected_index_opt){ thiz->on_mainTableSelection_reconcile_waiting_for_container_selection(selected_index_opt); },
		.on_trueButton_clicked			= [](MainWindow* thiz){ thiz->set_app_state(AppState::RECONCILE_CONTAINER_RETRIEVING); }
	});

	// --- 状態：棚卸し - コンテナ搬出待ち ---
	g_app_state_configs.try_emplace(AppState::RECONCILE_CONTAINER_RETRIEVING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&retrieveToggleButton, &storeToggleButton, &logoutButton}, {.is_sensitive = false} },
			{ {&messageLabel}, {.text = UiText{"コンテナを搬出しています\nしばらくお待ち下さい", "コンテナを はんしゅつ しています\nしばらく おまちください"}} },
			{ {&falseButton, &trueButton}, {.text = UiText{"", ""}, .is_sensitive = false} }
		},
		.table_states = {
			{ {&table2columns1st, &table2columns2nd}, {.is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.operationStackPageName	= "pageNone",
		.tableStackPageName		= "table2columns",
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state		= [](MainWindow* thiz){ thiz->on_enter_reconcile_container_retrieving(); },
		.on_plc_notified	= [](MainWindow* thiz, const plc::Notification& notification){ thiz->on_plc_reconcile_container_retrieving(notification); }
	});

	// --- 状態：棚卸し - コンテナ搬出完了 ---
	g_app_state_configs.try_emplace(AppState::RECONCILE_CONTAINER_RETRIEVED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"コンテナが届きました\n扉を開けてコンテナを取り出してください", "コンテナが とどきました\nとびらを あけて コンテナを とりだしてください"}} }
		},
		// --- 状態の「振る舞い」を定義 ---
		.on_enter_state		= [](MainWindow* thiz){ thiz->on_enter_reconcile_container_retrieving(); },
		.on_plc_notified	= [](MainWindow* thiz, const plc::Notification& notification){ thiz->on_plc_reconcile_container_retrieving(notification); }
	});

	// --- 状態：棚卸し - スキャン待ち ---
	g_app_state_configs.try_emplace(AppState::RECONCILE_READY_FOR_READING, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"コンテナ内の工具をすべてQRコードリーダーで読み取ってください", "コンテナないの こうぐを すべて QRコードリーダーで よみとってください"}} },
			{ {&falseButton}, {.text = UiText{"終了", "しゅうりょう"}, .is_sensitive = false} },
			{ {&trueButton}, {.text = UiText{"続ける", "つづける"}, .is_sensitive = false} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.operationStackPageName	= "reconcileInfoPage",
		.tableStackPageName		= "table2columns",
		// --- 状態の「振る舞い」を定義 ---
		.on_prepare_state	= [](MainWindow* thiz){
			thiz->update_table_state({&(thiz->table2columns1st), &(thiz->table2columns2nd)}, {
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
			}, false);
		},
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_reconcile_ready_for_reading(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_reconcile_ready_for_reading(); }
	});

	// --- 状態：棚卸し - 工具読み取り後 ---
	g_app_state_configs.try_emplace(AppState::RECONCILE_READ_COMPLETED, WidgetStateConfig{
		// --- UIの「見た目」を定義 ---
		.widget_states = {
			{ {&messageLabel}, {.text = UiText{"工具を全てスキャンできたら\n工具とコンテナを戻して扉を閉めてください", "こうぐを すべてスキャンできたら\nこうぐと コンテナを もどして とびらを しめてください"}} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName	= "table2columns",
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

void MainWindow::plc_on_notified(const plc::Notification& notification) {
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
					}}, true);
					warningDialog.show_all();
				} else {
					warningDialog.hide();
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
		} else {
			// FIXME: nullopt対応
			DEV_PRINT(stderr, "PLC監視スレッドからMONITOR通知を受け取りましたが、monitor_result が空でした\n");
		}
		break;
	default :
		DEV_PRINT(stderr, "Error: 未知のPLC通知を受け取りました\n");
	}
}

void MainWindow::tracker_on_notified(const tracker::Notification& notification) {
	try {
		auto state_config = g_app_state_configs.at(m_app_state);
		if (state_config.on_tracker_notified) {
			state_config.on_tracker_notified(this, notification);
		}
	} catch (const std::out_of_range& e) {
		DEV_PRINT(stderr, "Error: 未定義の状態で追跡システム通知を受け取りました (AppState={})\n", static_cast<int>(m_app_state));
		// 例外を無視して処理を続行
	}
}

//=============================================================================
// プライベートメンバ関数 (ヘルパー関数) 実装
//=============================================================================

void MainWindow::reset_variables() {
	m_is_insert_operation = false;
}

void MainWindow::reset_header_buttons(bool update_ui) {
	// 権限に応じてトグルボタンの操作可否を設定
	MstUserInfo logged_in_user_info = g_logged_in_user_info.value_or(MstUserInfo{.per_out = false});	// FIXME: nullopt対応
	// 取り出しトグルボタン
	update_widget_state(&retrieveToggleButton, WidgetState{
		.is_sensitive = logged_in_user_info.per_out,
	}, false);
	// 補充トグルボタン
	update_widget_state(&storeToggleButton, WidgetState{
		.is_sensitive = logged_in_user_info.per_add,
	}, false);
	// 棚卸しトグルボタン
	update_widget_state(&reconcileToggleButton, WidgetState{
		.is_sensitive = logged_in_user_info.per_inventory,
	}, false);
	// ログアウトボタンを操作可能に
	update_widget_state(&logoutButton, WidgetState{
		.is_sensitive = true
	}, update_ui);
}

void MainWindow::update_widget_state(sg::Widget* widget, WidgetState state, bool update_ui) {
	// m_widget_statesから現在のWidgetStateUpdaterのコピーを取得
	auto updater = m_widget_states.at(widget);

	// stateメンバーだけを新しい状態に更新
	if (updater.state.merge(state)) {
		// 変更があった場合、更新したWidgetStateUpdaterをマップに書き戻す
		m_widget_states.insert_or_assign(widget, updater);
	}

	if (update_ui) {
		refresh_ui(false);
	}
}

void MainWindow::update_widget_state(std::vector<sg::Widget*> widgets, WidgetState state, bool update_ui) {
	for (auto* widget : widgets) {
		update_widget_state(widget, state, false);
	}

	if (update_ui) {
		refresh_ui(false);
	}
}

void MainWindow::update_table_state(sgex::TreeView* table, TableState state, bool update_ui) {
	// m_table_statesから現在のTableStateのコピーを取得
	auto current_state = m_table_states.at(table);

	// stateメンバーだけを新しい状態に更新
	if (current_state.merge(state)) {
		// 変更があった場合、更新したTableStateをマップに書き戻す
		m_table_states.insert_or_assign(table, current_state);
	}

	if (update_ui) {
		refresh_ui(false);
	}
}

void MainWindow::update_table_state(std::vector<sgex::TreeView*> tables, TableState state, bool update_ui) {
	for (auto* table : tables) {
		update_table_state(table, state, false);
	}

	if (update_ui) {
		refresh_ui(false);
	}
}

void MainWindow::refresh_ui(bool force_redraw_all) {
	// 1. 更新対象となるウィジェットのリストを取得
	auto widgets_to_update = ((force_redraw_all)
		? (m_widget_states.get_all(true))				// 全ウィジェットを取得し、取得後に更新リストをクリア
		: (m_widget_states.get_updated_elements(true))	// 更新されたウィジェットのみ取得し、取得後に更新リストをクリア
	);

	// 2. 対象ウィジェットの表示を更新
	for (const auto& [widget, state_updater] : widgets_to_update) {
		// セッター関数が登録されていれば呼び出す
		if (state_updater.setter) {
			// セッター関数を呼び出して、テキストと状態を更新
			state_updater.setter(state_updater.state);
		}
	}

	// 3. 更新対象となるテーブルのリストを取得
	auto tables_to_update = ((force_redraw_all)
		? (m_table_states.get_all(true))				// 全テーブルを取得し、取得後に更新リストをクリア
		: (m_table_states.get_updated_elements(true))	// 更新されたテーブルのみ取得し、取得後に更新リストをクリア
	);

	// 4. 対象テーブルのカラムヘッダー表示を更新
	for (const auto& [table, headers] : tables_to_update) {
		if (headers.column_headers.has_value()) {
			for (guint i = 0; i < headers.column_headers.value().size(); i++) {
				sgex::TreeViewColumn column(table->get_column(i));
				if ((GtkTreeViewColumn*)column) {
					const auto& header = headers.column_headers.value()[i];
					const std::string& title = (m_text_mode == TextMode::KANA) ? header.kana : header.kanji;
					column.set_title(title.c_str());
				}
			}
		}
		if (headers.is_sensitive.has_value()	) { table->set_sensitive(headers.is_sensitive.value()); }
		if (headers.is_visible.has_value()		) { table->set_visible(headers.is_visible.value()); }
	}
}

void MainWindow::set_app_state(AppState state) {
	// メンバ変数の状態を更新 & 一部リセット
	m_app_state = state;

	// m_widget_statesを更新する
	const auto& state_config = g_app_state_configs.at(state);
	for (const auto& widget_state : state_config.widget_states) {
		for (const auto& widget : widget_state.widgets) {
			update_widget_state(widget, widget_state.state, false);
		}
	}

	// m_table_statesを更新する
	for (const auto& table_state : state_config.table_states) {
		for (const auto& table : table_state.tables) {
			update_table_state(table, table_state.state, false);
		}
	}

	// 各スタックの表示ページを更新
	if (!state_config.operationStackPageName.empty() && operationStack.get_visible_child_name() != state_config.operationStackPageName) {
		operationStack.set_visible_child_name(state_config.operationStackPageName.c_str());
	}
	if (!state_config.tableStackPageName.empty() && tableStack.get_visible_child_name() != state_config.tableStackPageName) {
		tableStack.set_visible_child_name(state_config.tableStackPageName.c_str());
		update_and_handle_pagination(PaginationAction::UPDATE_UI_ONLY, false);
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
			utils::scoped_value_guard guard(m_is_programmatically_changing_toggle, true);
			retrieveToggleButton.set_active(m_gui_mode == GuiMode::RETRIEVE);
			storeToggleButton.set_active(m_gui_mode == GuiMode::STORE);
			reconcileToggleButton.set_active(m_gui_mode == GuiMode::RECONCILE);
			return;
		}
	}

	// 1. トグルボタンの状態を排他的に更新
	{
		utils::scoped_value_guard guard(m_is_programmatically_changing_toggle, true);
		retrieveToggleButton.set_active(mode == GuiMode::RETRIEVE);
		storeToggleButton.set_active(mode == GuiMode::STORE);
		reconcileToggleButton.set_active(mode == GuiMode::RECONCILE);
	}

	// 2. モードが変化する場合、モードに応じて現在の状態を切り替え
	if (m_gui_mode != mode) {
		reset_variables();
		reset_header_buttons(false);

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

void MainWindow::update_and_handle_pagination(PaginationAction action, bool update_ui) {
	// 現在の状態設定から、アクティブなテーブルページ名を取得
	std::string page_name(tableStack.get_visible_child_name());

	// ページ名に応じて、適切なヘルパーを呼び出す
	if (page_name == "table3columns") {
		update_and_handle_pagination_3columns(action, false);
	} else if (page_name == "table2columns") {
		update_and_handle_pagination_2columns(action, false);
	} else if (page_name == "table2types") {
		update_and_handle_pagination_2types1st(action, false);
		update_and_handle_pagination_2types2nd(action, false);
	}

	// 画面表示を引数に応じて更新
	if (update_ui) {
		refresh_ui(false);
	}
}

void MainWindow::update_and_handle_pagination_3columns(PaginationAction action, bool update_ui) {
	// 引数に応じてページ送り操作を実行
	if (action == PaginationAction::GO_TO_PREV) {
		liststore3columnsAll.prev_page();
	} else if (action == PaginationAction::GO_TO_NEXT) {
		liststore3columnsAll.next_page();
	}
	// actionがUPDATE_UI_ONLYの場合は、ページ送りは行わない

	// モデルの現在の状態に基づいて、UIを更新
	pageLabel3columns.set_label(fmt::format("ページ {:2d} / {:2d}",
		liststore3columnsAll.get_current_page(),
		liststore3columnsAll.get_total_pages()
	).c_str());
	update_widget_state(&prevButton3columns, WidgetState{ .is_sensitive = liststore3columnsAll.has_prev_page() }, false);
	update_widget_state(&nextButton3columns, WidgetState{ .is_sensitive = liststore3columnsAll.has_next_page() }, false);

	if (update_ui) {
		refresh_ui(false);
	}
}

void MainWindow::update_and_handle_pagination_2columns(PaginationAction action, bool update_ui) {
	// 引数に応じてページ送り操作を実行
	if (action == PaginationAction::GO_TO_PREV) {
		liststore2columnsAll.prev_page();
	} else if (action == PaginationAction::GO_TO_NEXT) {
		liststore2columnsAll.next_page();
	}
	// actionがUPDATE_UI_ONLYの場合は、ページ送りは行わない

	// モデルの現在の状態に基づいて、UIを更新
	pageLabel2columns.set_label(fmt::format("ページ {:2d} / {:2d}",
		liststore2columnsAll.get_current_page(),
		liststore2columnsAll.get_total_pages()
	).c_str());
	update_widget_state(&prevButton2columns, WidgetState{ .is_sensitive = liststore2columnsAll.has_prev_page() }, false);
	update_widget_state(&nextButton2columns, WidgetState{ .is_sensitive = liststore2columnsAll.has_next_page() }, false);

	if (update_ui) {
		refresh_ui(false);
	}
}

void MainWindow::update_and_handle_pagination_2types1st(PaginationAction action, bool update_ui) {
	// 引数に応じてページ送り操作を実行
	if (action == PaginationAction::GO_TO_PREV) {
		paginatedListstore2types1st.prev_page();
	} else if (action == PaginationAction::GO_TO_NEXT) {
		paginatedListstore2types1st.next_page();
	}
	// actionがUPDATE_UI_ONLYの場合は、ページ送りは行わない

	// モデルの現在の状態に基づいて、UIを更新
	pageLabel2types1st.set_label(fmt::format("ページ {:2d} / {:2d}",
		paginatedListstore2types1st.get_current_page(),
		paginatedListstore2types1st.get_total_pages()
	).c_str());
	update_widget_state(&prevButton2types1st, WidgetState{ .is_sensitive = paginatedListstore2types1st.has_prev_page() }, false);
	update_widget_state(&nextButton2types1st, WidgetState{ .is_sensitive = paginatedListstore2types1st.has_next_page() }, false);

	if (update_ui) {
		refresh_ui(false);
	}
}

void MainWindow::update_and_handle_pagination_2types2nd(PaginationAction action, bool update_ui) {
	// 引数に応じてページ送り操作を実行
	if (action == PaginationAction::GO_TO_PREV) {
		paginatedListstore2types2nd.prev_page();
	} else if (action == PaginationAction::GO_TO_NEXT) {
		paginatedListstore2types2nd.next_page();
	}
	// actionがUPDATE_UI_ONLYの場合は、ページ送りは行わない

	// モデルの現在の状態に基づいて、UIを更新
	pageLabel2types2nd.set_label(fmt::format("ページ {:2d} / {:2d}",
		paginatedListstore2types2nd.get_current_page(),
		paginatedListstore2types2nd.get_total_pages()
	).c_str());
	update_widget_state(&prevButton2types2nd, WidgetState{ .is_sensitive = paginatedListstore2types2nd.has_prev_page() }, false);
	update_widget_state(&nextButton2types2nd, WidgetState{ .is_sensitive = paginatedListstore2types2nd.has_next_page() }, false);

	if (update_ui) {
		refresh_ui(false);
	}
}

bool MainWindow::authenticate_with_password() {
	// ダイアログ表示前にパスワード欄をクリア
	passwordEntry.set_text("");

	GtkResponseType response;

	// 既に認証済みならば、再度認証を求めない
	if (!m_password_authenticated) {
		// パスワード認証が成功するか、キャンセルされるまで繰り返す
		update_widget_state(&passwordEntryLabel, WidgetState{	// ラベルのテキストを更新
			.text = UiText{"パスワードを\n入力してください", "パスワードを\nにゅうりょく してください"}
		}, true);
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
					update_widget_state(&passwordEntryLabel, WidgetState{	// ラベルのテキストを更新
						.text = UiText{"パスワードが違います\nもう一度入力してください", "パスワードが ちがいます\nもういちど にゅうりょく してください"}
					}, true);
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

void MainWindow::set_tool_assignments(sgex::PaginatedListStoreModel<gui::MainWindow::MainTableData>& list_store_model, const std::vector<ToolAssignmentInfo> &tool_assignments)
{
	// ToolAssignmentInfo のリストを MainTableData のリストに変換
	std::vector<MainTableData> table_data_list;
	table_data_list.reserve(tool_assignments.size()); // 事前にメモリ確保

	// ツーリングリストのデータ構造(ToolingListInfo)をGUI表示用のデータ構造(MainTableData)に変換
	// 右側の値は、処理したい要素のリスト 左側の変数に順次代入される
	for (const auto &tool_assignment : tool_assignments)
	{
		table_data_list.push_back(MainTableData{
			// ToolAssignmentInfo と MstToolInfo から MainTableData に値をコピー
			.id = tool_assignment.id,	// 行の識別に使う (今回は tool_assignment_id を流用)
			.data_columns = {
				tool_assignment.tool_num,							// ツール番号
				tool_assignment.mst_tool_info.category,				// 分類
				tool_assignment.mst_tool_info.maker,				// メーカー
				tool_assignment.mst_tool_info.name,					// 型番
				tool_assignment.mst_tool_info.material,				// 材質
				std::to_string(tool_assignment.mst_tool_info.stc),	// 在庫数 (文字列に変換)
			},
			.cell_state_0to4 = ((tool_assignment.mst_tool_info.stc > 0)
				? (TableCellState::NORMAL)	// 在庫があれば通常状態
				: (TableCellState::FADED)	// 在庫が無ければ無効
			),
			// row.cell_state_5 はデフォルト(NONE)のまま
			.raw_data = tool_assignment // 生データを入れておく
		});	// リストに追加
	}

	// 変換したデータをページネーションモデルにセット (これで表示が更新される)
	list_store_model.set_master_data(std::move(table_data_list));
}

void MainWindow::update_retrieve_read_completed_buttons() {
	// 1. GUIの条件を判定
	bool gui_ready_for_action = !warningDialog.get_mapped() && !m_pending_tool_info.has_value();

	// 選択された中で最後の工具の時 (またはリストが空の時)
	bool is_last_tool = (!m_retrieving_index.has_value() || m_retrieving_index.value() + 1 >= m_retrieving_tools.size());

	// 2. 全システムの準備状態を判定
	bool all_systems_ready = gui_ready_for_action && m_plc_ready_for_action && m_tracker_ready_for_action;

	// 3. ボタンの状態を更新 (update_ui = false で即時描画はしない)

	// 終了ボタン: 全システムが準備OK かつ GUI条件(最後の工具) を満たす場合のみ有効
	update_widget_state(&falseButton, WidgetState{
		.is_sensitive = (all_systems_ready && is_last_tool)
	}, false);

	// 続ける/次へボタン: 全システムが準備OK の場合のみ有効
	update_widget_state(&trueButton, WidgetState{
		.is_sensitive = all_systems_ready
	}, false);

	// 4. 最後にUIを一括更新
	refresh_ui(false);
}

bool MainWindow::confirm_if_reground() {
	// 1. ダイアログの初期状態を設定（デフォルトは「いいえ」を選択）
	{
		utils::scoped_value_guard guard(m_is_programmatically_changing_toggle, true);
		notRegroundButton.set_active(false);
		regroundButton.set_active(true);
	}

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
		table2columns1st.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH); // テーブルのグリッド線を表示
		table2columns2nd.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH); // テーブルのグリッド線を表示
		table3columns1st.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH); // テーブルのグリッド線を表示
		table3columns2nd.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH); // テーブルのグリッド線を表示
		table3columns3rd.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH); // テーブルのグリッド線を表示

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
	if (m_is_programmatically_changing_toggle) {
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
	if (m_is_programmatically_changing_toggle) {
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
	if (m_is_programmatically_changing_toggle) {
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
		db::disconnect();
		m_password_authenticated = false;
		g_logged_in_user_info = nullopt;
		set_mode(GuiMode::NONE);
	} else {
		DEV_PRINT("ログアウトはキャンセルされました\n");
	}
}

void MainWindow::userIdEntry_on_activate() {
	DEV_PRINT("ユーザーIDエントリでEnterキーが押されました\n");

	if (!db::connect()) {
		// FIXME: エラー処理
	}

	if (db::user_authenticate(userIdEntry.get_text())) {
		DEV_PRINT("ユーザー認証成功\n");

		// ユーザーIDエントリにログインユーザー名を表示して固定
		update_widget_state(&userIdEntry, WidgetState{.can_focus = false, .is_editable = false}, false);

		reset_header_buttons(false);

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
		update_widget_state(&messageLabel, WidgetState{	// ラベルのテキストを更新
			.text = UiText{"該当するユーザーが見つかりません\nもう一度QRコードを読み取ってください", "がいとうする ユーザーが みつかりません\nもういちど QRコードを よみとってください"}
		}, true);
		userIdEntry.set_text("");
		userIdEntry.grab_focus();
	}
}

void MainWindow::mainTableSelection_on_changed(std::optional<size_t> selected_index_opt) {
	auto* func = new std::function<void()>([this, selected_index_opt]() {
		// このラムダ式はアイドル時に GUI スレッドで実行される

		try {
			auto state_config = g_app_state_configs.at(m_app_state);
			if (state_config.on_mainTableSelection_changed) {
				// 実際のモデル更新処理などを実行
				state_config.on_mainTableSelection_changed(this, selected_index_opt);
			}
		} catch (const std::out_of_range& e) {
			DEV_PRINT(stderr, "Error: 未定義の状態で mainTable の選択変更処理 (idle) が実行されました (AppState={})\n", static_cast<int>(m_app_state));
		} catch (...) {
			DEV_PRINT(stderr, "Error: Unexpected exception in delayed mainTableSelection_on_changed\n");
		}
	});

	g_idle_add(gui::MainWindow::dispatch_to_gui_thread, func);
}

void MainWindow::prevButton3columns_on_clicked() {
	DEV_PRINT("前のページへ\n");
	update_and_handle_pagination_3columns(PaginationAction::GO_TO_PREV, true);
}

void MainWindow::nextButton3columns_on_clicked() {
	DEV_PRINT("次のページへ\n");
	update_and_handle_pagination_3columns(PaginationAction::GO_TO_NEXT, true);
}

void MainWindow::prevButton2columns_on_clicked() {
	DEV_PRINT("前のページへ\n");
	update_and_handle_pagination_2columns(PaginationAction::GO_TO_PREV, true);
}

void MainWindow::nextButton2columns_on_clicked() {
	DEV_PRINT("次のページへ\n");
	update_and_handle_pagination_2columns(PaginationAction::GO_TO_NEXT, true);
}

void MainWindow::prevButton2types1st_on_clicked() {
	DEV_PRINT("前のページへ\n");
	update_and_handle_pagination_2types1st(PaginationAction::GO_TO_PREV, true);
}

void MainWindow::nextButton2types1st_on_clicked() {
	DEV_PRINT("次のページへ\n");
	update_and_handle_pagination_2types1st(PaginationAction::GO_TO_NEXT, true);
}

void MainWindow::prevButton2types2nd_on_clicked() {
	DEV_PRINT("前のページへ\n");
	update_and_handle_pagination_2types2nd(PaginationAction::GO_TO_PREV, true);
}

void MainWindow::nextButton2types2nd_on_clicked() {
	DEV_PRINT("次のページへ\n");
	update_and_handle_pagination_2types2nd(PaginationAction::GO_TO_NEXT, true);
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

void MainWindow::confirmInsertCountDialog_on_show() {
	// 初期値を表示
	insertCountEntry.set_text(std::to_string(m_insertCountEntry_current_count).c_str());
	refresh_ui(false);
}

void MainWindow::passwordEntryDialog_on_show() {
	// Gtkウィジェットにキーボードフォーカスを強制的に設定する
	passwordEntry.grab_focus();
}

void MainWindow::insertCountIncrementButton_on_clicked() {
	// 最大値 >= 0 のとき、最大値を超えないようにチェック
	if (m_confirmInsertCountDialog_max_count >= 0 && m_insertCountEntry_current_count < m_confirmInsertCountDialog_max_count) {
		insertCountEntry.set_text(std::to_string(++m_insertCountEntry_current_count).c_str());
	}
}

void MainWindow::insertCountDecrementButton_on_clicked() {
	// 0以下にならないようにチェック
	if (m_insertCountEntry_current_count > 1) {
		insertCountEntry.set_text(std::to_string(--m_insertCountEntry_current_count).c_str());
	}
}

void MainWindow::insertCountDetectButton_on_clicked() {
	update_widget_state(&messageDialogLabel, WidgetState{
		.text = UiText{"検出中です\nしばらくお待ちください。", "けんしゅつちゅうです\nしばらく おまちください"}
	}, true);
	messageDialog.set_transient_for(confirmInsertCountDialog);
	messageDialog.show_all();

	auto* func = new std::function<void()>([this]() {
		// このラムダ式はアイドル時に GUI スレッドで実行される
		if (auto count_opt = tracker::get_insert_count()) {
			insertCountEntry.set_text(std::to_string(m_insertCountEntry_current_count = count_opt.value()).c_str());
			update_widget_state(&confirmInsertCountLabel, WidgetState{
				.text = UiText{"検出に成功しました\n個数を確認して下さい", "けんしゅつに せいこうしました\nこすうを かくにん してください"}
			}, true);
		} else {
			update_widget_state(&confirmInsertCountLabel, WidgetState{
				.text = UiText{"検出に失敗しました\n手動で入力して下さい", "けんしゅつに しっぱいしました\nしゅどうで にゅうりょく してください"}
			}, true);
		}
		messageDialog.hide();
		messageDialog.set_transient_for(*this);
	});

	g_idle_add(gui::MainWindow::dispatch_to_gui_thread, func);
}

void MainWindow::insertCountOkButton_on_clicked() {
	m_confirmInsertCountDialog_max_count = -1;
	insertCountEntry.set_text("");
	confirmInsertCountDialog.response(GTK_RESPONSE_OK);
}

void MainWindow::regroundButton_on_toggled() {
	if (m_is_programmatically_changing_toggle) { return; }	// プログラムによる変更は無視

	if (regroundButton.get_active()) {
		utils::scoped_value_guard guard(m_is_programmatically_changing_toggle, true);
		notRegroundButton.set_active(false);	// 「いいえ」をオフにする
	}
}

void MainWindow::notRegroundButton_on_toggled() {
	if (m_is_programmatically_changing_toggle) { return; }	// プログラムによる変更は無視

	if (notRegroundButton.get_active()) {
		utils::scoped_value_guard guard(m_is_programmatically_changing_toggle, true);
		regroundButton.set_active(false); // 「はい」をオフにする
	}
}

//=========================================================================
// プライベートメンバ関数 (状態毎の関数) 実装
//=========================================================================

void MainWindow::on_enter_waiting_for_user_login() {
	userIdEntry.set_text("");
	userIdEntry.grab_focus();
}

// --- 取り出し ---

void MainWindow::on_enter_retrieve_waiting_for_program_qr() {
	// 変数を初期化
	m_retrieving_list.reset();
	m_retrieve_backup_table.clear();
	m_retrieving_tools.clear();
	m_retrieving_index.reset();

	// UIを初期化
	liststore2columnsAll.clear();	// テーブルを空にする
	lineEntry.set_text("");			// ライン名を空にする
	programEntry.set_text("");		// プログラム番号を空にする
	qrCodeEntry.set_text("");		// QRコード入力欄を空にする
	qrCodeEntry.grab_focus();		// QRコード入力欄にフォーカスを設定
}

void MainWindow::on_qrCodeEntry_retrieve_waiting_for_program_qr() {
	// QRコードからプログラム割当一覧を取得
	m_retrieving_list = db::get_ToolingListInfo(qrCodeEntry.get_text());

	// 取得成功時の処理(ToolingListInfoが存在する場合)
	if (m_retrieving_list.has_value()) {
		// 1. m_retrieving_list.value()：オプションから実際のToolingListInfoを取得
		ToolingListInfo& tooling_list = m_retrieving_list.value();

		// 2. ライン名とプログラム番号を表示
		lineEntry.set_text(tooling_list.line.name.c_str());
		programEntry.set_text(tooling_list.program.name.c_str());

		// ToolAssignmentInfo のリストを MainTableData のリストに変換して、
		// ページネーションモデルにセット (これで表示が更新される)
		set_tool_assignments(liststore2columnsAll, tooling_list.tool_assignments);

		// 5. 次の状態へ遷移
		set_app_state(AppState::RETRIEVE_SELECTING_TOOLS);

	// 取得失敗時の処理
	} else {
		// 取得失敗時の処理 (エラーメッセージ表示など)
		DEV_PRINT("ツーリングリストの取得に失敗しました。\n");
		update_widget_state(&messageLabel, WidgetState{	// ラベルのテキストを更新
			.text = UiText{"有効なプログラム割当一覧表QRコードではありません。", "ゆうこうな プログラムわりあていちらんひょう QRコードではありません。"}
		}, true);
	}
}

void MainWindow::on_enter_retrieve_selecting_tools()
{
	update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);
}

void MainWindow::on_mainTableSelection_retrieve_selecting_tools(std::optional<size_t> selected_index_opt) {
	// 選択されていて、かつ無効化されていない行の場合
	if (selected_index_opt.has_value()) {
		auto selected_index = selected_index_opt.value();
		auto selected_item = liststore2columnsAll.get_item(selected_index);

		DEV_PRINT("mainTableSelection_on_changed: Line {} is selected\n", selected_index);

		if (selected_item.cell_state_0to4 != TableCellState::FADED) {
			// 状態をトグルする
			selected_item.cell_state_0to4 = ((selected_item.cell_state_0to4 == TableCellState::SELECTED)
				? (TableCellState::NORMAL)
				: (TableCellState::SELECTED)
			);
		}

		// モデルを更新 (これにより GtkListStore も更新される)
		liststore2columnsAll.update(selected_index, selected_item);
	}

	update_widget_state(&trueButton, WidgetState{.is_sensitive = (liststore2columnsAll.find_first_index([](const MainTableData& selected_item) {
		return selected_item.cell_state_0to4 == TableCellState::SELECTED;
	}).has_value())}, true);
}

void MainWindow::on_falseButton_retrieve_selecting_tools() {
	liststore2columnsAll.modify_master_data(
		// ラムダ式にマスターデータへの参照(master_data)が渡される
		[](std::vector<MainTableData>& master_data) {
			// マスターデータを直接ループ処理
			for (auto& row : master_data) {
				// 条件に一致する行を見つけたら
				if (row.cell_state_0to4 == TableCellState::SELECTED) {
					// その行の状態を直接変更する
					row.cell_state_0to4 = TableCellState::NORMAL;
				}
			}
			// ラムダ式が終了すると、modify_master_data が display_page を呼び出し、変更がUIに反映される
		},
		true // true を指定して、現在のページを再表示
	);
}

void MainWindow::on_enter_retrieve_confirming_selection() {
	// 選択状態の行を抽出
	auto vec = liststore2columnsAll.find_items([](const MainTableData& row) {
		return row.cell_state_0to4 == TableCellState::SELECTED;
	});

	// 全要素の色を通常色に戻す
	for (auto& row : vec) {
		row.cell_state_0to4 = TableCellState::NORMAL;
	}

	// 表のデータを抽出したものに置き換えつつ、現在のデータを退避
	m_retrieve_backup_table = liststore2columnsAll.replace_master_data(std::move(vec));
}

void MainWindow::on_falseButton_retrieve_confirming_selection() {
	liststore2columnsAll.set_master_data(std::move(m_retrieve_backup_table));	// 退避していた表データを戻す
	set_app_state(AppState::RETRIEVE_SELECTING_TOOLS);
}

void MainWindow::on_trueButton_retrieve_confirming_selection() {
	m_retrieve_backup_table.clear();	// 退避していた表のバックアップをクリア
	m_retrieving_tools.clear();			// 取り出し中工具の vector をクリア
	m_retrieving_tools.reserve(liststore2columnsAll.size());	// 先にメモリを確保しておく

	// 取り出し中工具のリストに、確定した取り出す工具を格納しておく
	for (const auto& item : liststore2columnsAll.get_master_data()) {
		m_retrieving_tools.push_back(std::any_cast<const ToolAssignmentInfo&>(item.raw_data));	// FIXME: any型不一致対応
	}
	m_retrieving_index = 0;	// どの工具の取り出しを行っているかのインデックスを初期化
	set_app_state(AppState::RETRIEVE_CONTAINER_RETRIEVING);	// 画面遷移
}

void MainWindow::on_enter_retrieve_container_retrieving() {
	auto mst_tool_info = m_retrieving_tools.at(m_retrieving_index.value()).mst_tool_info;	// FIXME: nullopt対応
	auto area_info_opt = db::get_StrageAreaInfo_with_max_cases(mst_tool_info.id);
	m_operating_area = area_info_opt.value();	// FIXME: nullopt対応
	plc::retrieve_container(std::stoi(m_operating_area.mst_address_info.control_address));	// FIXME: 例外処理
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_RETRIEVE_COMPLETED,
		.flags = plc::WatchFlags{
			.enable_rise_notification = true,
			.rise_is_one_shot = true
		}
	});

	// FIXME: エラー対応
	tracker::g_tool_tracker->connect();
	tracker::g_recorder_client->start_recording();

	// インサートの操作か判定
	m_is_insert_operation = (mst_tool_info.category == TOOL_CATEGORY_INSERT);

	std::vector<MainTableData> vec;
	vec.reserve(m_retrieving_tools.size());
	for (size_t i = 0; i < m_retrieving_tools.size(); i++) {
		// 現在の要素への参照を取得
		const auto& retrieving_tool = m_retrieving_tools[i];

		vec.push_back(MainTableData{
			.id = retrieving_tool.id,
			.data_columns = {
				retrieving_tool.tool_num,							// ツール番号
				retrieving_tool.mst_tool_info.category,				// 分類
				retrieving_tool.mst_tool_info.maker,				// メーカー
				retrieving_tool.mst_tool_info.name,					// 型番
				retrieving_tool.mst_tool_info.material,				// 材質
				std::to_string(retrieving_tool.mst_tool_info.stc),	// 在庫数 (文字列に変換)
			},
			.cell_state_0to4 = ((i == m_retrieving_index)
				? (TableCellState::HIGHLIGHT)
				: (TableCellState::FADED)
			),
			.raw_data = retrieving_tool
		});
	}
	liststore2columnsAll.set_master_data(std::move(vec));
}

void MainWindow::on_plc_retrieve_container_retrieving(const plc::Notification& notification) {
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {	// BUG: nullopt対応
		auto result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_RETRIEVE_COMPLETED && result.status) {
			set_app_state(AppState::RETRIEVE_CONTAINER_RETRIEVED);
		} else {}	// BUG: エラー処理
	}
}

void MainWindow::on_enter_retrieve_container_retrieved() {
	// TODO: 扉を開けるまで待機する処理を実装
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_DOOR_OPEND,
		.flags = plc::WatchFlags{
			.enable_rise_notification = true,
			.rise_is_one_shot = true
		}
	});
}

void MainWindow::on_plc_retrieve_container_retrieved(const plc::Notification& notification) {
	// TODO: 扉を開けるまで待機する処理を実装
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {	// BUG: nullopt対応
		auto result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_DOOR_OPEND && result.status) {
			set_app_state(AppState::RETRIEVE_READY_FOR_READING);
		} else {}	// BUG: エラー処理
	}
}

void MainWindow::on_enter_retrieve_ready_for_reading() {
	// 状態変数を初期化
	m_plc_ready_for_action = false;		// 扉が閉まるのを待つため、最初は false
	m_tracker_ready_for_action = true;	// 最初は不正なし

	liststore2columnsAll.clear();
	qrCodeEntry.set_text("");
	qrCodeEntry.grab_focus();
}

void MainWindow::on_qrCodeEntry_retrieve_ready_for_reading() {
	MstToolInfo retrieving_tool = m_retrieving_tools.at(m_retrieving_index.value()).mst_tool_info;	// BUG: nullopt対応
	std::optional<UniqueToolInfo> unique_info_opt = db::get_UniqueToolInfo(qrCodeEntry.get_text());

	if (unique_info_opt.has_value()) {
		UniqueToolInfo& unique_info = unique_info_opt.value();
		if (unique_info.mst_tool_info.id != retrieving_tool.id && unique_info.storage_area_info.has_value() && unique_info.storage_area_info.value().id != m_operating_area.id) {
			update_widget_state({&messageLabel, &warningLabel}, WidgetState{	// ラベルのテキストを更新
				.text = UiText{"工具が違います\n別の工具を読み取ってください", "こうぐが ちがいます\nべつの こうぐを よみとってください"}
			}, true);
		}

		tracker::g_tool_tracker->notify_qr_scan();
		m_pending_tool_info = std::move(unique_info);
	} else {
		update_widget_state({&messageLabel, &warningLabel}, WidgetState{	// ラベルのテキストを更新
			.text = UiText{"該当する工具データが見つかりません\nもう一度QRコードを読み取ってください", "がいとうする こうぐデータが みつかりません\nもういちど QRコードを よみとってください"}
		}, true);
	}
	qrCodeEntry.grab_focus();
}

void MainWindow::on_tracker_retrieve_ready_for_reading(const tracker::Notification& notification) {
	bool result = false;
	MstToolInfo retrieving_tool = m_retrieving_tools.at(m_retrieving_index.value()).mst_tool_info;	// BUG: nullopt対応

	switch (notification.type) {
	case tracker::NotificationType::SCAN_SUCCESS: {
		if (m_pending_tool_info.has_value()) {
			// 念のため基本工具情報が合致しているかチェック
			UniqueToolInfo unique_info = std::move(m_pending_tool_info).value();
			m_pending_tool_info.reset();
			if (unique_info.mst_tool_info.id != retrieving_tool.id && unique_info.storage_area_info.has_value() && unique_info.storage_area_info.value().id != m_operating_area.id) {
				DEV_PRINT(stderr, "on_tracker_retrieve_read_completed: m_pending_tool_info が不正値です\n");
				// BUG: 不正値対応
//				update_widget_state({&messageLabel, &warningLabel}, WidgetState{	// ラベルのテキストを更新
//					.text = UiText{"工具が違います\n別の工具を読み取ってください", "こうぐが ちがいます\nべつの こうぐを よみとってください"}
//				}, true);
				result = false;
			} else {
				// 警告ダイアログが表示されていれば非表示に
				if (warningDialog.get_mapped()) {
					warningDialog.hide();
				}

				// この工具が既に取り出しリスト (m_operated_records) に存在するか検索
				auto it = std::find_if(m_operated_records.begin(), m_operated_records.end(),
					[&](const RecOperationTable& rec) {
						return rec.unique_tool_id.has_value() && rec.unique_tool_id.value() == unique_info.id;
					}
				);

				// 存在する場合 (重複スキャン)
				if (it != m_operated_records.end()) {
					if (m_is_insert_operation) {
						// 2a. インサートの場合: 個数確認ダイアログを再表示
						DEV_PRINT("重複スキャン (インサート): ID {} の個数を更新します。\n", unique_info.id);
						update_widget_state(&confirmInsertCountLabel, WidgetState{.text = UiText{
							"取り出す枚数を変更します\n何枚取り出すか入力してください",
							"とりだす まいすうを へんこうします\nなんまい とりだすか にゅうりょくしてください"
						}}, true);

						// ダイアログの初期値を、前回入力した個数に設定
						m_insertCountEntry_current_count = it->tool_num.value_or(1);
						m_confirmInsertCountDialog_max_count = unique_info.case_pack_num;
						confirmInsertCountDialog.run(this); // m_insertCountEntry_current_count に新しい値が入る

						// 履歴ベクトル(m_operated_records)の個数を更新
						it->tool_num = m_insertCountEntry_current_count;
						it->proc_time = sg::now(); // 処理時刻を更新

						// 表(liststore2columnsAll)の個数を更新
						// まず、表から該当する行のインデックスを検索
						auto table_row_opt = liststore2columnsAll.find_first_index_and_item(
							[&](const MainTableData& item){ return item.id == unique_info.id; }
						);

						if (table_row_opt.has_value()) {
							auto [index, item] = table_row_opt.value();
							item.data_columns[5] = std::to_string(m_insertCountEntry_current_count); // 6列目(個数)を更新
							liststore2columnsAll.update(index, item); // 表の行を更新
						} else {
							// 基本的にここには来ないはず (m_operated_records と liststore は同期しているため)
							// BUG: 不正値対応
							DEV_PRINT(stderr, "CRITICAL: m_operated_records に ID {} がありましたが、liststore にはありませんでした。\n", unique_info.id);
						}
					} else {
						// 2b. インサートでない場合: 何もしない
						DEV_PRINT("重複スキャン (非インサート): ID {} を無視します。\n", unique_info.id);
					}
				} else {
					if (m_is_insert_operation) {
						update_widget_state(&confirmInsertCountLabel, WidgetState{.text = UiText{
							"使用するインサートの枚数を\n入力してください",
							"しようする インサートの まいすうを\nにゅうりょく してください"
						}}, true);
						m_insertCountEntry_current_count = 1;
						m_confirmInsertCountDialog_max_count = unique_info.case_pack_num;
						confirmInsertCountDialog.run(this);
					}

					liststore2columnsAll.append(MainTableData{
						.id = unique_info.id,
						.data_columns = {
							fmt::format("{:010d}", unique_info.id),	// 10桁0埋め
							unique_info.mst_tool_info.category,
							unique_info.mst_tool_info.maker,
							unique_info.mst_tool_info.name,
							unique_info.mst_tool_info.material,
							to_string(((m_is_insert_operation)
								? (m_insertCountEntry_current_count)
								: (unique_info.regrind_count.value_or(0))	// FIXME: nullopt対応
							))
						},
						.raw_data = unique_info
					});

					m_operated_records.push_back(RecOperationTable{
						.operation_class	= OperationClass::OUT,
						.user_id			= ((g_logged_in_user_info.has_value()) ? (g_logged_in_user_info.value().id) : (0)),	// FIXME: nullopt対応
						.proc_time			= sg::now(),
						.storage_area_id	= unique_info.storage_area_info.value().id,
						.unique_tool_id		= unique_info.id,
						.video_path			= tracker::g_recorder_client->get_current_filename(),
						.tool_num			= ((m_is_insert_operation)
							? (m_insertCountEntry_current_count)
							: (1)
						)
					});

					update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);
				}
				result = true;
			}
		} else {
			DEV_PRINT(stderr, "on_tracker_retrieve_read_completed: m_pending_tool_info が nullopt の状況でスキャン成功通知を受け取りました\n");
			result = false;
			// FIXME: nullopt対応
		}
		break;
	}
	case tracker::NotificationType::SCAN_FAIL: {
		update_widget_state({&messageLabel, &warningLabel}, WidgetState{.text = UiText{
			"作業台に埋め込まれた\nQRコードリーダーで読み取ってください",
			"さぎょうだいに うめこまれた\nQRコードリーダーで よみとってください"
		}}, true);
		result = false;
		break;
	}
	case tracker::NotificationType::ANOMALY_UNSCANNED_OUT: {
		update_widget_state(&warningLabel, WidgetState{.text = UiText{
			"QRコードリーダーで読み取らずに工具を取り出そうとしています\nQRコードリーダーで読み取ってから、工具を取り出してください",
			"QRコードリーダーで よみとらずに こうぐを とりだそうと しています\nQRコードリーダーで よみとってから、こうぐを とりだしてください"
		}}, true);
		warningDialog.show_all();
		result = false;
		break;
	}
	// TODO: 不正対策
	default: {
		break;
	}
	}

	m_tracker_ready_for_action = result;
	update_retrieve_read_completed_buttons();

	if (result) {
		set_app_state(AppState::RETRIEVE_READ_COMPLETED);
	}
	qrCodeEntry.grab_focus();
}

void MainWindow::on_enter_retrieve_read_completed() {
	// TODO: 扉を閉めるまで待機する処理を実装
#ifdef STUB_PLC
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_DOOR_OPEND,
		.flags = plc::WatchFlags{
			.enable_fall_notification = true,
			.fall_is_one_shot = true,
		}
	});
#else
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_DOOR_OPEND,
		.flags = plc::WatchFlags{
			.enable_fall_notification = true,
			.enable_rise_notification = true,
			.fall_is_one_shot = false,
			.rise_is_one_shot = false
		}
	});
#endif
}

void MainWindow::on_falseButton_retrieve_read_completed() {
	// BUG: エラー対応
	plc::store_container();
	// TODO: 扉ロック
	plc::g_command_queue.push(plc::WatchCommand{.addr = plc::M_DEVICE_DOOR_OPEND, .flags = plc::WatchFlags{}});	// 監視対象を削除
	db::save_history(m_operated_records);
	db::disconnect();
	tracker::g_tool_tracker->disconnect();
	tracker::g_recorder_client->stop_recording();
	m_operated_records.clear();
	m_retrieve_backup_table.clear();
	m_retrieving_tools.clear();
	m_retrieving_index.reset();
	lineEntry.set_text("");		// ライン名を空にする
	programEntry.set_text("");	// プログラム番号を空にする
	set_mode(GuiMode::NONE);
}

void MainWindow::on_trueButton_retrieve_read_completed() {
	// BUG: エラー対応
	plc::store_container();
	// TODO: 扉ロック
	plc::g_command_queue.push(plc::WatchCommand{.addr = plc::M_DEVICE_DOOR_OPEND, .flags = plc::WatchFlags{}});	// 監視対象を削除
	db::save_history(m_operated_records);
	tracker::g_tool_tracker->disconnect();
	tracker::g_recorder_client->stop_recording();
	m_operated_records.clear();
	if (m_retrieving_index.has_value() && m_retrieving_index.value() + 1 < m_retrieving_tools.size()) {
		// 次の工具の搬出処理へ
		m_retrieving_index.value()++;
		set_app_state(AppState::RETRIEVE_CONTAINER_RETRIEVING);
	} else {
		// 選択された中で最後の工具の時
		m_retrieving_index.reset();

		// m_retrieving_list に保存しておいた、現在取り出し中のツーリングリストを更新
		if (m_retrieving_list.has_value() && db::update_ToolingListInfo(m_retrieving_list.value())) {
			// 更新に成功したら、表にセットして工具選択画面に遷移
			set_tool_assignments(liststore2columnsAll, m_retrieving_list.value().tool_assignments);
			set_app_state(AppState::RETRIEVE_SELECTING_TOOLS);
		} else {
			// 更新に失敗したら、プログラム割当一覧表読み取り画面に遷移
			set_app_state(AppState::RETRIEVE_WAITING_FOR_PROGRAM_QR);
		}
	}
}

void MainWindow::on_plc_retrieve_read_completed(const plc::Notification& notification) {
	// TODO: 扉を閉めるまで待機する処理を実装
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {	// BUG: nullopt対応
		auto result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_DOOR_OPEND) {
			// 1. PLCの状態変数を更新 (status=true が「開」、false が「閉」と仮定)
			m_plc_ready_for_action = !result.status; // 扉が閉まっている(!status)時に true

			// 2. 共通ヘルパーを呼び出して、3つの条件に基づきUIを更新
			update_retrieve_read_completed_buttons();
		} else {}	// BUG: エラー処理
	}
}

// --- 補充 ---

void MainWindow::on_enter_store_waiting_for_tool_qr() {
	storingToolListstore.clear();
	storingToolListstore.append(MstToolInfo{});
	qrCodeEntry.set_text("");
	qrCodeEntry.grab_focus();
}

void MainWindow::on_qrCodeEntry_store_waiting_for_tool_qr() {
	auto unique_info = db::get_UniqueToolInfo(qrCodeEntry.get_text());

	if (unique_info.has_value()) {
		auto mst_tool_info = unique_info.value().mst_tool_info;
		storingToolListstore.update(0, mst_tool_info);

		// インサートの操作か判定
		m_is_insert_operation = (mst_tool_info.category == TOOL_CATEGORY_INSERT);

		set_app_state(AppState::STORE_CONFIRMING_SELECTION);
	} else {
		// TODO: 該当データが無い場合の処理
		update_widget_state(&messageLabel, WidgetState{	// ラベルのテキストを更新
			.text = UiText{"該当する工具データが見つかりません\nもう一度QRコードを読み取ってください", "がいとうする こうぐデータが みつかりません\nもういちど QRコードを よみとってください"}
		}, true);
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

void MainWindow::on_plc_store_container_retrieving(const plc::Notification& notification) {
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {	// BUG: nullopt対応
		auto result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_RETRIEVE_COMPLETED && result.status) {
			set_app_state(AppState::STORE_CONTAINER_RETRIEVED);
		} else {}	// BUG: エラー処理
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

void MainWindow::on_plc_store_container_retrieved(const plc::Notification& notification) {
	// TODO: 扉を開けるまで待機する処理を実装
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {	// BUG: nullopt対応
		auto result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_DOOR_OPEND && result.status) {
			set_app_state(AppState::STORE_READY_FOR_READING);
		} else {}	// BUG: エラー処理
	}
}

void MainWindow::on_enter_store_ready_for_reading() {
	liststore2columnsAll.clear();
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
	std::optional<UniqueToolInfo> unique_info_opt = db::get_UniqueToolInfo(qrCodeEntry.get_text());

	if (unique_info_opt.has_value()) {
		UniqueToolInfo& unique_info = unique_info_opt.value();
		if (!m_is_insert_operation && unique_info.storage_condition == StorageCondition::TAKEN_OUT) {
			if ((is_reground = confirm_if_reground())) {
				// 再研磨操作の場合、再研磨回数を1増やす
				unique_info.regrind_count = (unique_info.regrind_count.value_or(0) + 1);
			}
		}

		liststore2columnsAll.append(MainTableData{
			.id = unique_info.id,
			.data_columns = {
				to_string(unique_info.id),
				unique_info.mst_tool_info.category,
				unique_info.mst_tool_info.maker,
				unique_info.mst_tool_info.name,
				unique_info.mst_tool_info.material,
				to_string(((m_is_insert_operation)
					? (unique_info.case_pack_num)
					: (unique_info.regrind_count)
				).value())
			},
			.raw_data = unique_info
		});
		update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);
		result = true;
	} else {
		// TODO: 該当データが無い場合の処理
		update_widget_state(&messageLabel, WidgetState{	// ラベルのテキストを更新
			.text = UiText{"該当する工具データが見つかりません\nもう一度QRコードを読み取ってください", "がいとうする こうぐデータが みつかりません\nもういちど QRコードを よみとってください"}
		}, true);
		result = false;
	}
	qrCodeEntry.grab_focus();
	return result;
}

void MainWindow::on_falseButton_store_read_completed() {
	save_store_history();
	set_mode(GuiMode::NONE);
}

void MainWindow::on_trueButton_store_read_completed() {
	save_store_history();
	set_app_state(AppState::STORE);
}

void MainWindow::on_plc_store_read_completed(const plc::Notification& notification) {
	// TODO: 扉を閉めるまで待機する処理を実装
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {	// BUG: nullopt対応
		auto result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_DOOR_OPEND) {
			update_widget_state({&falseButton, &trueButton}, WidgetState{.is_sensitive = !result.status}, true);
		} else {}	// BUG: エラー処理
	}
}

// --- 棚卸し ---

void MainWindow::on_enter_reconcile_waiting_for_container_selection() {
	// 1. 絞り込み条件（メンバー変数）を使用してDBからコンテナリストを取得
	// ※ 注: 絞り込み条件を保持するため、MainWindowクラス(tool_io_gui.hpp)に
	//      以下のメンバー変数を追加で定義する必要があります。
	//
	// std::optional<std::string> m_reconcile_filter_category;
	// std::optional<std::chrono::system_clock::time_point> m_reconcile_filter_start_date;
	// std::optional<std::chrono::system_clock::time_point> m_reconcile_filter_end_date;
	//
	// ※ 注: db::get_reconcile_container_list は db_client.cpp に新規実装が必要です。
	// auto container_list_opt = db::get_reconcile_container_list(
	// 	m_reconcile_filter_category,
	// 	m_reconcile_filter_start_date,
	// 	m_reconcile_filter_end_date
	// );

	// ---【以下、DB関数が実装されるまでのダミーデータ】---
	std::vector<ContainerReconcileInfo> dummy_list;
	dummy_list.push_back({
		.area_info = StrageAreaInfo {
			.id = 1,
			.item = MstToolInfo { .category = "ドリル", .maker = "OSG", .name = "ADF-2D-9", .material = "超硬" },
			.mst_address_info = MstAddressInfo { .display_address = "1-1-1" }
		},
		.last_reconcile_time = sg::make_time_point(2025, 10, 1) // 2025/10/01
	});
	dummy_list.push_back({
		.area_info = StrageAreaInfo {
			.id = 2,
			.item = MstToolInfo { .category = "インサート", .maker = "タンガロイ", .name = "TPMT16T308-CM", .material = "T515" },
			.mst_address_info = MstAddressInfo { .display_address = "1-1-2" }
		},
		.last_reconcile_time = std::nullopt // 棚卸し履歴なし
	});
	// (絞り込み条件を適用するロジックをダミーで追加)
	// if (m_reconcile_filter_category.has_value() && m_reconcile_filter_category.value() == "ドリル") {
	//     dummy_list.pop_back(); // インサートを削除
	// }
	auto container_list_opt = std::make_optional(std::move(dummy_list));
	// ---【ダミーデータここまで】---

	// 2. DBからの取得結果を処理
	if (!container_list_opt.has_value()) {
		// 取得失敗
		update_widget_state(&messageLabel, WidgetState{.text = UiText{"コンテナリストの取得に失敗しました", "コンテナリストの しゅとくに しっぱいしました"}}, true);
		liststore2columnsAll.clear(); // テーブルを空にする
		return;
	}

	// 3. DBデータをGUI用テーブルデータ (MainTableData) に変換
	std::vector<MainTableData> table_data_list;
	table_data_list.reserve(container_list_opt.value().size());

	for (const auto& row : container_list_opt.value()) {
		std::string last_time_str = "----/--/--";
		if (row.last_reconcile_time.has_value()) {
			// time_point を "YYYY/MM/DD" 形式の文字列にフォーマット
			last_time_str = fmt::format("{:%Y/%m/%d}", row.last_reconcile_time.value());
		}

		table_data_list.push_back(MainTableData{
			.id = row.area_info.id, // 識別用に StrageAreaInfo の ID を使う
			.data_columns = {
				row.area_info.mst_address_info.display_address, // [0] コンテナ番号
				row.area_info.item.category,                    // [1] 分類
				row.area_info.item.maker,                       // [2] メーカー
				row.area_info.item.name,                        // [3] 型番
				row.area_info.item.material,                    // [4] 材質
				last_time_str                                   // [5] 最終棚卸し日時
			},
			.cell_state_0to4 = TableCellState::NORMAL,  // 状態はすべて「通常」
			// .cell_state_5 = TableCellState::NORMAL,     // 状態はすべて「通常」（無くても動く）
			.raw_data = row.area_info                   // 選択時に使うため、生の StrageAreaInfo を保持
		});
	}

	// 4. 変換したデータを PaginatedListStoreModel にセット
	// (set_master_data は自動的に display_page(0) を呼び出し、UIを更新します)
	liststore2columnsAll.set_master_data(std::move(table_data_list));

	// 5. ページネーションUI（ボタンの有効/無効など）を更新
	update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);
}

void MainWindow::on_mainTableSelection_reconcile_waiting_for_container_selection(std::optional<size_t> selected_index_opt) {

}

void MainWindow::on_enter_reconcile_container_retrieving() {

}

void MainWindow::on_plc_reconcile_container_retrieving(const plc::Notification& notification) {

}

void MainWindow::on_enter_reconcile_container_retrieved() {

}

void MainWindow::on_plc_reconcile_container_retrieved(const plc::Notification& notification) {

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
	set_app_state(AppState::RECONCILE);
}

} // namespace gui
