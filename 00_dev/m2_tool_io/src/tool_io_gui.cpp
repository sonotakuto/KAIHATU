/**
 * @file tool_io_gui.cpp
 * @brief tool_io_gui.hppで宣言されたクラスのメンバ関数の実装ファイル。
 */
#include "./opt_extension.hpp"
#include "./utility.hpp"
#include "./container_utils.hpp"
#include "./SimpleGtk_extension.hpp"
#include "./includes.hpp"
#include "./db_client.hpp"
#include "./tracking_communicator.hpp"
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

const std::map<MainWindow::WarningType, MainWindow::UiText> MainWindow::g_warning_type_messages = {
	{WarningType::TOOL_NOT_FOUND, UiText{
		"該当する工具データが見つかりません\nもう一度QRコードを読み取って下さい",
		"がいとうする こうぐデータが みつかりません\nもういちど QRコードを よみとってください"
	}},
	{WarningType::DIFFERENT_TOOL, UiText{
		"工具が違います\n別の工具を読み取って下さい",
		"こうぐが ちがいます\nべつのこうぐを よみとってください"
	}},
	{WarningType::NOT_YET_STORED, UiText{
		"この工具は『補充待ち』になっています\n別の工具を読み取って下さい",
		"このこうぐは『ほじゅうまち』に なっています\nべつのこうぐを よみとってください"
	}},
	{WarningType::ALREADY_RETRIEVED, UiText{
		"この工具は既に『取り出し済み』になっています\n別の工具を読み取って下さい",
		"このこうぐは すでに『とりだしずみ』に なっています\nべつのこうぐを よみとってください"
	}},
	{WarningType::SCAN_FAILURE, UiText{
		"作業台に埋め込まれた\nQRコードリーダーで読み取って下さい",
		"さぎょうだいに うめこまれた\nQRコードリーダーで よみとってください"
	}},
	{WarningType::UNSCANNED_OUT_FRAME, UiText{
		"QRコードリーダーで読み取らずに工具を取り出そうとしています\nQRコードリーダーで読み取ってから、工具を取り出して下さい",
		"QRコードリーダーで よみとらずに こうぐを とりだそうと しています\nQRコードリーダーで よみとってから、こうぐを とりだしてください"
	}},
	{WarningType::SCANNED_IN_CONTAINER, UiText{
		"スキャン済みの工具を\nコンテナに戻さないで下さい",
		"スキャンずみの こうぐを\nコンテナに もどさないでください"
	}},
	{WarningType::OTHER_IN_CONTAINER, UiText{
		"コンテナの外にあった工具を\nコンテナに入れないで下さい",
		"コンテナのそとに あった こうぐを\nコンテナに いれないでください"
	}}
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
			lhs.can_focus == rhs.can_focus &&
			lhs.is_sensitive == rhs.is_sensitive &&
			lhs.is_visible == rhs.is_visible &&
			lhs.is_editable == rhs.is_editable;
}

bool operator!=(const MainWindow::WidgetState& lhs, const MainWindow::WidgetState& rhs) {
	return !(lhs == rhs);
}

// --- TableState ---

bool operator==(const MainWindow::TableState& lhs, const MainWindow::TableState& rhs) {
	// 全てのoptionalメンバーが一致する場合にtrueを返す
	return lhs.column_headers == rhs.column_headers &&
			lhs.is_sensitive == rhs.is_sensitive &&
			lhs.is_visible == rhs.is_visible &&
			lhs.is_selectable == rhs.is_selectable;
}

bool operator!=(const MainWindow::TableState& lhs, const MainWindow::TableState& rhs) {
	return !(lhs == rhs);
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
	storingToolTableBox(glade, "storingToolTableBox"),
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
	liststore3columnsAll(DISPLAY_ROWS_PER_COLUMN, NUM_OF_VIEW_COLUMNS, {.can_focus = false, .grid_lines = GTK_TREE_VIEW_GRID_LINES_BOTH, .selection_mode = GTK_SELECTION_NONE}),
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
	liststore2columnsAll(DISPLAY_ROWS_PER_COLUMN, NUM_OF_VIEW_COLUMNS, {.can_focus = false, .grid_lines = GTK_TREE_VIEW_GRID_LINES_BOTH, .selection_mode = GTK_SELECTION_NONE}),
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
	paginatedListstore2types1st(DISPLAY_ROWS_PER_COLUMN, NUM_OF_VIEW_COLUMNS, {.can_focus = false, .grid_lines = GTK_TREE_VIEW_GRID_LINES_BOTH, .selection_mode = GTK_SELECTION_NONE}),
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
	paginatedListstore2types2nd(DISPLAY_ROWS_PER_COLUMN, NUM_OF_VIEW_COLUMNS, {.can_focus = false, .grid_lines = GTK_TREE_VIEW_GRID_LINES_BOTH, .selection_mode = GTK_SELECTION_NONE}),
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
	confirmationDialog(glade, "confirmationDialog"),
	messageDialog(glade, "messageDialog"),
	overrideConfirmDialog(glade, "overrideConfirmDialog"),
	passwordEntryDialog(glade, "passwordEntryDialog"),
	warningDialog(glade, "warningDialog"),
	warningListDialog(glade, "warningListDialog"),
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
	// 確認ダイアログ内
	confirmationLabel(glade, "confirmationLabel"),
	confirmationNoButton(glade, "confirmationNoButton"),
	confirmationYesButton(glade, "confirmationYesButton"),
	// メッセージダイアログ内
	messageDialogLabel(glade, "messageDialogLabel"),
	// 強制実行確認ダイアログ内
	overrideConfirmLabel(glade, "overrideConfirmLabel"),
	overrideConfirmOverrideButton(glade, "overrideConfirmOverrideButton"),
	overrideConfirmCancelButton(glade, "overrideConfirmCancelButton"),
	// パスワード入力ダイアログ内
	passwordEntryLabel(glade, "passwordEntryLabel"),
	passwordEntry(glade, "passwordEntry"),
	passwordCancelButton(glade, "passwordCancelButton"),
	passwordEnterButton(glade, "passwordEnterButton"),
	// 警告ダイアログ内
	warningLabel(glade, "warningLabel"),
	// 警告一覧ダイアログ内
	warningListBox(glade, "warningListBox"),
	warningList(glade, "warningList"),
	warningListstore(
		warningList,
		{ G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_RGBA },
		[this](sgex::ListStore* store, GtkTreeIter* iter, const WarningListData& row) {
			try {
				GdkRGBA bgcolor = utils::at_or(this->g_table_row_colors, row.cell_state, TABLE_BGCOLOR_NORMAL);
				store->set(iter,
					0, this->get_display_text(this->g_warning_type_messages.at(row.type)).c_str(),
					1, std::to_string(row.count).c_str(),
					2, &bgcolor
				);
			} catch (const std::out_of_range& e) {
				DEV_PRINT("Error: WarningType enum value out of range: {}\n", static_cast<int>(row.type));
				throw; // 再スローして呼び出し元で処理させる
			}
		}
	),
	warningListCloseButton(glade, "warningListCloseButton"),
	warningListSuppressButton(glade, "warningListSuppressButton"),
	// 棚卸し絞り込みダイアログ内
	reconcileToolTypeFilterLabel(glade, "reconcileToolTypeFilterLabel"),
	reconcilePeriodFilterLabel(glade, "reconcilePeriodFilterLabel"),
	reconcileFilterEnterButton(glade, "reconcileFilterEnterButton"),
	reconcileToolTypeFilterComboBox(glade, "reconcileToolTypeFilterComboBox"),
	reconcileToolTypeFilterComboBoxListstore(
		reconcileToolTypeFilterComboBox,
		{ G_TYPE_STRING },
		[](sgex::ListStore* store, GtkTreeIter* iter, const std::string& row) {
			store->set(iter,
				0, row.c_str()
			);
		}
	),
	reconcilePeriodFilterSwitch(glade, "reconcilePeriodFilterSwitch"),
	reconcileFilterStartYearMonthLabel(glade, "reconcileFilterStartYearMonthLabel"),
	reconcileFilterStartYearIncrementButton(glade, "reconcileFilterStartYearIncrementButton"),
	reconcileFilterStartYearEntry(glade, "reconcileFilterStartYearEntry"),
	reconcileFilterStartYearDecrementButton(glade, "reconcileFilterStartYearDecrementButton"),
	reconcileFilterStartYearLabel(glade, "reconcileFilterStartYearLabel"),
	reconcileFilterStartMonthIncrementButton(glade, "reconcileFilterStartMonthIncrementButton"),
	reconcileFilterStartMonthEntry(glade, "reconcileFilterStartMonthEntry"),
	reconcileFilterStartMonthDecrementButton(glade, "reconcileFilterStartMonthDecrementButton"),
	reconcileFilterStartMonthLabel(glade, "reconcileFilterStartMonthLabel"),
	reconcileFilterEndYearMonthLabel(glade, "reconcileFilterEndYearMonthLabel"),
	reconcileFilterEndYearIncrementButton(glade, "reconcileFilterEndYearIncrementButton"),
	reconcileFilterEndYearEntry(glade, "reconcileFilterEndYearEntry"),
	reconcileFilterEndYearDecrementButton(glade, "reconcileFilterEndYearDecrementButton"),
	reconcileFilterEndYearLabel(glade, "reconcileFilterEndYearLabel"),
	reconcileFilterEndMonthIncrementButton(glade, "reconcileFilterEndMonthIncrementButton"),
	reconcileFilterEndMonthEntry(glade, "reconcileFilterEndMonthEntry"),
	reconcileFilterEndMonthDecrementButton(glade, "reconcileFilterEndMonthDecrementButton"),
	reconcileFilterEndMonthLabel(glade, "reconcileFilterEndMonthLabel"),
	// 状態管理メンバ
	m_is_first_show(true),
	m_is_programmatically_changing_toggle(false),
	m_password_authenticated(false),
	m_gui_mode(GuiMode::RETRIEVE),
	m_app_state(AppState::WAITING_FOR_USER_LOGIN),
	m_text_mode(TextMode::KANJI),
	m_plc_ready_for_action(true)		// 最初は扉が閉まっているので true
{
	// --- コンストラクタ本体 ---

	// Gladeファイルの "mainWindow" の定義からウィンドウを初期化
	this->init(glade, "mainWindow");

	// ウィンドウの閉じるボタンでアプリケーションが終了するように設定
	this->setDefaultCloseOperation(sgex::Window::CloseOperation::DESTROY);

	// ダイアログは破棄せず、非表示にするだけ
	confirmInsertCountDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);
	confirmRegroundDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);
	confirmationDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);
	messageDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);
	overrideConfirmDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);
	passwordEntryDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);
	warningDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);
	warningListDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);
	reconcileFilterDialog.setDefaultCloseOperation(sgex::Window::CloseOperation::HIDE);

	this->set_timeout(100);	// 100ミリ秒ごとにタイムアウトイベントを発生させる

	this->fullscreen();	// フルスクリーンで表示

	// 中央揃えに設定するカラムの配列を作成
	std::vector<GtkTreeViewColumn*> center_aligned_columns;
	center_aligned_columns.reserve(15); // 予め要素数を確保
	center_aligned_columns.push_back(table3columns1st.get_column(0));
	center_aligned_columns.push_back(table3columns1st.get_column(5));
	center_aligned_columns.push_back(table3columns2nd.get_column(0));
	center_aligned_columns.push_back(table3columns2nd.get_column(5));
	center_aligned_columns.push_back(table3columns3rd.get_column(0));
	center_aligned_columns.push_back(table3columns3rd.get_column(5));
	center_aligned_columns.push_back(table2columns1st.get_column(0));
	center_aligned_columns.push_back(table2columns1st.get_column(5));
	center_aligned_columns.push_back(table2columns2nd.get_column(0));
	center_aligned_columns.push_back(table2columns2nd.get_column(5));
	center_aligned_columns.push_back(table2types1st.get_column(0));
	center_aligned_columns.push_back(table2types1st.get_column(5));
	center_aligned_columns.push_back(table2types2nd.get_column(0));
	center_aligned_columns.push_back(table2types2nd.get_column(5));
	center_aligned_columns.push_back(warningList.get_column(1));

	// 各カラムを中央揃えに設定
	for (auto& column : center_aligned_columns) {
		if (column != nullptr) {
			GList* cells = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(column));
			for (GList* cell_iter = cells; cell_iter != nullptr; cell_iter = cell_iter->next) {
				GtkCellRenderer* cell = GTK_CELL_RENDERER(cell_iter->data);
				g_object_set(cell, "xalign", 0.5, NULL);	// 0.0: 左揃え、0.5: 中央揃え、1.0: 右揃え
			}
			g_list_free(cells);
		}
	}

	center_aligned_columns.clear(); // 不要になったのでクリア

	// 各表を囲むウィジェットを設定
	storingToolListstore.set_enclosing_widget(&storingToolTableBox);
	liststore3columns1st.set_enclosing_widget(&table3columns1stBox);
	liststore3columns2nd.set_enclosing_widget(&table3columns2ndBox);
	liststore3columns3rd.set_enclosing_widget(&table3columns3rdBox);
	liststore2columns1st.set_enclosing_widget(&table2columns1stBox);
	liststore2columns2nd.set_enclosing_widget(&table2columns2ndBox);
	liststore2types1st.set_enclosing_widget(&table2types1stBox);
	liststore2types2nd.set_enclosing_widget(&table2types2ndBox);
	warningListstore.set_enclosing_widget(&warningListBox);

	// 単独で運用する TreeViewListStoreModel の容量を事前に確保
	storingToolListstore.reserve(1);
	warningListstore.reserve(6);	// 警告一覧は最大6種類の警告が表示される可能性があるため

	// PaginatedListStoreModel に TreeViewListStoreModel を登録
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
	liststore3columnsAll.on_selection_changed_by_user([this](std::optional<size_t> selected_index_opt) { this->mainTableSelection_on_changed(selected_index_opt); });
	prevButton3columns.on_clicked([this]() { this->update_and_handle_pagination_3columns(PaginationAction::GO_TO_PREV, true); });
	nextButton3columns.on_clicked([this]() { this->update_and_handle_pagination_3columns(PaginationAction::GO_TO_NEXT, true); });
	liststore2columnsAll.on_selection_changed_by_user([this](std::optional<size_t> selected_index_opt) { this->mainTableSelection_on_changed(selected_index_opt); });
	prevButton2columns.on_clicked([this]() { this->update_and_handle_pagination_2columns(PaginationAction::GO_TO_PREV, true); });
	nextButton2columns.on_clicked([this]() { this->update_and_handle_pagination_2columns(PaginationAction::GO_TO_NEXT, true); });
	prevButton2types1st.on_clicked([this]() { this->update_and_handle_pagination_2types1st(PaginationAction::GO_TO_PREV, true); });
	nextButton2types1st.on_clicked([this]() { this->update_and_handle_pagination_2types1st(PaginationAction::GO_TO_NEXT, true); });
	prevButton2types2nd.on_clicked([this]() { this->update_and_handle_pagination_2types2nd(PaginationAction::GO_TO_PREV, true); });
	nextButton2types2nd.on_clicked([this]() { this->update_and_handle_pagination_2types2nd(PaginationAction::GO_TO_NEXT, true); });
	qrCodeEntry.on_activate([this]() { this->qrCodeEntry_on_activate(); });
	falseButton.on_clicked([this]() { this->falseButton_on_clicked(); });
	trueButton.on_clicked([this]() { this->trueButton_on_clicked(); });
	confirmInsertCountDialog.on_show([this]() { this->confirmInsertCountDialog_on_show(); });
	passwordEntryDialog.on_show([this]() { this->passwordEntryDialog_on_show(); });
	warningListDialog.on_response([this](gint response_id) { this->warningDialog.hide(); });
	insertCountIncrementButton.on_clicked([this]() { this->insertCountIncrementButton_on_clicked(); });
	insertCountDecrementButton.on_clicked([this]() { this->insertCountDecrementButton_on_clicked(); });
	insertCountDetectButton.on_clicked([this]() { this->insertCountDetectButton_on_clicked(); });
	insertCountOkButton.on_clicked([this]() { this->insertCountOkButton_on_clicked(); });
	notRegroundButton.on_toggled([this]() { this->notRegroundButton_on_toggled(); });
	regroundButton.on_toggled([this]() { this->regroundButton_on_toggled(); });
	confirmRegroundOkButton.on_clicked([this]() { this->confirmRegroundDialog.response(GtkResponseType::GTK_RESPONSE_OK); });
	confirmationNoButton.on_clicked([this]() { this->confirmationDialog.response(GtkResponseType::GTK_RESPONSE_NO); });
	confirmationYesButton.on_clicked([this]() { this->confirmationDialog.response(GtkResponseType::GTK_RESPONSE_YES); });
	overrideConfirmOverrideButton.on_clicked([this]() { this->confirmationDialog.response(GtkResponseType::GTK_RESPONSE_YES); });
	overrideConfirmCancelButton.on_clicked([this]() { this->confirmationDialog.response(GtkResponseType::GTK_RESPONSE_CANCEL); });
	passwordEntry.on_activate([this]() { this->passwordEntryDialog.response(GtkResponseType::GTK_RESPONSE_OK); });	// 確定ボタンがクリックされた時と同様
	passwordCancelButton.on_clicked([this]() { this->passwordEntryDialog.response(GtkResponseType::GTK_RESPONSE_CANCEL); });
	passwordEnterButton.on_clicked([this]() { this->passwordEntryDialog.response(GtkResponseType::GTK_RESPONSE_OK); });
	warningList.on_map([this]() { this->warningListCloseButton_on_map(); });
	warningListCloseButton.on_clicked([this]() { this->warningListCloseButton_on_clicked(); });
	warningListSuppressButton.on_clicked([this]() { this->warningListSuppressButton_on_clicked(); });

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

	// --- 3列表示テーブルのページ番号表示ラベル ---
	m_widget_states.try_emplace(&pageLabel3columns, WidgetStateUpdater{
		{
			.text = UiText{"ページ  0 /  0", ""},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->pageLabel3columns.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->pageLabel3columns.set_visible(state.is_visible.value()); }
		}
	});

	// --- 2列表示テーブルのページ戻りボタン ---
	m_widget_states.try_emplace(&prevButton2columns, WidgetStateUpdater{
		{
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.can_focus.has_value()		) { this->prevButton2columns.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->prevButton2columns.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->prevButton2columns.set_visible(state.is_visible.value()); }
		}
	});

	// --- 2列表示テーブルのページ送りボタン ---
	m_widget_states.try_emplace(&nextButton2columns, WidgetStateUpdater{
		{
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.can_focus.has_value()		) { this->nextButton2columns.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->nextButton2columns.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->nextButton2columns.set_visible(state.is_visible.value()); }
		}
	});

	// --- 2列表示テーブルのページ番号表示ラベル ---
	m_widget_states.try_emplace(&pageLabel2columns, WidgetStateUpdater{
		{
			.text = UiText{"ページ  0 /  0", ""},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->pageLabel2columns.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->pageLabel2columns.set_visible(state.is_visible.value()); }
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

	// --- 2種表示テーブルのページ戻りボタン (1種目) ---
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

	// --- 2種表示テーブルのページ送りボタン (1種目) ---
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

	// --- 2種表示テーブルのページ番号表示ラベル (1種目) ---
	m_widget_states.try_emplace(&pageLabel2types1st, WidgetStateUpdater{
		{
			.text = UiText{"ページ  0 /  0", ""},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->pageLabel2types1st.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->pageLabel2types1st.set_visible(state.is_visible.value()); }
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

	// --- 2種表示テーブルのページ番号表示ラベル (2種目) ---
	m_widget_states.try_emplace(&pageLabel2types2nd, WidgetStateUpdater{
		{
			.text = UiText{"ページ  0 /  0", ""},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->pageLabel2types2nd.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->pageLabel2types2nd.set_visible(state.is_visible.value()); }
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
			.text = UiText{"一度取り出された工具を検出しました\n再研磨された工具ですか？", "いちど とりだされた こうぐを けんしゅつしました\nさいけんま された こうぐですか？"},
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

	// --- 確認ダイアログ ---
	m_widget_states.try_emplace(&confirmationLabel, WidgetStateUpdater{
		{
			.text = UiText{"", ""},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->confirmationLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->confirmationLabel.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&confirmationNoButton, WidgetStateUpdater{
		{
			.text = UiText{"いいえ", "いいえ"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->confirmationNoButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->confirmationNoButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->confirmationNoButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->confirmationNoButton.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&confirmationYesButton, WidgetStateUpdater{
		{
			.text = UiText{"はい", "はい"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->confirmationYesButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->confirmationYesButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->confirmationYesButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->confirmationYesButton.set_visible(state.is_visible.value()); }
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
	m_widget_states.try_emplace(&warningListCloseButton, WidgetStateUpdater{
		{
			.text = UiText{"閉じる", "とじる"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->warningListCloseButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->warningListCloseButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->warningListCloseButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->warningListCloseButton.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&warningListSuppressButton, WidgetStateUpdater{
		{
			.text = UiText{"しばらく無視", "しばらく むし"},
			.can_focus		= false,
			.is_sensitive	= true,
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->warningListSuppressButton.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.can_focus.has_value()		) { this->warningListSuppressButton.set_can_focus(state.can_focus.value()); }
			if (state.is_sensitive.has_value()	) { this->warningListSuppressButton.set_sensitive(state.is_sensitive.value()); }
			if (state.is_visible.has_value()	) { this->warningListSuppressButton.set_visible(state.is_visible.value()); }
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
			.text = UiText{"期間を指定", "きかんを してい"},
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
	m_widget_states.try_emplace(&reconcileFilterStartYearMonthLabel, WidgetStateUpdater{
		{
			.text = UiText{"開始年月", "かいし\nねんげつ"},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->reconcileFilterStartYearMonthLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->reconcileFilterStartYearMonthLabel.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&reconcileFilterStartYearLabel, WidgetStateUpdater{
		{
			.text = UiText{"年", "ねん"},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->reconcileFilterStartYearLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->reconcileFilterStartYearLabel.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&reconcileFilterStartMonthLabel, WidgetStateUpdater{
		{
			.text = UiText{"月", "がつ"},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->reconcileFilterStartMonthLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->reconcileFilterStartMonthLabel.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&reconcileFilterEndYearMonthLabel, WidgetStateUpdater{
		{
			.text = UiText{"終了年月", "しゅうりょう\nねんげつ"},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->reconcileFilterEndYearMonthLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->reconcileFilterEndYearMonthLabel.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&reconcileFilterEndYearLabel, WidgetStateUpdater{
		{
			.text = UiText{"年", "ねん"},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->reconcileFilterEndYearLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->reconcileFilterEndYearLabel.set_visible(state.is_visible.value()); }
		}
	});
	m_widget_states.try_emplace(&reconcileFilterEndMonthLabel, WidgetStateUpdater{
		{
			.text = UiText{"月", "がつ"},
			.is_visible		= true
		},
		[this](const WidgetState& state){
			if (state.text.has_value()			) { this->reconcileFilterEndMonthLabel.set_label(get_display_text(state.text.value()).c_str()); }
			if (state.is_visible.has_value()	) { this->reconcileFilterEndMonthLabel.set_visible(state.is_visible.value()); }
		}
	});

	//=========================================================================
	// 各テーブルのカラムヘッダー表示テキストを保持するマップの構築
	//=========================================================================

	// --- 補充する工具の情報を表示するテーブル ---
	m_table_states.try_emplace(&storingToolListstore, TableState{
		.column_headers = std::vector<UiText>{
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"}
		},
		.is_sensitive	= true,
		.is_visible		= true,
		.is_selectable	= false
	});

	// --- 3列表示テーブル ---
	m_table_states.try_emplace(&liststore3columnsAll, TableState{
		.column_headers = std::vector<UiText>{
			{"ツール\n  番号", "  ツール\nばんごう"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"在庫数", "ざいこ\n  すう"}
		},
		.is_sensitive	= true,
		.is_visible		= true,
		.is_selectable	= false
	});

	// --- 2列表示テーブル ---
	m_table_states.try_emplace(&liststore2columnsAll, TableState{
		.column_headers = std::vector<UiText>{
			{"ID", "ID"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"再研磨\n  回数", "さいけんま\n  かいすう"}
		},
		.is_sensitive	= true,
		.is_visible		= true,
		.is_selectable	= false
	});

	// --- 2種表示テーブル ---
	m_table_states.try_emplace(&paginatedListstore2types1st, TableState{
		.column_headers = std::vector<UiText>{
			{"ID", "ID"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"再研磨\n  回数", "さいけんま\n  かいすう"}
		},
		.is_sensitive	= true,
		.is_visible		= true,
		.is_selectable	= false
	});
	m_table_states.try_emplace(&paginatedListstore2types2nd, TableState{
		.column_headers = std::vector<UiText>{
			{"ID", "ID"},
			{"分類", "ぶんるい"},
			{"メーカー", "メーカー"},
			{"型番", "かたばん"},
			{"材質", "ざいしつ"},
			{"再研磨\n  回数", "さいけんま\n  かいすう"}
		},
		.is_sensitive	= true,
		.is_visible		= true,
		.is_selectable	= false
	});

	// --- 警告一覧 ---
	m_table_states.try_emplace(&warningListstore, TableState{
		.column_headers = std::vector<UiText>{
			{"警告", "けいこく"},
			{"発生件数", "はっせい\nけんすう"}
		},
		.is_sensitive	= true,
		.is_visible		= true,
		.is_selectable	= false
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
			{ {&falseButton}, {.text = UiText{"全選択解除", "ぜんせんたくかいじょ"}, .is_sensitive = true} }
		},
		.table_states = {
			{ {&liststore2columnsAll}, {
				.column_headers = std::vector<UiText>{
					{"ツール\n  番号", "  ツール\nばんごう"},
					{"分類", "ぶんるい"},
					{"メーカー", "メーカー"},
					{"型番", "かたばん"},
					{"材質", "ざいしつ"},
					{"在庫数", "ざいこ\n  すう"}
				},
				.is_sensitive = true,
				.is_selectable = true
			} }
		},
		// --- 各スタックの「表示ページ」を定義 ---
		.tableStackPageName		= "table2columns",
		// --- 状態の「振る舞い」を定義 ---
		.on_prepare_state = [](MainWindow* thiz){
			thiz->reset_header_buttons(false);

			// 選択された工具がある場合にのみ「確定」ボタンを有効化する
			thiz->update_widget_state(&(thiz->trueButton), WidgetState{
				.text = UiText{"確定", "かくてい"},
				.is_sensitive = (thiz->liststore2columnsAll.find_first_index([](const MainTableData& item) {
					return item.cell_state_0to4 == TableCellState::SELECTED;
				}).has_value())
			}, true);
		},
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
		.table_states = {
			{ {&liststore2columnsAll}, {.is_selectable = false} }
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
			{ {&liststore2columnsAll}, {.is_sensitive = false} }
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
			thiz->update_table_state(&(thiz->liststore2columnsAll), {
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
				},
				.is_sensitive = true,
				.is_selectable = false
			}, false);
		},
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_retrieve_ready_for_reading(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_retrieve_ready_for_reading(); },
		.on_falseButton_clicked		= [](MainWindow* thiz){ thiz->on_falseButton_retrieve_ready_for_reading(); },
		.on_trueButton_clicked		= [](MainWindow* thiz){ thiz->on_trueButton_retrieve_ready_for_reading(); },
		.on_plc_notified			= [](MainWindow* thiz, const plc::Notification& notification){ thiz->on_plc_retrieve_ready_for_reading(notification); },
		.on_tracker_notified		= [](MainWindow* thiz, const tracker::ToolTrackerClient::Notification& notification){ thiz->on_tracker_retrieve_ready_for_reading(notification); }
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
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_retrieve_ready_for_reading(); },
		.on_falseButton_clicked		= [](MainWindow* thiz){ thiz->on_falseButton_retrieve_ready_for_reading(); },
		.on_trueButton_clicked		= [](MainWindow* thiz){ thiz->on_trueButton_retrieve_ready_for_reading(); },
		.on_plc_notified			= [](MainWindow* thiz, const plc::Notification& notification){ thiz->on_plc_retrieve_ready_for_reading(notification); },
		.on_tracker_notified		= [](MainWindow* thiz, const tracker::ToolTrackerClient::Notification& notification){ thiz->on_tracker_retrieve_ready_for_reading(notification); }
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
			{ {&liststore2columnsAll}, {.is_sensitive = false, .is_selectable = false} }
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
			thiz->update_table_state(&(thiz->liststore2columnsAll), {
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
				},
				.is_sensitive = true,
				.is_selectable = false
			}, false);
		},
		.on_enter_state				= [](MainWindow* thiz){ thiz->on_enter_store_ready_for_reading(); },
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_store_ready_for_reading(); },
		.on_falseButton_clicked		= [](MainWindow* thiz){ thiz->on_falseButton_store_ready_for_reading(); },
		.on_trueButton_clicked		= [](MainWindow* thiz){ thiz->on_trueButton_store_ready_for_reading(); },
		.on_plc_notified			= [](MainWindow* thiz, const plc::Notification& notification){ thiz->on_plc_store_ready_for_reading(notification); }
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
		.on_qrCodeEntry_activate	= [](MainWindow* thiz){ thiz->on_qrCodeEntry_store_ready_for_reading(); },
		.on_falseButton_clicked		= [](MainWindow* thiz){ thiz->on_falseButton_store_ready_for_reading(); },
		.on_trueButton_clicked		= [](MainWindow* thiz){ thiz->on_trueButton_store_ready_for_reading(); },
		.on_plc_notified			= [](MainWindow* thiz, const plc::Notification& notification){ thiz->on_plc_store_ready_for_reading(notification); }
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
			{ {&liststore2columnsAll}, {
				.column_headers = std::vector<UiText>{
					{"コンテナ\n  番号", "コンテナ\nばんごう"},
					{"分類", "ぶんるい"},
					{"メーカー", "メーカー"},
					{"型番", "かたばん"},
					{"材質", "ざいしつ"},
					{"  　最終\n棚卸し日時", "  　さいしゅう\nたなおろしにちじ"}
				},
				.is_sensitive = true,
				.is_selectable = true
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
			{ {&liststore2columnsAll}, {.is_sensitive = false, .is_selectable = false} }
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
			thiz->update_table_state(&(thiz->liststore2columnsAll), {
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
				},
				.is_sensitive = true,
				.is_selectable = false
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
						"ステータスモニタでの操作により、画面がロックされました\n詳細はステータスモニタをご覧ください",
						"ステータスモニタでの そうさにより、がめんが ロックされました\nしょうさいは ステータスモニタを ごらんください"
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

void MainWindow::tracker_on_notified(const tracker::ToolTrackerClient::Notification& notification) {
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
	// --- グローバルな要素 ---
	db::disconnect();
	g_logged_in_user_info = nullopt;

	// --- 状態遷移管理用 ---
	m_password_authenticated = false;

	// --- ウィジェット制御用 ---
	m_is_insert_operation = false;
	m_insertCountEntry_current_count = 0;
	m_confirmInsertCountDialog_max_count = -1;

	// --- 操作中の状態保持用 (複数場面共通) ---
	m_operating_area.reset();
	m_operating_tool.reset();
	m_operated_records.clear();
	m_pending_tool_info.reset();
	m_plc_ready_for_action = false;
	m_warning_closed_by_user = false;
	m_ignore_all_warnings = false;

	// --- 操作中の状態保持用 (取り出し専用) ---
	m_retrieving_list.reset();
	m_retrieve_backup_table.clear();
	m_retrieving_tools.clear();
	m_retrieving_index.reset();
	m_unscanned_count = 0;

	// --- 操作中の状態保持用 (補充専用) ---
	m_reground_tools.clear();


	// --- 操作中の状態保持用 (棚卸し専用) ---

	// --- 各エントリーを空に ---
	userIdEntry.set_text("");
	lineEntry.set_text("");
	containerEntry.set_text("");
	programEntry.set_text("");
	qrCodeEntry.set_text("");
	insertCountEntry.set_text("");
	passwordEntry.set_text("");
	// TODO: 棚卸しコンテナリスト絞り込みダイアログの Entry もクリアする？

	// --- 各表を空に ---
	storingToolListstore.clear();
	liststore3columnsAll.clear();
	liststore2columnsAll.clear();
	paginatedListstore2types1st.clear();
	paginatedListstore2types2nd.clear();

	// --- テキストモードを漢字にリセット ---
	set_text_mode(TextMode::KANJI);
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

void MainWindow::update_widget_state(sg::Widget* widget, const WidgetState& state, bool update_ui) {
	// widget に対応する WidgetStateUpdater をその場で更新し、変更があった場合に更新を記録
	bool changed = m_widget_states.modify(widget, [&](auto& updater) {
		// その場の参照に対して merge を実行し、変更があったかどうかを返す
		return updater.state.merge(state);
	});

	// 変更があった場合、UIを更新
	if (update_ui && changed) {
		refresh_ui(false);
	}
}

void MainWindow::update_widget_state(sg::Widget* widget, WidgetState&& state, bool update_ui) {
	// widget に対応する WidgetStateUpdater をその場で更新し、変更があった場合に更新を記録
	bool changed = m_widget_states.modify(widget, [&](auto& updater) {
		// その場の参照に対して merge を実行し、変更があったかどうかを返す
		return updater.state.merge(std::move(state));
	});

	// 変更があった場合、UIを更新
	if (update_ui && changed) {
		refresh_ui(false);
	}
}

void MainWindow::update_widget_state(const std::vector<sg::Widget*>& widgets, const WidgetState& state, bool update_ui) {
	for (auto* widget : widgets) {
		update_widget_state(widget, state, false);
	}

	if (update_ui) {
		refresh_ui(false);
	}
}

void MainWindow::update_table_state(sgex::ListStoreViewInterface* table, const TableState& state, bool update_ui) {
	// table に対応する TableState をその場で更新し、変更があった場合に更新を記録
	bool changed = m_table_states.modify(table, [&](auto& current_state) {
		// その場の参照に対して merge を実行し、変更があったかどうかを返す
		return current_state.merge(state);
	});

	// 変更があった場合、UIを更新
	if (update_ui && changed) {
		refresh_ui(false);
	}
}

void MainWindow::update_table_state(sgex::ListStoreViewInterface* table, TableState&& state, bool update_ui) {
	// table に対応する TableState をその場で更新し、変更があった場合に更新を記録
	bool changed = m_table_states.modify(table, [&](auto& current_state) {
		// その場の参照に対して merge を実行し、変更があったかどうかを返す
		return current_state.merge(std::move(state));
	});

	// 変更があった場合、UIを更新
	if (update_ui && changed) {
		refresh_ui(false);
	}
}

void MainWindow::update_table_state(const std::vector<sgex::ListStoreViewInterface*>& tables, const TableState& state, bool update_ui) {
	for (auto* table : tables) {
		update_table_state(table, state, false);
	}

	if (update_ui) {
		refresh_ui(false);
	}
}

void MainWindow::refresh_ui(bool force_redraw_all) {
	// --- ウィジェットの更新処理 ---

	// 共通の更新ロジックをラムダ式で定義 (DRY原則)
	auto apply_widget_update = [](const auto& key, const auto& state_updater) {
		// セッター関数が登録されていれば呼び出す
		if (state_updater.setter) {
			// セッター関数を呼び出して、テキストと状態を更新
			state_updater.setter(state_updater.state);
		}
	};

	if (force_redraw_all) {
		// 全要素を走査 (ロック内/参照アクセス) + 履歴クリア
		m_widget_states.for_each(apply_widget_update, true);
	} else {
		// 更新要素のみ走査 (ロック内/参照アクセス) + 履歴クリア
		m_widget_states.for_each_updated(apply_widget_update, true);
	}

	// --- テーブルの更新処理 ---

	// 共通の更新ロジックをラムダ式で定義
	auto apply_table_update = [this](const auto& table, const auto& state) {
		if (state.column_headers.has_value()) {
			guint n_columns = table->get_n_columns();	// カラム数
			guint n_headers = state.column_headers.value().size();	// ヘッダー数
			for (guint i = 0; (i < n_headers && i < n_columns); i++) {
				// get_display_text はメンバ関数と仮定して this-> で呼び出し
				table->set_title(i, this->get_display_text(state.column_headers.value().at(i)).c_str());
			}
		}
		if (state.is_sensitive.has_value()) {
			table->set_sensitive(state.is_sensitive.value());
		}
		if (state.is_visible.has_value()) {
			table->set_visible(state.is_visible.value());
		}
		if (state.is_selectable.has_value()){
			table->set_selection_mode(((state.is_selectable.value())
				? (GtkSelectionMode::GTK_SELECTION_SINGLE)
				: (GtkSelectionMode::GTK_SELECTION_NONE)
			));
		}
	};

	if (force_redraw_all) {
		m_table_states.for_each(apply_table_update, true);
	} else {
		m_table_states.for_each_updated(apply_table_update, true);
	}

	if (force_redraw_all) {
		warningListstore.refresh(false);
	}
}

void MainWindow::set_app_state(AppState state) {
	// メンバ変数の状態を更新 & 一部リセット
	m_app_state = state;

	// m_widget_statesを更新する
	const auto& state_config = g_app_state_configs.at(state);
	for (const auto& widget_state : state_config.widget_states) {
		update_widget_state(widget_state.widgets, widget_state.state, false);
	}

	// m_table_statesを更新する
	for (const auto& table_state : state_config.table_states) {
		update_table_state(table_state.tables, table_state.state, false);
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

void MainWindow::set_text_mode(TextMode text_mode, bool update_ui) {
	if (m_text_mode != text_mode) {
		m_text_mode = text_mode;

		if (update_ui) {
			refresh_ui(true);
		}
	}
}

void MainWindow::main_ListStore_setter(sgex::ListStore* store, GtkTreeIter* iter, const MainTableData& row) {
	GdkRGBA bgcolor_0to4;
	GdkRGBA bgcolor_5;
	bgcolor_0to4 = utils::at_or(g_table_row_colors, row.cell_state_0to4, TABLE_BGCOLOR_NORMAL);
	bgcolor_5 = ((row.cell_state_5 == TableCellState::NONE)
		? (bgcolor_0to4)
		: (utils::at_or(g_table_row_colors, row.cell_state_5, bgcolor_0to4))
	);
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
	// 現在のテーブルページが3カラム表示でなければ何もしない
	if (std::string(tableStack.get_visible_child_name()) != "table3columns") {
		return;
	}

	// 引数に応じてページ送り操作を実行
	if (action == PaginationAction::GO_TO_PREV) {
		liststore3columnsAll.prev_page();
	} else if (action == PaginationAction::GO_TO_NEXT) {
		liststore3columnsAll.next_page();
	}
	// actionがUPDATE_UI_ONLYの場合は、ページ送りは行わない

	// モデルの現在の状態に基づいて、UIを更新
	std::string page_label = fmt::format("ページ {:2d} / {:2d}",
		liststore3columnsAll.get_current_page(),
		liststore3columnsAll.get_total_pages()
	);
	update_widget_state(&pageLabel3columns, WidgetState{ .text = UiText{page_label, page_label} }, false);
	update_widget_state(&prevButton3columns, WidgetState{ .is_sensitive = liststore3columnsAll.has_prev_page() }, false);
	update_widget_state(&nextButton3columns, WidgetState{ .is_sensitive = liststore3columnsAll.has_next_page() }, false);

	if (update_ui) {
		refresh_ui(false);
	}
}

void MainWindow::update_and_handle_pagination_2columns(PaginationAction action, bool update_ui) {
	// 現在のテーブルページが2カラム表示でなければ何もしない
	if (std::string(tableStack.get_visible_child_name()) != "table2columns") {
		return;
	}

	// 引数に応じてページ送り操作を実行
	if (action == PaginationAction::GO_TO_PREV) {
		liststore2columnsAll.prev_page();
	} else if (action == PaginationAction::GO_TO_NEXT) {
		liststore2columnsAll.next_page();
	}
	// actionがUPDATE_UI_ONLYの場合は、ページ送りは行わない

	// モデルの現在の状態に基づいて、UIを更新
	std::string page_label = fmt::format("ページ {:2d} / {:2d}",
		liststore2columnsAll.get_current_page(),
		liststore2columnsAll.get_total_pages()
	);
	update_widget_state(&pageLabel2columns, WidgetState{ .text = UiText{page_label, page_label} }, false);
	update_widget_state(&prevButton2columns, WidgetState{ .is_sensitive = liststore2columnsAll.has_prev_page() }, false);
	update_widget_state(&nextButton2columns, WidgetState{ .is_sensitive = liststore2columnsAll.has_next_page() }, false);

	if (update_ui) {
		refresh_ui(false);
	}
}

void MainWindow::update_and_handle_pagination_2types1st(PaginationAction action, bool update_ui) {
	// 現在のテーブルページが2種表示でなければ何もしない
	if (std::string(tableStack.get_visible_child_name()) != "table2types") {
		return;
	}

	// 引数に応じてページ送り操作を実行
	if (action == PaginationAction::GO_TO_PREV) {
		paginatedListstore2types1st.prev_page();
	} else if (action == PaginationAction::GO_TO_NEXT) {
		paginatedListstore2types1st.next_page();
	}
	// actionがUPDATE_UI_ONLYの場合は、ページ送りは行わない

	// モデルの現在の状態に基づいて、UIを更新
	std::string page_label = fmt::format("ページ {:2d} / {:2d}",
		paginatedListstore2types1st.get_current_page(),
		paginatedListstore2types1st.get_total_pages()
	);
	update_widget_state(&pageLabel2types1st, WidgetState{ .text = UiText{page_label, page_label} }, false);
	update_widget_state(&prevButton2types1st, WidgetState{ .is_sensitive = paginatedListstore2types1st.has_prev_page() }, false);
	update_widget_state(&nextButton2types1st, WidgetState{ .is_sensitive = paginatedListstore2types1st.has_next_page() }, false);

	if (update_ui) {
		refresh_ui(false);
	}
}

void MainWindow::update_and_handle_pagination_2types2nd(PaginationAction action, bool update_ui) {
	// 現在のテーブルページが2種表示でなければ何もしない
	if (std::string(tableStack.get_visible_child_name()) != "table2types") {
		return;
	}

	// 引数に応じてページ送り操作を実行
	if (action == PaginationAction::GO_TO_PREV) {
		paginatedListstore2types2nd.prev_page();
	} else if (action == PaginationAction::GO_TO_NEXT) {
		paginatedListstore2types2nd.next_page();
	}
	// actionがUPDATE_UI_ONLYの場合は、ページ送りは行わない

	// モデルの現在の状態に基づいて、UIを更新
	std::string page_label = fmt::format("ページ {:2d} / {:2d}",
		paginatedListstore2types2nd.get_current_page(),
		paginatedListstore2types2nd.get_total_pages()
	);
	update_widget_state(&pageLabel2types2nd, WidgetState{ .text = UiText{page_label, page_label} }, false);
	update_widget_state(&prevButton2types2nd, WidgetState{ .is_sensitive = paginatedListstore2types2nd.has_prev_page() }, false);
	update_widget_state(&nextButton2types2nd, WidgetState{ .is_sensitive = paginatedListstore2types2nd.has_next_page() }, false);

	if (update_ui) {
		refresh_ui(false);
	}
}

bool MainWindow::confirm_to_user(const UiText& message) {
	// ラベルにメッセージを設定
	update_widget_state(&confirmationLabel, WidgetState{ .text = message }, true);

	// TODO: 通知音再生？
	// ダイアログをモーダル表示し、ユーザーの応答を待つ
	if (confirmationDialog.run(this) == GtkResponseType::GTK_RESPONSE_YES) {
		// ユーザーが「はい」を選択した場合
		DEV_PRINT("ユーザー確認: はい\n");
		return true;
	} else {
		// ユーザーが「いいえ」を選択した場合
		DEV_PRINT("ユーザー確認: いいえ\n");
		return false;
	}
}

bool MainWindow::confirm_to_user(UiText&& message) {
	// ラベルにメッセージを設定
	update_widget_state(&confirmationLabel, WidgetState{ .text = std::move(message) }, true);

	// TODO: 通知音再生？
	// ダイアログをモーダル表示し、ユーザーの応答を待つ
	if (confirmationDialog.run(this) == GtkResponseType::GTK_RESPONSE_YES) {
		// ユーザーが「はい」を選択した場合
		DEV_PRINT("ユーザー確認: はい\n");
		return true;
	} else {
		// ユーザーが「いいえ」を選択した場合
		DEV_PRINT("ユーザー確認: いいえ\n");
		return false;
	}
}

bool MainWindow::override_confirm_to_user(const UiText& message) {
	// ラベルにメッセージを設定
	update_widget_state(&overrideConfirmLabel, WidgetState{ .text = message }, true);

	// TODO: 警告音再生
	// ダイアログをモーダル表示し、ユーザーの応答を待つ
	if (overrideConfirmDialog.run(this) == GtkResponseType::GTK_RESPONSE_YES) {
		// ユーザーが「無視して続ける」を選択した場合
		DEV_PRINT("強制実行確認: 無視して続行\n");
		return true;
	} else {
		// ユーザーが「キャンセル」を選択した場合
		DEV_PRINT("強制実行確認: キャンセル\n");
		return false;
	}
}

bool MainWindow::override_confirm_to_user(UiText&& message) {
	// ラベルにメッセージを設定
	update_widget_state(&overrideConfirmLabel, WidgetState{ .text = std::move(message) }, true);

	// TODO: 警告音再生
	// ダイアログをモーダル表示し、ユーザーの応答を待つ
	if (overrideConfirmDialog.run(this) == GtkResponseType::GTK_RESPONSE_YES) {
		// ユーザーが「無視して続ける」を選択した場合
		DEV_PRINT("強制実行確認: 無視して続行\n");
		return true;
	} else {
		// ユーザーが「キャンセル」を選択した場合
		DEV_PRINT("強制実行確認: キャンセル\n");
		return false;
	}
}

void MainWindow::update_read_completed_buttons(bool update_ui) {
	switch (m_app_state) {
	case AppState::RETRIEVE_READY_FOR_READING :
	case AppState::RETRIEVE_READ_COMPLETED :
		update_retrieve_read_completed_buttons(update_ui);
		break;
	default :
		break;
	}
}

bool MainWindow::update_warning_display() {
	bool has_state_warning = false;
	WarningType scan_error_type = WarningType::NONE;

	// リスト内の警告をチェック
	// (ListStoreModelBase に const_iterator がある前提)
	for (const auto& item : warningListstore) {
		if (is_state_warning(item.type)) {
			has_state_warning = true;
		} else {
			// スキャンエラーのタイプを保持しておく（最新1件を表示するため）
			// リストの末尾にあるものが最新と仮定
			scan_error_type = item.type;
		}
	}

	if (has_state_warning && !m_ignore_all_warnings) {
		// --- パターンA: 重要警告がある場合 ---
		// ダイアログを表示 (スキャンエラーもリストに含まれているので一緒に見える)
		if (!warningListDialog.get_mapped()) {
			warningListDialog.show_all();
		}
	} else if (scan_error_type != WarningType::NONE) {
		// --- パターンB: スキャン警告のみの場合 ---
		// ダイアログは隠す
		warningListDialog.hide();

		// メッセージラベルに警告用のスタイルを適用してエラー内容を表示
		messageLabel.add_css_class(CSS_BGCOLOR_WARNING);

		update_widget_state(&messageLabel, WidgetState{
			.text = g_warning_type_messages.at(scan_error_type)	// 内容を表示
		}, false);

	} else {
		// --- パターンC: 警告なし ---
		warningListDialog.hide();

		// エラー表示用スタイルを外す
		messageLabel.remove_css_class(CSS_BGCOLOR_WARNING);

		// ここで本来の案内メッセージに戻すが、
		// スキャン成功時には自動的に現在の画面に再遷移するため、ここでの書き換えは不要
	}

	update_read_completed_buttons(false);
	refresh_ui(false);
	return has_state_warning;
}

void MainWindow::add_warning(WarningType type, bool update_ui) {
	/* TODO: 警告音再生
	if (!m_ignore_all_warnings || !is_state_warning(type)) {
	}
	*/

	// 既存のハイライトを探して解除
	// (イテレータで回しても良いですが、更新にはindexが必要なので従来のループ推奨)
	for (size_t i = 0; i < warningListstore.size(); ++i) {
		// at() で要素参照
		const auto& item_ref = warningListstore.at(i);

		if (item_ref.cell_state == TableCellState::HIGHLIGHT) {
			// 今回更新する対象と同じなら、リセットせずスキップ（後のステップで上書き更新されるため）
			if (item_ref.type == type) {
				break;
			}

			// 別の行ならNORMALに戻して更新
			WarningListData item = item_ref;
			item.cell_state = TableCellState::NORMAL;
			warningListstore.update(i, std::move(item));
			break; // ハイライトは1つという前提
		}
	}

	// 二分探索 (std::lower_bound)
	// モデル自体が begin()/end() を持っているので、直接渡せます
	auto it = std::lower_bound(warningListstore.begin(), warningListstore.end(), type,
		[](const WarningListData& a, WarningType b) {
			return a.type < b;
		}
	);

	// イテレータをインデックスに変換
	// insert や update はインデックスを要求するため変換が必要です
	size_t index = std::distance(warningListstore.begin(), it);

	// 更新 または 挿入
	// it が end() でなく、かつ値が一致すれば「既存あり」
	if (it != warningListstore.end() && it->type == type) {
		// --- 既に存在する場合 (Update) ---
		warningListstore.update(index, WarningListData{
			.type = it->type,
			.count = it->count + 1,
			.cell_state = TableCellState::HIGHLIGHT
		});
	} else {
		// --- 存在しない場合 (Insert) ---
		// 見つかった位置(index)に挿入してソート順を維持
		warningListstore.insert(index, WarningListData{
			.type = type,
			.count = 1,
			.cell_state = TableCellState::HIGHLIGHT
		});
	}

	if (update_ui){
		update_warning_display();	// 警告表示を更新
	}
}

std::pair<int, int> MainWindow::remove_warning(WarningType type, bool force_clear, bool update_ui) {
	// 二分探索で対象の要素を探す
	auto it = std::lower_bound(warningListstore.begin(), warningListstore.end(), type,
		[](const WarningListData& a, WarningType b) {
			return a.type < b;
		}
	);

	// 見つかったかどうかの判定
	if (it != warningListstore.end() && it->type == type) {
		size_t index = std::distance(warningListstore.begin(), it);
		WarningListData item = *it;	// データをコピー

		int decreased_count = 0;	// 減らした数

		// カウント操作
		if (force_clear) {
			// 強制削除なら、現在のカウント全てを減らす
			decreased_count = item.count;
			item.count = 0;
		} else {
			// 通常削除なら、1つだけ減らす
			decreased_count = 1;
			item.count--;
		}

		item.cell_state = TableCellState::NORMAL;	// 状態をNORMALに

		// 削除 または 更新
		if (item.count <= 0) {
			// カウントが0以下になったらリストから削除
			warningListstore.erase(index);
		} else {
			// まだ残っているなら更新
			warningListstore.update(index, std::move(item));
		}

		if (update_ui){
			update_warning_display();	// 警告表示を更新
		}
		return {decreased_count, item.count};	// 結果を返す {減らした数, 現在の数}
	}

	if (update_ui){
		update_warning_display();	// 警告表示を更新
	}
	return {0, 0};	// 対象が見つからなかった場合
}

void MainWindow::clear_warnings(bool update_ui) {
	// 警告があればクリア処理を実行
	if (!warningListstore.empty()) {
		// リストを全削除 (モデルとビューの両方がクリアされます)
		warningListstore.clear();
	}
	if (update_ui){
		update_warning_display();	// 警告表示を更新
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
			if (response == GtkResponseType::GTK_RESPONSE_OK) {
				DEV_PRINT("認証開始\n");
				if (db::authenticate_with_password(std::string(passwordEntry.get_text()))) {
					DEV_PRINT("認証成功\n");
					m_password_authenticated = true;
				} else {
					DEV_PRINT("認証失敗：パスワードが違います\n");
					update_widget_state(&passwordEntryLabel, WidgetState{	// ラベルのテキストを更新
						.text = UiText{"パスワードが違います\nもう一度入力してください", "パスワードが ちがいます\nもういちど にゅうりょく してください"}
					}, true);
				}
			} else if (response == GtkResponseType::GTK_RESPONSE_CANCEL || response == GtkResponseType::GTK_RESPONSE_DELETE_EVENT) {
				// キャンセルボタンが押されたか、ダイアログが閉じられた場合
				m_password_authenticated = false;
				DEV_PRINT("認証キャンセル\n");
			}
		} while (response == GtkResponseType::GTK_RESPONSE_OK && !m_password_authenticated);

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
	for (const auto &tool_assignment : tool_assignments) {
		table_data_list.push_back(MainTableData{
			// ToolAssignmentInfo と MstToolInfo から MainTableData に値をコピー
			.id = tool_assignment.id,	// 行の識別に使う (今回は tool_assignment_id を流用)
			.data_columns = {
				tool_assignment.tool_num,									// ツール番号
				tool_assignment.mst_tool_info.category,						// 分類
				tool_assignment.mst_tool_info.maker,						// メーカー
				tool_assignment.mst_tool_info.name,							// 型番
				tool_assignment.mst_tool_info.material,						// 材質
				std::to_string(tool_assignment.mst_tool_info.stc.value()),	// BUG: nullopt対応 在庫数 (文字列に変換)
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

void MainWindow::update_retrieve_read_completed_buttons(bool update_ui) {
	// GUIの条件を判定
	bool gui_ready_for_action = !warningDialog.get_mapped()
		&& !warningListDialog.get_mapped()
		&& !m_pending_tool_info.has_value();	// BUG: 固まって応答が無いときの対応を考える (https://gemini.google.com/share/759604d1a439 ←最後の方の2つの会話)

	// 選択された中で最後の工具の時 (またはリストが空の時)
	bool is_last_tool = (!m_retrieving_index.has_value() || (m_retrieving_index.value() + 1 >= m_retrieving_tools.size()));

	// 全システムの準備状態を判定
	bool all_systems_ready = gui_ready_for_action && m_plc_ready_for_action;

	// ボタンの状態を更新 (update_ui = false で即時描画はしない)

	// 終了ボタン: 全システムが準備OK かつ GUI条件(最後の工具) を満たす場合のみ有効
	update_widget_state(&falseButton, WidgetState{
		.is_sensitive = (all_systems_ready && is_last_tool)
	}, false);

	// 続ける/次へボタン: 全システムが準備OK の場合のみ有効
	update_widget_state(&trueButton, WidgetState{
		.is_sensitive = all_systems_ready
	}, false);

	// 4. 最後にUIを一括更新
	if (update_ui) {
		refresh_ui(false);
	}
}

bool MainWindow::confirm_if_reground(bool default_value) {
	// 1. ダイアログの初期状態を設定
	{
		utils::scoped_value_guard guard(m_is_programmatically_changing_toggle, true);
		notRegroundButton.set_active(!default_value);
		regroundButton.set_active(default_value);
	}

	// 2. ダイアログをモーダル表示し、ユーザーの応答を待つ
	confirmRegroundDialog.run(this);
	confirmRegroundDialog.hide();

	// 3. OKボタンが押されたら、トグルボタンの状態を確認して返す
	return regroundButton.get_active();
}

//=============================================================================
// プライベートメンバ関数 (イベントハンドラ) 実装
//=============================================================================

void MainWindow::on_window_show() {
	if (m_is_first_show) {
		// 初回表示の時だけ、UI全体を初期状態に合わせて完全に再描画し、初期設定を行う
		storingToolListstore.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH);				// テーブルのグリッド線を表示
		liststore3columnsAll.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH, true);			// テーブルのグリッド線を表示
		liststore2columnsAll.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH, true);			// テーブルのグリッド線を表示
		paginatedListstore2types1st.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH, true);	// テーブルのグリッド線を表示
		paginatedListstore2types2nd.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH, true);	// テーブルのグリッド線を表示
		warningListstore.set_grid_lines(GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_BOTH);					// テーブルのグリッド線を表示

		set_mode(GuiMode::NONE);	// 最初は取り出しモード
		set_app_state(AppState::WAITING_FOR_USER_LOGIN);	// 最初の状態を設定
		refresh_ui(true);
		m_is_first_show = false;	// フラグを倒して次回以降は実行しない
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
	set_text_mode(((m_text_mode == TextMode::KANJI) ? (TextMode::KANA) : (TextMode::KANJI)), true);
}

void MainWindow::logoutButton_on_clicked() {
	DEV_PRINT("ログアウトボタンがクリックされました\n");
	if (confirm_to_user(UiText{"ログアウトしますか？", "ログアウトしますか？"})) {
		DEV_PRINT("ログアウトします\n");
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
		messageLabel.remove_css_class(CSS_BGCOLOR_WARNING);	// メッセージラベル背景の警告色を解除

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
		messageLabel.add_css_class(CSS_BGCOLOR_WARNING);	// メッセージラベルの背景を警告色に
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

void MainWindow::warningListDialog_on_response(gint response_id) {
	if (static_cast<GtkResponseType>(response_id) == GtkResponseType::GTK_RESPONSE_CLOSE) {
		clear_warnings(true);
	}
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
		.text = UiText{"検出中です\nしばらくお待ちください", "けんしゅつちゅうです\nしばらく おまちください"}
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
	confirmInsertCountDialog.response(GtkResponseType::GTK_RESPONSE_OK);
}

void MainWindow::notRegroundButton_on_toggled() {
	if (m_is_programmatically_changing_toggle) { return; }	// プログラムによる変更は無視

	if (notRegroundButton.get_active()) {
		utils::scoped_value_guard guard(m_is_programmatically_changing_toggle, true);
		regroundButton.set_active(false); // 「はい」をオフにする
	}
}

void MainWindow::regroundButton_on_toggled() {
	if (m_is_programmatically_changing_toggle) { return; }	// プログラムによる変更は無視

	if (regroundButton.get_active()) {
		utils::scoped_value_guard guard(m_is_programmatically_changing_toggle, true);
		notRegroundButton.set_active(false);	// 「いいえ」をオフにする
	}
}

void MainWindow::warningListCloseButton_on_map() {
	auto* func = new std::function<void()>([this]() {
		this->warningListstore.refresh(true);
	});
	g_idle_add(MainWindow::dispatch_to_gui_thread, func);
}

void MainWindow::warningListCloseButton_on_clicked() {
	if (!m_warning_closed_by_user) {
		m_warning_closed_by_user = true;
		for (auto& val : m_operated_records) {
			val.track_result = TrackResult::OVERRIDDEN;
		}
	}
	warningListDialog.response(GtkResponseType::GTK_RESPONSE_CLOSE);
}

void MainWindow::warningListSuppressButton_on_clicked() {
	if (!m_warning_closed_by_user) {
		m_warning_closed_by_user = true;
		for (auto& val : m_operated_records) {
			val.track_result = TrackResult::OVERRIDDEN;
		}
	}
	m_ignore_all_warnings = true;
	warningListDialog.response(GtkResponseType::GTK_RESPONSE_CLOSE);
}

//=========================================================================
// プライベートメンバ関数 (状態毎の関数) 実装
//=========================================================================

void MainWindow::on_enter_waiting_for_user_login() {
	reset_variables();	// 変数を初期化
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
	update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);
}

void MainWindow::on_qrCodeEntry_retrieve_waiting_for_program_qr() {
	// QRコードからプログラム割当一覧を取得
	m_retrieving_list = db::get_ToolingListInfo(qrCodeEntry.get_text());

	// 取得成功時の処理(ToolingListInfoが存在する場合)
	if (m_retrieving_list.has_value()) {
		// m_retrieving_list.value()：オプションから実際のToolingListInfoを取得
		ToolingListInfo& tooling_list = m_retrieving_list.value();

		// ライン名とプログラム番号を表示
		lineEntry.set_text(tooling_list.line.name.c_str());
		programEntry.set_text(tooling_list.program.name.c_str());

		// ToolAssignmentInfo のリストを MainTableData のリストに変換して、
		// ページネーションモデルにセット (これで表示が更新される)
		set_tool_assignments(liststore2columnsAll, tooling_list.tool_assignments);
		update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);

/* BUG: テストデータ
for (int i = 0; i < 120; i++) {
	liststore2columnsAll.append(MainTableData{
		.id = i,
		.data_columns = {std::to_string(i), "", "", "", "", ""},
		.cell_state_0to4 = TableCellState::NORMAL
	});
}
//*/

		// メッセージラベル背景の警告色を解除
		messageLabel.remove_css_class(CSS_BGCOLOR_WARNING);

		// 次の状態へ遷移
		set_app_state(AppState::RETRIEVE_SELECTING_TOOLS);

	// 取得失敗時の処理
	} else {
		// 取得失敗時の処理 (エラーメッセージ表示など)
		DEV_PRINT("ツーリングリストの取得に失敗しました。\n");
		messageLabel.add_css_class(CSS_BGCOLOR_WARNING);	// メッセージラベルの背景を警告色に
		update_widget_state(&messageLabel, WidgetState{	// ラベルのテキストを更新
			.text = UiText{"該当するプログラムが見つかりません\nもう一度QRコードを読み取ってください", "がいとうする プログラムが みつかりません\nもういちど QRコードを よみとってください"}
		}, true);
	}
}

void MainWindow::on_enter_retrieve_selecting_tools() {
	update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);
}

void MainWindow::on_mainTableSelection_retrieve_selecting_tools(std::optional<size_t> selected_index_opt) {
	// 選択されていて、かつ無効化されていない行の場合
	if (selected_index_opt.has_value()) {
		// 状態をトグルして描画を更新
		liststore2columnsAll.modify_master_data([&selected_index_opt](std::vector<MainTableData>& master_data){
			auto& selected_item = master_data.at(selected_index_opt.value());

			DEV_PRINT("mainTableSelection_on_changed: Line {} is selected\n", selected_index_opt.value());

			if (selected_item.cell_state_0to4 != TableCellState::FADED) {
				// 状態をトグルする
				selected_item.cell_state_0to4 = ((selected_item.cell_state_0to4 == TableCellState::SELECTED)
					? (TableCellState::NORMAL)
					: (TableCellState::SELECTED)
				);
			}
		}, true);
	}

	// 選択された工具がある場合にのみ「確定」ボタンを有効化する
	update_widget_state(&trueButton, WidgetState{.is_sensitive = (liststore2columnsAll.find_first_index([](const MainTableData& item) {
		return item.cell_state_0to4 == TableCellState::SELECTED;
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

	// 「確定」ボタンを無効化
	update_widget_state(&trueButton, WidgetState{.is_sensitive = false}, true);
}

void MainWindow::on_enter_retrieve_confirming_selection() {
	// 選択状態の行を抽出
	auto vec = liststore2columnsAll.find_values([](const MainTableData& row) {
		return row.cell_state_0to4 == TableCellState::SELECTED;
	});

	// 全要素の色を通常色に戻す
	for (auto& row : vec) {
		row.cell_state_0to4 = TableCellState::NORMAL;
	}

	// 表のデータを抽出したものに置き換えつつ、現在のデータを退避
	m_retrieve_backup_table = liststore2columnsAll.replace_master_data(std::move(vec));
	update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);
}

void MainWindow::on_falseButton_retrieve_confirming_selection() {
	liststore2columnsAll.set_master_data(std::move(m_retrieve_backup_table));	// 退避していた表データを戻す
	update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);
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
	// 操作対象を取得
	const MstToolInfo& mst_tool_info = m_retrieving_tools.at(m_retrieving_index.value()).mst_tool_info;	// FIXME: nullopt対応

	// コンテナ搬出
	m_operating_area = db::get_StrageAreaInfo_with_max_cases(mst_tool_info.id);
	plc::retrieve_container(std::stoi(m_operating_area.value().mst_address_info.control_address));	// FIXME: nullopt対応	// FIXME: 例外処理
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_RETRIEVE_COMPLETED,
		.flags = plc::WatchFlags{
			.enable_rise_notification = true,
			.rise_is_one_shot = true
		}
	});

	// 追跡システムの開始
	// FIXME: エラー対応
	tracker::g_tool_tracker->connect();
	tracker::g_recorder_client->start_recording();

	// インサートの操作か判定
	m_is_insert_operation = (mst_tool_info.category == TOOL_CATEGORY_INSERT);

	// 表を作る
	std::vector<MainTableData> vec;
	vec.reserve(m_retrieving_tools.size());
	for (size_t i = 0; i < m_retrieving_tools.size(); i++) {
		// 現在の要素への参照を取得
		const auto& retrieving_tool = m_retrieving_tools[i];

		vec.push_back(MainTableData{
			.id = retrieving_tool.id,
			.data_columns = {
				retrieving_tool.tool_num,									// ツール番号
				retrieving_tool.mst_tool_info.category,						// 分類
				retrieving_tool.mst_tool_info.maker,						// メーカー
				retrieving_tool.mst_tool_info.name,							// 型番
				retrieving_tool.mst_tool_info.material,						// 材質
				std::to_string(retrieving_tool.mst_tool_info.stc.value()),	// BUG: nullopt対応 在庫数 (文字列に変換)
			},
			.cell_state_0to4 = ((i == m_retrieving_index)
				? (TableCellState::HIGHLIGHT)
				: (TableCellState::FADED)
			),
			.raw_data = retrieving_tool
		});
	}
	liststore2columnsAll.set_master_data(std::move(vec));
	update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);
}

void MainWindow::on_plc_retrieve_container_retrieving(const plc::Notification& notification) {
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {	// BUG: nullopt対応
		const plc::MonitorResult& result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_RETRIEVE_COMPLETED && result.status) {
			set_app_state(AppState::RETRIEVE_CONTAINER_RETRIEVED);
		} else {}	// BUG: エラー処理
	}
}

void MainWindow::on_enter_retrieve_container_retrieved() {
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_READY_TO_STORE,
		.flags = plc::WatchFlags{
			.enable_fall_notification = true,
			.fall_is_one_shot = true
		}
	});
}

void MainWindow::on_plc_retrieve_container_retrieved(const plc::Notification& notification) {
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {	// BUG: nullopt対応
		const plc::MonitorResult& result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_READY_TO_STORE && !result.status) {
			set_app_state(AppState::RETRIEVE_READY_FOR_READING);
		} else {}	// BUG: エラー処理
	}
}

void MainWindow::on_enter_retrieve_ready_for_reading() {
	// 状態変数を初期化
	m_plc_ready_for_action = false;		// 扉が閉まるのを待つため、最初は false

	// PLCの監視コマンドを登録
#ifdef STUB_PLC
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_READY_TO_STORE,
		.flags = plc::WatchFlags{
			.enable_rise_notification = true,
			.rise_is_one_shot = true,
		}
	});
#else
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_READY_TO_STORE,
		.flags = plc::WatchFlags{
			.enable_fall_notification = true,
			.enable_rise_notification = true,
			.fall_is_one_shot = false,
			.rise_is_one_shot = false
		}
	});
#endif

	liststore2columnsAll.clear();
	update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);
	qrCodeEntry.set_text("");
	qrCodeEntry.grab_focus();
}

// TODO: storage_condition の値に応じた警告表示を追加
void MainWindow::on_qrCodeEntry_retrieve_ready_for_reading() {
	// 既存の「スキャン系エラー」をすべて消す
	// (これらは「前回のスキャン結果」なので、新しいスキャンをした時点でもう不要)
	clear_scan_warnings(true);

	const MstToolInfo& retrieving_tool = m_retrieving_tools.at(m_retrieving_index.value()).mst_tool_info;	// BUG: nullopt対応
	std::optional<UniqueToolInfo> unique_info_opt = db::get_UniqueToolInfo(qrCodeEntry.get_text());

	if (unique_info_opt.has_value()) {
		UniqueToolInfo& unique_info = unique_info_opt.value();
		if (unique_info.mst_tool_info.id != retrieving_tool.id && unique_info.storage_area_info.has_value() && unique_info.storage_area_info.value().id != m_operating_area.value().id) {	// FIXME: nullopt対応
			add_warning(WarningType::DIFFERENT_TOOL, true);
			qrCodeEntry.grab_focus();
			return;
		}

		if (unique_info.storage_condition == StorageCondition::TAKEN_OUT) {
			add_warning(WarningType::ALREADY_RETRIEVED, true);
			qrCodeEntry.grab_focus();
			return;
		}

		if (unique_info.storage_condition == StorageCondition::NOT_STORED) {
			add_warning(WarningType::NOT_YET_STORED, true);
			qrCodeEntry.grab_focus();
			return;
		}

		tracker::g_tool_tracker->notify_qr_scan();
		m_pending_tool_info = std::move(unique_info);
	} else {
		add_warning(WarningType::TOOL_NOT_FOUND, true);
	}
	qrCodeEntry.grab_focus();
}

void MainWindow::on_falseButton_retrieve_ready_for_reading() {
	if (
		m_unscanned_count > 0 &&
		!override_confirm_to_user(UiText{
			"コンテナから取り出した後、\nQRコードを読み取っていない工具があります\n\n取り出す工具は、全てQRコードを読み取って下さい",
			"コンテナから とりだしたあと、\nQRコードを よみとっていない こうぐが あります\n\nとりだすこうぐは、すべてQRコードを よみとってください"
		})
	) {
		return;
	}
	if (
		m_operated_records.empty() &&
		!confirm_to_user(UiText{
			"1個も工具を取り出していませんが、\nコンテナを搬入してよろしいですか？",
			"1こも こうぐを とりだしていませんが、\nコンテナを はんにゅうして よろしいですか？"
		})
	) {
		return;
	}
	// BUG: エラー対応
	plc::store_container();
	plc::g_command_queue.push(plc::WatchCommand{.addr = plc::M_DEVICE_READY_TO_STORE, .flags = plc::WatchFlags{}});	// 監視対象を削除
	db::save_history(m_operated_records);
	tracker::g_tool_tracker->disconnect();
	tracker::g_recorder_client->stop_recording();
	set_mode(GuiMode::NONE);
}

void MainWindow::on_trueButton_retrieve_ready_for_reading() {
	if (
		m_unscanned_count > 0 &&
		!override_confirm_to_user(UiText{
			"コンテナから取り出した後、\nQRコードを読み取っていない工具があります\n\n取り出す工具は、全てQRコードを読み取って下さい",
			"コンテナから とりだしたあと、\nQRコードを よみとっていない こうぐが あります\n\nとりだすこうぐは、すべてQRコードを よみとってください"
		})
	) {
		return;
	}
	if (
		m_operated_records.empty() &&
		!confirm_to_user(UiText{
			"1個も工具を取り出していませんが、\nコンテナを搬入してよろしいですか？",
			"1こも こうぐを とりだしていませんが、\nコンテナを はんにゅうして よろしいですか？"
		})
	) {
		return;
	}
	// BUG: エラー対応
	plc::store_container();
	plc::g_command_queue.push(plc::WatchCommand{.addr = plc::M_DEVICE_READY_TO_STORE, .flags = plc::WatchFlags{}});	// 監視対象を削除
	db::save_history(m_operated_records);
	tracker::g_tool_tracker->disconnect();
	tracker::g_recorder_client->stop_recording();
	m_is_insert_operation = false;
	m_operating_area.reset();
	m_operated_records.clear();
	m_plc_ready_for_action = false;
	m_warning_closed_by_user = false;
	m_ignore_all_warnings = false;
	m_unscanned_count = 0;
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
			update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);
			set_app_state(AppState::RETRIEVE_SELECTING_TOOLS);
		} else {
			// 更新に失敗したら、プログラム割当一覧表読み取り画面に遷移
			set_app_state(AppState::RETRIEVE_WAITING_FOR_PROGRAM_QR);
		}
	}
}

void MainWindow::on_plc_retrieve_ready_for_reading(const plc::Notification& notification) {
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {	// BUG: nullopt対応
		auto result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_READY_TO_STORE) {
			// PLCの状態変数を更新 (status=true が格納準備完了状態)
			m_plc_ready_for_action = result.status;

			// 共通ヘルパーを呼び出して、3つの条件に基づきUIを更新
			update_retrieve_read_completed_buttons();
		} else {}	// BUG: エラー処理
	}
}

void MainWindow::on_tracker_retrieve_ready_for_reading(const tracker::ToolTrackerClient::Notification& notification) {
	bool result = false;
	const MstToolInfo& retrieving_tool = m_retrieving_tools.at(m_retrieving_index.value()).mst_tool_info;	// BUG: nullopt対応

	switch (notification.type) {
	case tracker::ToolTrackerClient::NotificationType::SCAN_SUCCESS: {
		if (m_pending_tool_info.has_value()) {
			UniqueToolInfo unique_info = std::move(m_pending_tool_info).value();
			m_pending_tool_info.reset();

			// 念のため基本工具情報等が合致しているかチェック
			if (
				unique_info.mst_tool_info.id != retrieving_tool.id &&
				unique_info.storage_area_info.has_value() && unique_info.storage_area_info.value().id != m_operating_area.value().id &&	// FIXME: nullopt対応
				unique_info.storage_condition != StorageCondition::TAKEN_OUT &&
				unique_info.storage_condition != StorageCondition::NOT_STORED
			) {
				DEV_PRINT(stderr, "on_tracker_retrieve_read_completed: m_pending_tool_info が不正値です\n");
				// BUG: 不正値対応
//				add_warning(WarningType::DIFFERENT_TOOL, true);
				result = false;
			} else {
				remove_warning(WarningType::SCAN_FAILURE, true, false);	// 強制削除
				remove_warning(WarningType::UNSCANNED_OUT_FRAME, false, true);

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
						confirmInsertCountDialog.run(this);	// m_insertCountEntry_current_count に新しい値が入る

						// 履歴ベクトル(m_operated_records)の個数を更新	// TODO: 表でのデータ有無確認後に更新するかこのままか考える
						it->tool_num = -m_insertCountEntry_current_count;
						it->proc_time = sg::now(); // 処理時刻を更新

						// 表(liststore2columnsAll)の個数を更新
						bool is_found = false;	// データを見つけたか: 基本的に true になるはず (m_operated_records と liststore は同期しているため)
						liststore2columnsAll.modify_master_data([&](std::vector<MainTableData>& data){
							// 表から該当する行を検索
							auto it = std::find_if(data.begin(), data.end(), [&](const MainTableData& item) {
								return item.id == unique_info.id;
							});

							if (it != data.end()) {
								// 見つかったら、6列目(個数)を更新
								it->data_columns[5] = std::to_string(m_insertCountEntry_current_count);

								// フラグを立てて外に知らせる
								is_found = true;
							}
							// ここではエラー処理をしない
						}, true);	// 第2引数を true にして、変更後に現在のページで更新させる

						// エラー処理
						if (!is_found) {
							// 基本的にここには来ないはず (m_operated_records と liststore は同期しているため)
							// BUG: 不正値対応
							DEV_PRINT(stderr, "CRITICAL: m_operated_records に ID {} がありましたが、liststore にはありませんでした。\n", unique_info.id);
						}
						m_insertCountEntry_current_count = 0;
					} else {
						// 2b. インサートでない場合: 何もしない
						DEV_PRINT("重複スキャン (非インサート): ID {} を無視します。\n", unique_info.id);
					}
				} else {	// 新規スキャン
					// 場にある未スキャン工具の数を減らす
					if (m_unscanned_count > 0) {
						m_unscanned_count--;
					}

					if (m_is_insert_operation) {
						update_widget_state(&confirmInsertCountLabel, WidgetState{.text = UiText{
							"使用するインサートの枚数を\n入力して下さい",
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
						.tool_num			= -((m_is_insert_operation)
							? (m_insertCountEntry_current_count)
							: (1)
						),
						.line_id			= m_retrieving_list.value().line.id,
						.track_result		= ((m_warning_closed_by_user)
							? (TrackResult::OVERRIDDEN)
							: (TrackResult::NORMAL)
						)
					});

					if (m_unscanned_count > 0) {
						m_unscanned_count--;
					}

					update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);

					m_insertCountEntry_current_count = 0;
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
	case tracker::ToolTrackerClient::NotificationType::SCAN_FAILURE: {
		add_warning(WarningType::SCAN_FAILURE, true);
		result = false;
		break;
	}
	case tracker::ToolTrackerClient::NotificationType::SCANNED_IN_CONTAINER: {
		add_warning(WarningType::SCANNED_IN_CONTAINER, true);
		result = false;
		break;
	}
	case tracker::ToolTrackerClient::NotificationType::SCANNED_OUT_CONTAINER: {
		remove_warning(WarningType::SCANNED_IN_CONTAINER, false, true);
		result = false;
		break;
	}
	case tracker::ToolTrackerClient::NotificationType::OTHER_IN_CONTAINER: {
		add_warning(WarningType::OTHER_IN_CONTAINER, true);
		result = false;
		break;
	}
	case tracker::ToolTrackerClient::NotificationType::OTHER_OUT_CONTAINER: {
		remove_warning(WarningType::OTHER_IN_CONTAINER, false, true);
		result = false;
		break;
	}
	case tracker::ToolTrackerClient::NotificationType::UNSCANNED_IN_CONTAINER: {
		if (m_unscanned_count > 0) {
			m_unscanned_count--;
		}
		if (m_unscanned_count <= 0) {
			remove_warning(WarningType::UNSCANNED_OUT_FRAME, true, true);	// TODO: 仲間にこれで良いか確認
		}
		result = false;
		break;
	}
	case tracker::ToolTrackerClient::NotificationType::UNSCANNED_OUT_CONTAINER: {
		m_unscanned_count++;
		result = false;
		break;
	}
	case tracker::ToolTrackerClient::NotificationType::UNSCANNED_OUT_FRAME: {
		add_warning(WarningType::UNSCANNED_OUT_FRAME, true);
		result = false;
		break;
	}
	// TODO: 不正対策
	default: {
		break;
	}
	}

	update_retrieve_read_completed_buttons();

	if (result) {
		set_app_state(AppState::RETRIEVE_READ_COMPLETED);
	}
	qrCodeEntry.grab_focus();
}

// --- 補充 ---

void MainWindow::on_enter_store_waiting_for_tool_qr() {
	// --- ウィジェット制御用 ---
	m_is_insert_operation = false;
	m_insertCountEntry_current_count = 0;
	m_confirmInsertCountDialog_max_count = -1;

	// --- 操作中の状態保持用 (複数場面共通) ---
	m_operating_area.reset();
	m_operating_tool.reset();
	m_operated_records.clear();
	m_plc_ready_for_action = false;

	// --- 操作中の状態保持用 (補充専用) ---
	m_reground_tools.clear();

	// --- 各表をリセット ---
	storingToolListstore.clear();
	storingToolListstore.append(MstToolInfo{});
	liststore2columnsAll.clear();
	update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);

	// --- エントリーを空に ---
	qrCodeEntry.set_text("");
	qrCodeEntry.grab_focus();
}

// TODO: storage_condition の値に応じた警告表示を追加
void MainWindow::on_qrCodeEntry_store_waiting_for_tool_qr() {
	auto unique_info_opt = db::get_UniqueToolInfo(qrCodeEntry.get_text());
	clear_scan_warnings(true);

	// 有効性チェック
	if (
		unique_info_opt.has_value() &&
		!unique_info_opt.value().mst_tool_info.is_frozen &&
		unique_info_opt.value().mst_tool_info.stc.has_value()
	) {
		auto& mst_tool_info = unique_info_opt.value().mst_tool_info;
		storingToolListstore.update(0, mst_tool_info);

		// インサートの操作か判定
		m_is_insert_operation = (mst_tool_info.category == TOOL_CATEGORY_INSERT);

		// 補充する工具の基本情報を保存
		m_operating_tool = std::move(mst_tool_info);

		set_app_state(AppState::STORE_CONFIRMING_SELECTION);
	} else {
		// TODO: 該当データが無い場合の処理
		add_warning(WarningType::TOOL_NOT_FOUND, true);
	}
}

void MainWindow::on_enter_store_container_retrieving() {
	m_operating_area = db::get_StrageAreaInfo_with_min_cases(m_operating_tool.value().id);	// FIXME: nullopt対応
	plc::retrieve_container(std::stoi(m_operating_area.value().mst_address_info.control_address));	// FIXME: nullopt対応	// FIXME: 例外処理
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_RETRIEVE_COMPLETED,
		.flags = plc::WatchFlags{
			.enable_rise_notification = true,
			.rise_is_one_shot = true
		}
	});

	// FIXME: エラー対応
	tracker::g_recorder_client->start_recording();
}

void MainWindow::on_plc_store_container_retrieving(const plc::Notification& notification) {
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {	// BUG: nullopt対応
		const plc::MonitorResult& result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_RETRIEVE_COMPLETED && result.status) {
			set_app_state(AppState::STORE_CONTAINER_RETRIEVED);
		} else {}	// BUG: エラー処理
	}
}

void MainWindow::on_enter_store_container_retrieved() {
	// TODO: 扉を開けるまで待機する処理を実装
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_READY_TO_STORE,
		.flags = plc::WatchFlags{
			.enable_fall_notification = true,
			.fall_is_one_shot = true
		}
	});
}

void MainWindow::on_plc_store_container_retrieved(const plc::Notification& notification) {
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {	// BUG: nullopt対応
		const plc::MonitorResult& result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_READY_TO_STORE && !result.status) {
			set_app_state(AppState::STORE_READY_FOR_READING);
		} else {}	// BUG: エラー処理
	}
}

void MainWindow::on_enter_store_ready_for_reading() {
	// 状態変数を初期化
	m_plc_ready_for_action = false;		// 扉が閉まるのを待つため、最初は false

	// PLCの監視コマンドを登録
#ifdef STUB_PLC
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_READY_TO_STORE,
		.flags = plc::WatchFlags{
			.enable_rise_notification = true,
			.rise_is_one_shot = true,
		}
	});
#else
	plc::g_command_queue.push(plc::WatchCommand{
		.addr = plc::M_DEVICE_READY_TO_STORE,
		.flags = plc::WatchFlags{
			.enable_fall_notification = true,
			.enable_rise_notification = true,
			.fall_is_one_shot = false,
			.rise_is_one_shot = false
		}
	});
#endif

	liststore2columnsAll.clear();
	update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);
	qrCodeEntry.set_text("");
	qrCodeEntry.grab_focus();
}

// TODO: storage_condition の値に応じた警告表示を追加
void MainWindow::on_qrCodeEntry_store_ready_for_reading() {
	struct info_and_reground{
		UniqueToolInfo unique_info;
		bool is_reground;
	};

	// 既存の「スキャン系エラー」をすべて消す
	// (これらは「前回のスキャン結果」なので、新しいスキャンをした時点でもう不要)
	clear_scan_warnings(true);

	bool result = false;
	bool is_reground = false;
	const MstToolInfo& operating_tool = m_operating_tool.value();	// BUG: nullopt対応
	std::optional<UniqueToolInfo> unique_info_opt = db::get_UniqueToolInfo(qrCodeEntry.get_text());

	if (unique_info_opt.has_value()) {
		UniqueToolInfo& unique_info = unique_info_opt.value();
		// 工具が合っているかチェック
		if (unique_info.mst_tool_info.id != operating_tool.id) {	// FIXME: nullopt対応
			add_warning(WarningType::DIFFERENT_TOOL, true);
			qrCodeEntry.grab_focus();
			return;
		}

		// データ整合性チェック
		if (unique_info.storage_condition == StorageCondition::STORED) {
			add_warning(WarningType::ALREADY_STORED, true);
			qrCodeEntry.grab_focus();
			return;
		}

		// この工具が既に補充リスト (m_operated_records) に存在するか検索
		const auto record_itr = std::find_if(m_operated_records.begin(), m_operated_records.end(),
			[&](const RecOperationTable& rec) {
				return rec.unique_tool_id.has_value() && rec.unique_tool_id.value() == unique_info.id;
			}
		);

		// 存在する場合 (重複スキャン)
		if (record_itr != m_operated_records.end()) {
			// 再補充工具の場合: 再研磨確認ダイアログを再表示
			if (!m_is_insert_operation && unique_info.storage_condition == StorageCondition::TAKEN_OUT) {

				// 履歴ベクトルを更新	// TODO: 表でのデータ有無確認後に更新するかこのままか考える
				record_itr->proc_time = sg::now(); // 処理時刻を更新

				// m_reground_tools に存在するか lower_bound で検索
				//	見つかった場合: その要素へのイテレータ
				//	見つからない場合: 「挿入すべき位置（次に大きい要素）」へのイテレータ
				auto set_itr = m_reground_tools.lower_bound(unique_info.id);

				// 存在判定
				//	it が end() でなく、かつ指している値が検索値と一致すれば「発見」
				bool set_found = (set_itr != m_reground_tools.end() && *set_itr == unique_info.id);

				// 再度再研磨工具か確認
				is_reground = confirm_if_reground(set_found);	// デフォルト値は前回の選択

				// m_reground_tools を更新
				if (is_reground) {
					if (!set_found) {
						// lower_bound で得た it は、「挿入すべき位置」を指す。
						// これをヒントとして渡すことで、再検索のコストがほぼゼロになる。
						m_reground_tools.emplace_hint(set_itr, unique_info.id);
					}
				} else if (set_found) {
					// 既に見つけているイテレータを使って削除
					m_reground_tools.erase(set_itr);
				}

				// 表(liststore2columnsAll)の個数を更新
				bool is_found = false;	// データを見つけたか: 基本的に true になるはず (m_operated_records と liststore は同期しているため)
				liststore2columnsAll.modify_master_data([&](std::vector<MainTableData>& data){
					// 表から該当する行を検索
					auto it = std::find_if(data.begin(), data.end(), [&](const MainTableData& item) {
						return item.id == unique_info.id;
					});

					if (it != data.end()) {
						// 見つかったら、6列目(個数)を更新
						it->data_columns[5] = std::to_string((unique_info.regrind_count.value_or(0) + ((is_reground) ? (1) : (0))));	// FIXME: nullopt対応

						// フラグを立てて外に知らせる
						is_found = true;
					}
					// ここではエラー処理をしない
				}, true);	// 第2引数を true にして、変更後に現在のページで更新させる

				// エラー処理
				if (!is_found) {
					// 基本的にここには来ないはず (m_operated_records と liststore は同期しているため)
					// BUG: 不正値対応
					DEV_PRINT(stderr, "CRITICAL: m_operated_records に ID {} がありましたが、liststore にはありませんでした。\n", unique_info.id);
				}
			}
		} else {	// 新規スキャン
			// 再研磨工具か確認
			if (!m_is_insert_operation && unique_info.storage_condition == StorageCondition::TAKEN_OUT) {
				is_reground = confirm_if_reground(true);
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
						? (unique_info.case_pack_num)
						: (unique_info.regrind_count.value_or(0) + ((is_reground) ? (1) : (0)))	// FIXME: nullopt対応
					))
				},
				.raw_data = unique_info
			});

			m_operated_records.push_back(RecOperationTable{
				.operation_class	= OperationClass::ADD,
				.user_id			= ((g_logged_in_user_info.has_value()) ? (g_logged_in_user_info.value().id) : (0)),	// FIXME: nullopt対応
				.proc_time			= sg::now(),
				.storage_area_id	= m_operating_area.value().id,	// BUG: nullopt対応
				.unique_tool_id		= unique_info.id,
				.video_path			= tracker::g_recorder_client->get_current_filename(),
				.tool_num			= ((unique_info.case_pack_num == 0)
					? (1)
					: (unique_info.case_pack_num)
				)
			});
			update_and_handle_pagination_2columns(PaginationAction::UPDATE_UI_ONLY, true);
		}
		result = true;
	} else {
		add_warning(WarningType::TOOL_NOT_FOUND, true);
	}
	qrCodeEntry.grab_focus();

	if (result) {
		set_app_state(AppState::STORE_READ_COMPLETED);
	}
}

void MainWindow::on_falseButton_store_ready_for_reading() {
	if (
		m_operated_records.empty() &&
		!confirm_to_user(UiText{
			"1個も工具を補充していませんが、\nコンテナを搬入してよろしいですか？",
			"1こも こうぐを ほじゅうしていませんが、\nコンテナを はんにゅうして よろしいですか？"
		})
	) {
		return;
	}
	// BUG: エラー対応
	plc::store_container();
	plc::g_command_queue.push(plc::WatchCommand{.addr = plc::M_DEVICE_READY_TO_STORE, .flags = plc::WatchFlags{}});	// 監視対象を削除
	db::increment_regrind_counts(m_reground_tools);
	db::save_history(m_operated_records);
	tracker::g_recorder_client->stop_recording();
	set_mode(GuiMode::NONE);
}

void MainWindow::on_trueButton_store_ready_for_reading() {
	if (
		m_operated_records.empty() &&
		!confirm_to_user(UiText{
			"1個も工具を補充していませんが、\nコンテナを搬入してよろしいですか？",
			"1こも こうぐを ほじゅうしていませんが、\nコンテナを はんにゅうして よろしいですか？"
		})
	) {
		return;
	}
	// BUG: エラー対応
	plc::store_container();
	plc::g_command_queue.push(plc::WatchCommand{.addr = plc::M_DEVICE_READY_TO_STORE, .flags = plc::WatchFlags{}});	// 監視対象を削除
	db::increment_regrind_counts(m_reground_tools);
	db::save_history(m_operated_records);
	tracker::g_recorder_client->stop_recording();
	set_app_state(AppState::STORE);
}

void MainWindow::on_plc_store_ready_for_reading(const plc::Notification& notification) {
	if (notification.type == plc::NotificationType::MONITOR && notification.monitor_result.has_value()) {	// BUG: nullopt対応
		auto result = notification.monitor_result.value();
		if (result.addr == plc::M_DEVICE_READY_TO_STORE) {
			update_widget_state({&falseButton, &trueButton}, WidgetState{.is_sensitive = result.status}, true);
		} else {}	// BUG: エラー処理
	}
}

// --- 棚卸し ---

void MainWindow::on_enter_reconcile_waiting_for_container_selection() {

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
