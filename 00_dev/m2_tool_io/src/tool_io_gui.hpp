/**
 * @file tool_io_gui.hpp
 * @brief アプリケーションのメインウィンドウクラスを定義したヘッダファイル。
 */
#pragma once
#include <SimpleGtk>
#include "./typedef.hpp"
#include "./parameters.hpp"
#include "./SimpleGtk_extension.hpp"
#include "./tracked_map.hpp"

/**
 * @class MainWindow
 * @brief アプリケーションのメインウィンドウクラス。
 * @details GladeファイルからUI定義を読み込み、ウィジェットの管理とイベント処理を担当します。
 */
struct MainWindow : public tool_io::Window {
public:
	//=========================================================================
	// 公開メンバ定数 (enum) と 型定義 (struct)
	//=========================================================================
	/**
	 * @enum GuiMode
	 * @brief 現在の操作モードを定義する列挙型。
	 */
	enum class GuiMode {
		NONE,		// 初期状態
		RETRIEVE,	// 取り出しモード
		STORE,		// 補充モード
		RECONCILE	// 棚卸しモード
	};

	/**
	 * @enum AppState
	 * @brief アプリケーションの具体的な画面状態を示す列挙型クラス。
	 */
	enum class AppState {
		/// ユーザーログイン待ち
		NONE,		// 移行用
		WAITING_FOR_USER_LOGIN,	// ユーザーQRコードの読み取り待ち

		/// 取り出しモード
		RETRIEVE,	// 移行用
		RETRIEVE_WAITING_FOR_PROGRAM_QR,	// TODO: 説明を書く
		RETRIEVE_SELECTING_TOOLS,			// TODO: 説明を書く
		RETRIEVE_CONFIRMING_SELECTION,		// TODO: 説明を書く

		/// 補充モード
		STORE,		// 移行用
		STORE_WAITING_FOR_TOOL_QR,		// 補充する工具のQRコード読み取り待ち
		STORE_CONFIRMING_SELECTION,		// 読み取った工具の確認
		STORE_CONTAINER_RETRIEVING,		// コンテナ搬出待ち
		STORE_CONTAINER_RETRIEVED,		// コンテナ搬出完了
		STORE_READY_FOR_READING,		// 実際に補充を行う(インサート以外) - 工具読み取り前
		STORE_READING_TOOL_QR,			// 実際に補充を行う(インサート以外) - 工具読み取り中
		STORE_READ_COMPLETED,			// 実際に補充を行う(インサート以外) - 工具読み取り後
		STORE_INSERT_READY_FOR_READING,	// 実際に補充を行う(インサート) - 工具読み取り前
		STORE_INSERT_READING_TOOL_QR,	// 実際に補充を行う(インサート) - 工具読み取り中
		STORE_INSERT_READ_COMPLETED,	// 実際に補充を行う(インサート) - 工具読み取り後

		/// 棚卸しモード
		RECONCILE,	// 移行用
		RECONCILE_WAITING_FOR_TOOL_SELECTION	// TODO: 説明を書く
		// TODO: 他のモードの状態もここに追加
	};

	/**
	 * @enum TextMode
	 * @brief TextMode UIの表示言語モードを定義する列挙型。
	 */
	enum class TextMode {
		/// @brief 漢字モード
		KANJI,
		/// @brief かなモード
		KANA
	};

	/**
	 * @struct UiText
	 * @brief UI要素の かな/漢字 両方のテキストを定義する構造体。
	 */
	struct UiText {
		/// @brief 漢字テキスト
		std::string kanji{};
		/// @brief かなテキスト
		std::string kana{};
	};

	/**
	 * @struct WidgetState
	 * @brief ウィジェット一式の表示状態を保持する構造体。
	 * @details std::optionalでラップされ、値を持たないメンバは「変更しない」ことを示す。
	 */
	struct WidgetState {
		/// @brief ウィジェットのテキスト (UiText)
		std::optional<UiText> text{};
		/// @brief ウィジェットの感応 (true: 有効, false: 無効)
		std::optional<bool> is_sensitive{};
		/// @brief ウィジェットの表示 (true: 表示, false: 非表示)
		std::optional<bool> is_visible{};

		/**
		 * @brief 他のWidgetStateの値でこのインスタンスを更新する。
		 * @param other 更新に使用するWidgetStateインスタンス。
		 * @return 変更があったかどうか (true: 変更あり, false: 変更なし)
		 */
		bool merge(const WidgetState& other);
	};

	/**
	 * @struct TableState
	 * @brief テーブルの表示状態を保持する構造体。
	 * @details std::optionalでラップされ、値を持たないメンバは「変更しない」ことを示す。
	 */
	struct TableState {
		/// @brief 列ヘッダーのテキストリスト (UiTextのvector)
		std::optional<std::vector<UiText>> column_headers{};
		/// @brief 列ヘッダーの感応 (true: 有効, false: 無効)
		std::optional<bool> is_sensitive{};
		/**
		 * @brief 列ヘッダーの表示 (true: 表示, false: 非表示)
		 * @details 使用は非推奨（gladeファイルの設計上、外側のコンテナごと非表示にしないと枠が消えないため）
		 */
		std::optional<bool> is_visible{};

		/**
		 * @brief 他のTableStateの値でこのインスタンスを更新する。
		 * @param other 更新に使用するTableStateインスタンス。
		 * @return 変更があったかどうか (true: 変更あり, false: 変更なし)
		 */
		bool merge(const TableState& other);
	};

	/**
	 * @struct WidgetStateUpdater
	 * @brief 状態非依存のUI要素のテキストと、それを更新する関数を保持する構造体。
	 */
	struct WidgetStateUpdater {
		/// @brief ウィジェットの状態
		WidgetState state{.is_sensitive = true, .is_visible = true};
		/// @brief ウィジェットの状態を設定するためのセッター関数
		std::function<void(const WidgetState&)> setter = nullptr;
	};

	/**
	 * @struct WidgetUpdateGroup
	 * @brief 複数のウィジェットに単一の状態設定を適用するためのグループ構造体
	 */
	struct WidgetUpdateGroup {
		/// @brief 対象ウィジェットのポインタリスト
		std::vector<sg::Widget*> widgets{};
		/// @brief 適用する状態
		WidgetState state{};
	};

	/**
	 * @struct TableUpdateGroup
	 * @brief 複数のテーブルに単一の状態設定を適用するためのグループ構造体
	 */
	struct TableUpdateGroup {
		/// @brief 対象テーブルのポインタリスト
		std::vector<tool_io::TreeView*> tables{};
		/// @brief 適用する状態
		TableState state{};
	};

	/**
	 * @struct WidgetStateConfig
	 * @brief 状態ごとのウィジェット設定を完全に定義する構造体。
	 */
	struct WidgetStateConfig {
		// --- UIの「見た目」を定義 ---

		/// @brief 各ウィジェットの状態
		std::vector<WidgetUpdateGroup> widget_states{};
		/// @brief 各テーブルの状態
		std::vector<TableUpdateGroup> table_states{};

		// --- 各スタックの「表示ページ」を定義 ---

		/// @brief operationStackの表示ページ名
		std::string operationStackPageName{};
		/// @brief table_stackの表示ページ名
		std::string tableStackPageName{};

		// --- 状態の「振る舞い」を定義 ---

		/// @brief 状態遷移直後に実行するアクション (nullptrなら何もしない)
		std::function<void(MainWindow*)> on_enter_state = nullptr;
		/// @brief qrCodeEntry がクリックされたときのアクション (nullptrなら何もしない)
		std::function<void(MainWindow*)> on_qrCodeEntry_activate = nullptr;
		/// @brief falseButton がクリックされたときのアクション (nullptrなら何もしない)
		std::function<void(MainWindow*)> on_falseButton_clicked = nullptr;
		/// @brief trueButton がクリックされたときのアクション (nullptrなら何もしない)
		std::function<void(MainWindow*)> on_trueButton_clicked = nullptr;
	};

	//=========================================================================
	// コンストラクタ
	//=========================================================================

	/**
	 * @brief MainWindowのコンストラクタ。
	 * @details ウィジェットの初期化とイベントハンドラの登録を行います。
	 */
	MainWindow();

	//=========================================================================
	// 公開メンバ関数
	//=========================================================================
	/**
	 * @brief アプリケーションの状態を更新し、UI（メッセージ、ボタン等）を再描画します。
	 * @param state 新しいアプリケーションの状態。
	 * @details 状態遷移の唯一の窓口となる関数です。
	 */
	void set_app_state(AppState state);

	//=========================================================================
	// 公開メンバ変数 (UIウィジェット)
	//=========================================================================

	/// @brief GladeファイルからUI定義を読み込むためのオブジェクト
	Glade glade;

	// --- ヘッダー部 ---
	/// @brief 日時を表示するラベル (ID: dateTimeLabel)
	tool_io::Label dateTimeLabel;
	/// @brief 操作モードを「取り出し」に切り替えるトグルボタン (ID: retrieveToggleButton)
	tool_io::ToggleButton retrieveToggleButton;
	/// @brief 操作モードを「補充」に切り替えるトグルボタン (ID: storeToggleButton)
	tool_io::ToggleButton storeToggleButton;
	/// @brief 操作モードを「棚卸し」に切り替えるトグルボタン (ID: reconcileToggleButton)
	tool_io::ToggleButton reconcileToggleButton;
	/// @brief かな/漢字入力を切り替えるボタン (ID: kanaKanjiButton)
	tool_io::Button kanaKanjiButton;
	/// @brief ログアウト処理を開始するボタン (ID: logoutButton)
	tool_io::Button logoutButton;
	/// @brief ログイン中のユーザーIDを表示/入力するエントリ (ID: userIdEntry)
	tool_io::Entry userIdEntry;

	// --- 中央操作部 ---
	/// @brief ユーザーへの指示などを表示するメッセージラベル (ID: messageLabel)
	tool_io::Label messageLabel;
	/// @brief "取り出し", "補充", "棚卸し"の各操作パネルを切り替えるスタック (ID: operationStack)
	tool_io::Stack operationStack;
	/// @brief (取り出し時) 「ライン番号：」のラベル (ID: lineLabel)
	tool_io::Label lineLabel;
	/// @brief (取り出し時) ライン番号を入力するエントリ (ID: lineEntry)
	tool_io::Entry lineEntry;
	/// @brief (取り出し時) 「プログラム番号：」のラベル (ID: programLabel)
	tool_io::Label programLabel;
	/// @brief (取り出し時) プログラム番号を入力するエントリ (ID: programEntry)
	tool_io::Entry programEntry;
	/// @brief (補充時) 補充する工具の情報を表示するテーブル (ID: storingToolTable)
	tool_io::TreeView storingToolTable;
	/// @brief (補充時) 補充する工具の情報を表示するテーブルのリストストア
	tool_io::TreeViewListStoreModel<ToolBasicInfo> storingToolListstore;
	/// @brief (棚卸し時) 絞り込みダイアログを開くボタン (ID: filterButton)
	tool_io::Button filterButton;
	/// @brief (棚卸し時) 「コンテナ番号：」のラベル (ID: containerLabel)
	tool_io::Label containerLabel;
	/// @brief (棚卸し時) 現在操作中のコンテナ番号を表示するエントリ (ID: containerEntry)
	tool_io::Entry containerEntry;

	// --- テーブル表示部 ---
	/// @brief 複数のテーブル表示形式を切り替えるスタック (ID: table_stack)
	tool_io::Stack table_stack;
	/// @brief 3列表示テーブルのボックス (1) (ID: table3column1stBox)
	tool_io::Box table3column1stBox;
	/// @brief 3列表示テーブル (1) (ID: table3column1st)
	tool_io::TreeView table3column1st;
	/// @brief 3列表示テーブルのリストストア (1)
	tool_io::TreeViewListStoreModel<MainTableData> Liststore3column1st;
	/// @brief 3列表示テーブルのボックス (2) (ID: table3column2ndBox)
	tool_io::Box table3column2ndBox;
	/// @brief 3列表示テーブル (2) (ID: table3column2nd)
	tool_io::TreeView table3column2nd;
	/// @brief 3列表示テーブルのリストストア (2)
	tool_io::TreeViewListStoreModel<MainTableData> Liststore3column2nd;
	/// @brief 3列表示テーブルのボックス (3) (ID: table3column3rdBox)
	tool_io::Box table3column3rdBox;
	/// @brief 3列表示テーブル (3) (ID: table3column3rd)
	tool_io::TreeView table3column3rd;
	/// @brief 3列表示テーブルのリストストア (3)
	tool_io::TreeViewListStoreModel<MainTableData> Liststore3column3rd;
	/// @brief 2列表示テーブルのボックス (1) (ID: table2column1stBox)
	tool_io::Box table2column1stBox;
	/// @brief 2列表示テーブル (1) (ID: table2column1st)
	tool_io::TreeView table2column1st;
	/// @brief 2列表示テーブルのリストストア (1)
	tool_io::TreeViewListStoreModel<MainTableData> Liststore2column1st;
	/// @brief 2列表示テーブルのボックス (2) (ID: table2column2ndBox)
	tool_io::Box table2column2ndBox;
	/// @brief 2列表示テーブル (2) (ID: table2column2nd)
	tool_io::TreeView table2column2nd;
	/// @brief 2列表示テーブルのリストストア (2)
	tool_io::TreeViewListStoreModel<MainTableData> Liststore2column2nd;

	// --- フッター部 ---
	/// @brief テーブルのページネーション(ページ送り)UIを格納するボックス (ID: paginationBox)
	tool_io::Box paginationBox;
	/// @brief 前のページへ移動するボタン (ID: prevButton)
	tool_io::Button prevButton;
	/// @brief 次のページへ移動するボタン (ID: nextButton)
	tool_io::Button nextButton;
	/// @brief 現在のページ番号を表示するラベル (ID: pageNumberLabel)
	tool_io::Label pageNumberLabel;
	/// @brief QRコード受付UIを格納するボックス (ID: qrCodeEntryBox)
	tool_io::Box qrCodeEntryBox;
	/// @brief 「QRコード受付：」のラベル (ID: qrCodeEntryLabel)
	tool_io::Label qrCodeEntryLabel;
	/// @brief QRコードを入力するエントリ (ID: qrCodeEntry)
	tool_io::Entry qrCodeEntry;
	/// @brief 否定的なアクション（キャンセルなど）を行うボタン (ID: falseButton)
	tool_io::Button falseButton;
	/// @brief 肯定的なアクション（次へ, 確定など）を行うボタン (ID: trueButton)
	tool_io::Button trueButton;

	// --- ダイアログの親ウィジェット ---
	/// @brief インサート枚数確認ダイアログ (ID: confirmInsertCountDialog)
	tool_io::Dialog confirmInsertCountDialog;
	/// @brief 再研磨確認ダイアログ (ID: confirmRegrindDialog)
	tool_io::Dialog confirmRegrindDialog;
	/// @brief ログアウト確認ダイアログ (ID: logoutDialog)
	tool_io::Dialog logoutDialog;
	/// @brief パスワード入力ダイアログ (ID: passwordEntryDialog)
	tool_io::Dialog passwordEntryDialog;
	/// @brief 警告表示ダイアログ (ID: warningDialog)
	tool_io::Dialog warningDialog;
	/// @brief 棚卸し絞り込みダイアログ (ID: reconcileFilterDialog)
	tool_io::Dialog reconcileFilterDialog;

	// --- インサート枚数確認ダイアログ内のウィジェット ---
	/// @brief ダイアログのメインメッセージラベル (ID: confirmInsertCountLabel)
	tool_io::Label confirmInsertCountLabel;
	/// @brief 枚数を増やすボタン (ID: incrementButton)
	tool_io::Button insertCountIncrementButton;
	/// @brief 枚数を減らすボタン (ID: decrementButton)
	tool_io::Button insertCountDecrementButton;
	/// @brief 枚数を入力するエントリ (ID: countEntry)
	tool_io::Entry insertCountEntry;
	/// @brief 「OK」ボタン (ID: insertCountOkButton)
	tool_io::Button insertCountOkButton;

	// --- 再研磨確認ダイアログ内のウィジェット ---
	/// @brief ダイアログのメインメッセージラベル (ID: confirmRegrindLabel)
	tool_io::Label confirmRegrindLabel;
	/// @brief 「いいえ」トグルボタン (ID: notRegrindButton)
	tool_io::ToggleButton notRegrindButton;
	/// @brief 「はい」トグルボタン (ID: regrindButton)
	tool_io::ToggleButton regrindButton;
	/// @brief 「OK」ボタン (ID: confirmRegrindOkButton)
	tool_io::Button confirmRegrindOkButton;

	// --- ログアウト確認ダイアログ内のウィジェット ---
	/// @brief 「いいえ」ボタン (ID: logoutNoButton)
	tool_io::Button logoutNoButton;
	/// @brief 「はい」ボタン (ID: logoutYesButton)
	tool_io::Button logoutYesButton;

	// --- パスワード入力ダイアログ内のウィジェット ---
	/// @brief ダイアログのメインメッセージラベル (ID: passwordEntryLabel)
	tool_io::Label passwordEntryLabel;
	/// @brief 「キャンセル」ボタン (ID: passwordCancelButton)
	tool_io::Button passwordCancelButton;
	/// @brief 「確定」ボタン (ID: passwordEnterButton)
	tool_io::Button passwordEnterButton;
	/// @brief パスワード入力欄 (ID: passwordEntry)
	tool_io::Entry passwordEntry;

	// --- 警告ダイアログ内のウィジェット ---
	/// @brief 警告メッセージを表示するラベル (ID: warningLabel)
	tool_io::Label warningLabel;

	// --- 棚卸し絞り込みダイアログ内のウィジェット ---
	/// @brief 「分類」ラベル (ID: reconcileToolTypeFilterLabel)
	tool_io::Label reconcileToolTypeFilterLabel;
	/// @brief 「最終棚卸し日時」ラベル (ID: reconcilePeriodFilterLabel)
	tool_io::Label reconcilePeriodFilterLabel;
	/// @brief 「決定」ボタン (ID: reconcileFilterEnterButton)
	tool_io::Button reconcileFilterEnterButton;
	/// @brief 「期間リセット」ボタン (ID: reconcileFilterResetButton)
	tool_io::Button reconcileFilterResetButton;
	/// @brief 工具分類選択コンボボックス (ID: reconcileToolTypeFilterComboBox)
	tool_io::ComboBox reconcileToolTypeFilterComboBox;
	/// @brief 期間フィルタON/OFFスイッチ (ID: reconcilePeriodFilterSwitch)
	tool_io::Switch reconcilePeriodFilterSwitch;
	/// @brief 期間指定用 開始年増加ボタン (ID: reconcileFilterStartYearIncrementButton)
	tool_io::Button reconcileFilterStartYearIncrementButton;
	/// @brief 期間指定用 開始年入力エントリ (ID: reconcileFilterStartYearEntry)
	tool_io::Entry reconcileFilterStartYearEntry;
	/// @brief 期間指定用 開始年減少ボタン (ID: reconcileFilterStartYearDecrementButton)
	tool_io::Button reconcileFilterStartYearDecrementButton;
	/// @brief 期間指定用 開始月増加ボタン (ID: reconcileFilterStartMonthIncrementButton)
	tool_io::Button reconcileFilterStartMonthIncrementButton;
	/// @brief 期間指定用 開始月入力エントリ (ID: reconcileFilterStartMonthEntry)
	tool_io::Entry reconcileFilterStartMonthEntry;
	/// @brief 期間指定用 開始月減少ボタン (ID: reconcileFilterStartMonthDecrementButton)
	tool_io::Button reconcileFilterStartMonthDecrementButton;
	/// @brief 期間指定用 終了年増加ボタン (ID: reconcileFilterEndYearIncrementButton)
	tool_io::Button reconcileFilterEndYearIncrementButton;
	/// @brief 期間指定用 終了年入力エントリ (ID: reconcileFilterEndYearEntry)
	tool_io::Entry reconcileFilterEndYearEntry;
	/// @brief 期間指定用 終了年減少ボタン (ID: reconcileFilterEndYearDecrementButton)
	tool_io::Button reconcileFilterEndYearDecrementButton;
	/// @brief 期間指定用 終了月増加ボタン (ID: reconcileFilterEndMonthIncrementButton)
	tool_io::Button reconcileFilterEndMonthIncrementButton;
	/// @brief 期間指定用 終了月入力エントリ (ID: reconcileFilterEndMonthEntry)
	tool_io::Entry reconcileFilterEndMonthEntry;
	/// @brief 期間指定用 終了月減少ボタン (ID: reconcileFilterEndMonthDecrementButton)
	tool_io::Button reconcileFilterEndMonthDecrementButton;

private:
	//=========================================================================
	// プライベートメンバ関数 (ヘルパー関数)
	//=========================================================================

	/**
	 * @brief 現在のテキストモードに応じて表示用の文字列を取得します。
	 * @param ui_text かな/漢字両方のテキストを持つUiText構造体。
	 * @return 表示用の文字列 (かな or 漢字)。
	 * @details m_text_mode メンバ変数に基づいて、ui_text の適切なフィールドを返します。
	 */
	const std::string& get_display_text(const UiText& ui_text) const {
		return (m_text_mode == TextMode::KANA)
				? ui_text.kana
				: ui_text.kanji;
	}

	/**
	 * @brief ウィジェットの状態を更新するヘルパー関数。
	 * @param widget 更新対象のウィジェット。
	 * @param state 適用する状態 (WidgetState)。
	 * @details state の各フィールドが std::nullopt でない場合にのみ、対応するウィジェットのプロパティを更新します。
	 */
	void update_widget_state(sg::Widget* widget, WidgetState state);

	/**
	 * @brief テーブルの状態を更新するヘルパー関数。
	 * @param table 更新対象のテーブルウィジェット。
	 * @param state 適用する状態 (TableState)。
	 * @details state の各フィールドが std::nullopt でない場合にのみ、対応するテーブルのプロパティを更新します。
	 */
	void update_table_state(tool_io::TreeView* table, TableState state);

	/**
	 * @brief アプリケーションの操作モードを設定します。
	 * @param mode 設定したい新しいモード (RETRIEVE, STORE, RECONCILE)。
	 * @details この関数がモードに対応する全てのUI状態（モード変数、ボタン排他、画面表示）を更新します。
	 */
	void set_mode(GuiMode mode);

	/**
	 * @brief パスワード認証ダイアログを表示し、認証を行います。
	 * @return bool 認証に成功した場合はtrue、失敗またはキャンセルの場合はfalseを返します。
	 */
	bool authenticate_with_password();

	/**
	 * @brief UI全体の表示を、モデルの現在の状態に応じて再描画する。
	 * @param force_redraw_all trueなら全ウィジェットを、falseなら変更があったウィジェットのみを更新。
	 */
	void refresh_ui(bool force_redraw_all = false);

	//=========================================================================
	// プライベートメンバ関数 (イベントハンドラ)
	//=========================================================================

	/**
	 * @brief ウィンドウが初回表示されたときに一度だけ呼び出されるハンドラ
	 */
	void on_window_show();

	/**
	 * @brief set_timeoutで設定された間隔ごとに呼び出されるタイマーハンドラ。
	 * @return bool trueを返すとタイマーが継続し、falseを返すと停止します。
	 */
	bool on_timeout() override;

	/**
	 * @brief retrieveToggleButton の "toggled" シグナルに対応するハンドラ。
	 * @details 「取り出し」モードへの切り替えを試みる。
	 */
	void retrieveToggleButton_on_toggled();

	/**
	 * @brief storeToggleButton の "toggled" シグナルに対応するハンドラ。
	 * @details 「補充」モードへの切り替えを試みる。
	 */
	void storeToggleButton_on_toggled();

	/**
	 * @brief reconcileToggleButton の "toggled" シグナルに対応するハンドラ。
	 * @details 「棚卸し」モードへの切り替えを試みる。
	 */
	void reconcileToggleButton_on_toggled();

	/**
	 * @brief kanaKanjiButton の "clicked" シグナルに対応するハンドラ。
	 * @details テキストモードを切り替え、UIを再描画する。
	 */
	void kanaKanjiButton_on_clicked();

	/**
	 * @brief logoutButton の "clicked" シグナルに対応するハンドラ。
	 * @details ログアウト確認ダイアログを表示する。
	 */
	void logoutButton_on_clicked();

	/**
	 * @brief userIdEntry の "activate" シグナルに対応するハンドラ。
	 * @details ユーザーIDの入力が完了したときに呼び出され、ユーザー認証を試みる。
	 */
	void userIdEntry_on_activate();

	/**
	 * @brief prevButton の "clicked" シグナルに対応するハンドラ。
	 * @details 前のページに移動する。
	 */
	void prevButton_on_clicked();

	/**
	 * @brief nextButton の "clicked" シグナルに対応するハンドラ。
	 * @details 次のページに移動する。
	 */
	void nextButton_on_clicked();

	/**
	 * @brief qrCodeEntry の "activate" シグナルに対応するハンドラ。
	 * @details 現在の状態に応じたコールバックを実行する。
	 */
	void qrCodeEntry_on_activate();

	/**
	 * @brief falseButton の "clicked" シグナルに対応するハンドラ。
	 * @details 現在の状態に応じたコールバックを実行する。
	 */
	void falseButton_on_clicked();

	/**
	 * @brief trueButton の "clicked" シグナルに対応するハンドラ。
	 * @details 現在の状態に応じたコールバックを実行する。
	 */
	void trueButton_on_clicked();

	/**
	 * @brief logoutNoButton の "clicked" シグナルに対応するハンドラ。
	 * @details logoutDialog で GTK_RESPONSE_NO を返す。
	 */
	void logoutNoButton_on_clicked();

	/**
	 * @brief logoutYesButton の "clicked" シグナルに対応するハンドラ。
	 * @details logoutDialog で GTK_RESPONSE_YES を返す。
	 */
	void logoutYesButton_on_clicked();

	/**
	 * @brief passwordEntryDialog の "show" シグナルに対応するハンドラ。
	 * @details ダイアログが表示された直後に呼び出され、パスワード入力欄にフォーカスを当てる。
	 */
	void passwordEntryDialog_on_show();

	/**
	 * @brief passwordEnterButton の "clicked" シグナルに対応するハンドラ。
	 * @details パスワード入力を確定して認証を試みる。
	 */
	void passwordEnterButton_on_clicked();

	/**
	 * @brief passwordCancelButton の "clicked" シグナルに対応するハンドラ。
	 * @details パスワード入力をキャンセルする。
	 */
	void passwordCancelButton_on_clicked();

	//=========================================================================
	// プライベートメンバ関数 (状態毎の関数)
	//=========================================================================

	/// @brief WAITING_FOR_USER_LOGIN 状態に入ったときの処理
	void on_enter_waiting_for_user_login();

	// --- 取り出しモードの状態遷移関数 ---
	// TODO: 取り出しモードの状態遷移関数をここに追加

	// --- 補充モードの状態遷移関数 ---
	/// @brief STORE_WAITING_FOR_TOOL_QR 状態に入ったときの処理
	void on_enter_store_waiting_for_tool_qr();
	/// @brief STORE_WAITING_FOR_TOOL_QR 状態の qrCodeEntry.on_activate の処理
	void on_qrCodeEntry_store_waiting_for_tool_qr();

	/// @brief STORE_CONTAINER_RETRIEVING 状態に入ったときの処理
	void on_enter_store_container_retrieving();

	/// @brief STORE_CONTAINER_RETRIEVED 状態に入ったときの処理
	void on_enter_store_container_retrieved();

	/// @brief STORE_READY_FOR_READING 状態に入ったときの処理
	void on_enter_store_ready_for_reading();
	/// @brief STORE_READY_FOR_READING 状態の qrCodeEntry.on_activate の処理
	void on_qrCodeEntry_store_ready_for_reading();

	/// @brief STORE_READING_TOOL_QR 状態に入ったときの処理
	void on_enter_store_reading_tool_qr();

	/// @brief STORE_READ_COMPLETED 状態に入ったときの処理
	void on_enter_store_read_completed();
	/// @brief STORE_WAITING_FOR_TOOL_QR 状態の falseButton.on_clicked の処理
	void on_falseButton_store_read_completed();
	/// @brief STORE_WAITING_FOR_TOOL_QR 状態の trueButton.on_clicked の処理
	void on_trueButton_store_read_completed();

	/// @brief STORE_INSERT_READY_FOR_READING 状態に入ったときの処理
	void on_enter_store_insert_ready_for_reading();

	/// @brief STORE_INSERT_READING_TOOL_QR 状態に入ったときの処理
	void on_enter_store_insert_reading_tool_qr();

	/// @brief STORE_INSERT_READ_COMPLETED 状態に入ったときの処理
	void on_enter_store_insert_read_completed();

	// --- 棚卸しモードの状態遷移関数 ---
	// TODO: 棚卸しモードの状態遷移関数をここに追加

	//=========================================================================
	// プライベートメンバ変数 (状態管理用)
	//=========================================================================

	/// @brief ウィンドウが初回表示かどうかを判定するフラグ
	bool m_is_first_show;

	/**
	 * @brief プログラムが内部的にトグルボタンの状態を変更しているかを示すフラグ。
	 * @details trueの間、toggledイベントハンドラは何もしません。
	 */
	bool m_isProgrammaticallyChangingToggle;

	/// @brief 現在のGUI操作モードを保持するメンバ変数。
	GuiMode m_gui_mode;
	/// @brief 現在の状態を保持するメンバ変数。
	AppState m_app_state;
	/// @brief 現在のテキストモード(かな/漢字)を保持するメンバ変数。
	TextMode m_text_mode;

	/// @brief ウィジェットの状態を保持し、変更があったウィジェットを追跡するマップ。
	utils::TrackedMap<sg::Widget*, WidgetStateUpdater> m_widget_states;

	/// @brief 各テーブルのカラムヘッダー表示テキストを保持するマップ。
	utils::TrackedMap<tool_io::TreeView*, TableState> m_table_states;

	/**
	 * @brief アプリケーション状態とウィジェット設定を紐付けるマップ。
	 * @details アプリケーションの全状態の「見た目」と「振る舞い」をここで一元管理します。
	 */
	std::map<AppState, WidgetStateConfig> g_app_state_configs;

};

// MainWindow::UiTextの比較演算子 (宣言)
bool operator==(const MainWindow::UiText& lhs, const MainWindow::UiText& rhs);
bool operator!=(const MainWindow::UiText& lhs, const MainWindow::UiText& rhs);

// MainWindow::WidgetStateの比較演算子 (宣言)
bool operator==(const MainWindow::WidgetState& lhs, const MainWindow::WidgetState& rhs);
bool operator!=(const MainWindow::WidgetState& lhs, const MainWindow::WidgetState& rhs);

// MainWindow::TableStateの比較演算子 (宣言)
bool operator==(const MainWindow::TableState& lhs, const MainWindow::TableState& rhs);
bool operator!=(const MainWindow::TableState& lhs, const MainWindow::TableState& rhs);