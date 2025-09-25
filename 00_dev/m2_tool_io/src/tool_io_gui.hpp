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
	// 公開メンバ定数 (enum) と 型定義 (struct)
	//=========================================================================
	/**
	 * @enum GuiMode
	 * @brief 現在の操作モードを定義する列挙型。
	 */
	enum class GuiMode {
		RETRIEVE,	// 取り出しモード
		STORE,		// 補充モード
		RECONCILE	// 棚卸しモード
	};

	/**
	 * @enum StaticWidgetId
	 * @brief 状態に依存しない静的なUI要素を識別するためのID。
	 */
	enum class StaticWidgetId {
		// モード切替ボタン
		RetrieveToggleButton,
		StoreToggleButton,
		ReconcileToggleButton,
		// ダイアログ内のラベルなど
		ConfirmInsertCountLabel,
		// TODO: 他に必要なウィジェットIDをここに追加
	};

	/**
	 * @enum AppState
	 * @brief アプリケーションの具体的な画面状態を示す列挙型クラス。
	 */
	enum class AppState {
		WAITING_FOR_USER_LOGIN,
		RETRIEVE_WAITING_FOR_PROGRAM_QR,
		RETRIEVE_SELECTING_TOOLS,
		RETRIEVE_CONFIRMING_SELECTION,
		// TODO: 他のモードの状態もここに追加
	};

	/**
	 * @enum TextMode
	 * @brief TextMode UIの表示言語モードを定義する列挙型。
	 */
	enum class TextMode { KANJI, KANA };

	/**
	 * @struct UiText
	 * @brief UI要素の かな/漢字 両方のテキストを定義する構造体。
	 */
	struct UiText {
		std::string kanji;
		std::string kana;
	};

	/**
	 * @struct StaticTextUpdater
	 * @brief 状態非依存のUI要素のテキストと、それを更新する関数を保持する構造体。
	 */
	struct StaticTextUpdater {
		UiText text;								// かな/漢字 両方のテキスト
		std::function<void(const char*)> setter;	// テキストを設定するためのセッター関数
	};

	/**
	 * @struct UiStateConfig
	 * @brief UiStateConfig 状態ごとのUI設定を完全に定義する構造体。
	 */
	struct UiStateConfig {
		// 表示テキスト関連
		UiText message;
		UiText trueButtonText;
		bool isTrueButtonSensitive;
		UiText falseButtonText;
		bool isFalseButtonSensitive;

		// 表示ページ関連
		std::string operationStackPageName;
		std::string tableStackPageName;

		// 実行する関数（コールバック）関連
		std::function<void(MainWindow*)> on_enter_state;
		std::function<void(MainWindow*)> on_true_button_clicked;
		std::function<void(MainWindow*)> on_false_button_clicked;
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
	sg::Label dateTimeLabel;
	/// @brief 操作モードを「取り出し」に切り替えるトグルボタン (ID: retrieveToggleButton)
	tool_io::ToggleButton retrieveToggleButton;
	/// @brief 操作モードを「補充」に切り替えるトグルボタン (ID: storeToggleButton)
	tool_io::ToggleButton storeToggleButton;
	/// @brief 操作モードを「棚卸し」に切り替えるトグルボタン (ID: reconcileToggleButton)
	tool_io::ToggleButton reconcileToggleButton;
	/// @brief かな/漢字入力を切り替えるボタン (ID: kanaKanjiButton)
	sg::Button kanaKanjiButton;
	/// @brief ログアウト処理を開始するボタン (ID: logoutButton)
	sg::Button logoutButton;
	/// @brief ログイン中のユーザーIDを表示/入力するエントリ (ID: userIdEntry)
	tool_io::Entry userIdEntry;

	// --- 中央操作部 ---
	/// @brief ユーザーへの指示などを表示するメッセージラベル (ID: messageLabel)
	sg::Label messageLabel;
	/// @brief "取り出し", "補充", "棚卸し"の各操作パネルを切り替えるスタック (ID: operationStack)
	tool_io::Stack operationStack;
	/// @brief (取り出し時) ライン番号を入力するエントリ (ID: lineEntry)
	tool_io::Entry lineEntry;
	/// @brief (取り出し時) プログラム番号を入力するエントリ (ID: programEntry)
	tool_io::Entry programEntry;
	/// @brief (補充時) 補充する工具の情報を表示するテーブル (ID: storingToolTable)
	sg::TreeView storingToolTable;
	/// @brief (棚卸し時) 絞り込みダイアログを開くボタン (ID: filterButton)
	sg::Button filterButton;
	/// @brief (棚卸し時) 現在操作中のコンテナ番号を表示するエントリ (ID: containerEntry)
	tool_io::Entry containerEntry;

	// --- テーブル表示部 ---
	/// @brief 複数のテーブル表示形式を切り替えるスタック (ID: table_stack)
	tool_io::Stack table_stack;
	/// @brief 3列表示テーブル (1) (ID: table3column1st)
	sg::TreeView table3column1st;
	/// @brief 3列表示テーブル (2) (ID: table3column2nd)
	sg::TreeView table3column2nd;
	/// @brief 3列表示テーブル (3) (ID: table3column3rd)
	sg::TreeView table3column3rd;
	/// @brief 2列表示テーブル (1) (ID: table2column1st)
	sg::TreeView table2column1st;
	/// @brief 2列表示テーブル (2) (ID: table2column2nd)
	sg::TreeView table2column2nd;

	// --- フッター部 ---
	/// @brief テーブルのページネーション(ページ送り)UIを格納するボックス (ID: paginationBox)
	tool_io::Box paginationBox;
	/// @brief 前のページへ移動するボタン (ID: prevButton)
	sg::Button prevButton;
	/// @brief 次のページへ移動するボタン (ID: nextButton)
	sg::Button nextButton;
	/// @brief 現在のページ番号を表示するラベル (ID: pageNumberLabel)
	sg::Label pageNumberLabel;
	/// @brief 肯定的なアクション（次へ, 確定など）を行うボタン (ID: trueButton)
	sg::Button trueButton;
	/// @brief 否定的なアクション（キャンセルなど）を行うボタン (ID: falseButton)
	sg::Button falseButton;

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
	sg::Label confirmInsertCountLabel;
	/// @brief 枚数を増やすボタン (ID: incrementButton)
	sg::Button insertCountIncrementButton;
	/// @brief 枚数を減らすボタン (ID: decrementButton)
	sg::Button insertCountDecrementButton;
	/// @brief 枚数を入力するエントリ (ID: countEntry)
	tool_io::Entry insertCountEntry;
	/// @brief 「OK」ボタン (ID: insertCountOkButton)
	sg::Button insertCountOkButton;

	// --- 再研磨確認ダイアログ内のウィジェット ---
	/// @brief ダイアログのメインメッセージラベル (ID: confirmRegrindLabel)
	sg::Label confirmRegrindLabel;
	/// @brief 「いいえ」トグルボタン (ID: notRegrindButton)
	tool_io::ToggleButton notRegrindButton;
	/// @brief 「はい」トグルボタン (ID: regrindButton)
	tool_io::ToggleButton regrindButton;
	/// @brief 「OK」ボタン (ID: confirmRegrindOkButton)
	sg::Button confirmRegrindOkButton;

	// --- ログアウト確認ダイアログ内のウィジェット ---
	/// @brief 「いいえ」ボタン (ID: logoutNoButton)
	sg::Button logoutNoButton;
	/// @brief 「はい」ボタン (ID: logoutYesButton)
	sg::Button logoutYesButton;

	// --- パスワード入力ダイアログ内のウィジェット ---
	/// @brief ダイアログのメインメッセージラベル (ID: passwordEntryLabel)
	sg::Label passwordEntryLabel;
	/// @brief 「キャンセル」ボタン (ID: passwordCancelButton)
	sg::Button passwordCancelButton;
	/// @brief 「確定」ボタン (ID: passwordEnterButton)
	sg::Button passwordEnterButton;
	/// @brief パスワード入力欄 (ID: passwordEntry)
	tool_io::Entry passwordEntry;

	// --- 警告ダイアログ内のウィジェット ---
	/// @brief 警告メッセージを表示するラベル (ID: warningLabel)
	sg::Label warningLabel;

	// --- 棚卸し絞り込みダイアログ内のウィジェット ---
	/// @brief 「分類」ラベル (ID: reconcileToolTypeFilterLabel)
	sg::Label reconcileToolTypeFilterLabel;
	/// @brief 「最終棚卸し日時」ラベル (ID: reconcilePeriodFilterLabel)
	sg::Label reconcilePeriodFilterLabel;
	/// @brief 「決定」ボタン (ID: reconcileFilterEnterButton)
	sg::Button reconcileFilterEnterButton;
	/// @brief 「期間リセット」ボタン (ID: reconcileFilterResetButton)
	sg::Button reconcileFilterResetButton;
	/// @brief 工具分類選択コンボボックス (ID: reconcileToolTypeFilterComboBox)
	tool_io::ComboBox reconcileToolTypeFilterComboBox;
	/// @brief 期間フィルタON/OFFスイッチ (ID: reconcilePeriodFilterSwitch)
	tool_io::Switch reconcilePeriodFilterSwitch;
	/// @brief 期間指定用 開始年増加ボタン (ID: reconcileFilterStartYearIncrementButton)
	sg::Button reconcileFilterStartYearIncrementButton;
	/// @brief 期間指定用 開始年入力エントリ (ID: reconcileFilterStartYearEntry)
	tool_io::Entry reconcileFilterStartYearEntry;
	/// @brief 期間指定用 開始年減少ボタン (ID: reconcileFilterStartYearDecrementButton)
	sg::Button reconcileFilterStartYearDecrementButton;
	/// @brief 期間指定用 開始月増加ボタン (ID: reconcileFilterStartMonthIncrementButton)
	sg::Button reconcileFilterStartMonthIncrementButton;
	/// @brief 期間指定用 開始月入力エントリ (ID: reconcileFilterStartMonthEntry)
	tool_io::Entry reconcileFilterStartMonthEntry;
	/// @brief 期間指定用 開始月減少ボタン (ID: reconcileFilterStartMonthDecrementButton)
	sg::Button reconcileFilterStartMonthDecrementButton;
	/// @brief 期間指定用 終了年増加ボタン (ID: reconcileFilterEndYearIncrementButton)
	sg::Button reconcileFilterEndYearIncrementButton;
	/// @brief 期間指定用 終了年入力エントリ (ID: reconcileFilterEndYearEntry)
	tool_io::Entry reconcileFilterEndYearEntry;
	/// @brief 期間指定用 終了年減少ボタン (ID: reconcileFilterEndYearDecrementButton)
	sg::Button reconcileFilterEndYearDecrementButton;
	/// @brief 期間指定用 終了月増加ボタン (ID: reconcileFilterEndMonthIncrementButton)
	sg::Button reconcileFilterEndMonthIncrementButton;
	/// @brief 期間指定用 終了月入力エントリ (ID: reconcileFilterEndMonthEntry)
	tool_io::Entry reconcileFilterEndMonthEntry;
	/// @brief 期間指定用 終了月減少ボタン (ID: reconcileFilterEndMonthDecrementButton)
	sg::Button reconcileFilterEndMonthDecrementButton;

	private:
	//=========================================================================
	// プライベートメンバ関数 (ヘルパー関数)
	//=========================================================================

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
	 * @brief 静的テキスト部分を更新する関数
	 * @details 状態に依存しないUI要素のテキストを、現在のテキストモードに応じて更新します。
	 */
	void refresh_static_texts();

	//=========================================================================
	// プライベートメンバ関数 (イベントハンドラ)
	//=========================================================================

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
	 * @brief trueButton の "clicked" シグナルに対応するハンドラ。
	 * @details 現在の状態に応じたコールバックを実行する。
	 */
	void trueButton_on_clicked();

	/**
	 * @brief falseButton の "clicked" シグナルに対応するハンドラ。
	 * @details 現在の状態に応じたコールバックを実行する。
	 */
	void falseButton_on_clicked();

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
	// プライベートメンバ変数 (状態管理用)
	//=========================================================================

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

	/// @brief 静的テキストのウィジェットとその更新情報を紐付ける辞書
	std::map<sg::Widget*, StaticTextUpdater> m_static_text_updaters;

	/**
	 * @brief 状態とUI設定を紐付ける "辞書" (ステートマシン定義)。
	 * @details アプリケーションの全状態の「見た目」と「振る舞い」をここで一元管理します。
	 */
	static const std::map<AppState, UiStateConfig> m_ui_state_map;
};