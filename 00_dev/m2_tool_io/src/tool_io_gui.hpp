/**
 * @file tool_io_gui.hpp
 * @brief アプリケーションのメインウィンドウクラスを定義したヘッダファイル。
 */
#pragma once
#include <cstdint>
#include <set>
#include <SimpleGtk>
#include "stdex/opt_extension.hpp"
#include "utils/tracked_map.hpp"
#include "gutils/gmain_utils.hpp"
#include "gutils/process_runner.hpp"
#include "SimpleGtk_extension.hpp"
#include "typedef.hpp"
#include "parameters.hpp"
#include "plc_communicator.hpp"
#include "tracking_communicator.hpp"

/**
 * @namespace gui
 * @brief GUIに関連するクラス,関数,定数を格納する名前空間。
 */
namespace gui {

struct MainWindow;

/// @brief グローバルな画面領域情報。
inline sgex::Rectangle g_monitor_area;

/// @brief アプリケーション全体で唯一のウィンドウインスタンスへのポインタ。
inline std::unique_ptr<gui::MainWindow> mainWindow;

/**
 * @struct MainWindow
 * @brief アプリケーションのメインウィンドウクラス。
 * @details GladeファイルからUI定義を読み込み、ウィジェットの管理とイベント処理を担当します。
 */
struct MainWindow : public sgex::Window {
public:
	//==========================================================================================
	// 公開メンバ定数 (enum) と 型定義 (struct) [PUBLIC_CONSTS_AND_TYPES]
	//==========================================================================================
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
		RETRIEVE_WAITING_FOR_PROGRAM_QR,	// プログラム割当一覧表のQRコード読み取り待ち
		RETRIEVE_SELECTING_TOOLS,			// 工具選択画面
		RETRIEVE_CONFIRMING_SELECTION,		// 選択した工具の確認
		RETRIEVE_CONTAINER_RETRIEVING,		// コンテナ搬出待ち
		RETRIEVE_CONTAINER_RETRIEVED,		// コンテナ搬出完了
		RETRIEVE_READY_FOR_READING,			// 実際に取り出しを行う - 工具読み取り前
		RETRIEVE_READ_COMPLETED,			// 実際に取り出しを行う - 工具読み取り後

		/// 補充モード
		STORE,		// 移行用
		STORE_WAITING_FOR_TOOL_QR,	// 補充する工具のQRコード読み取り待ち
		STORE_CONFIRMING_SELECTION,	// 読み取った工具の確認
		STORE_CONTAINER_RETRIEVING,	// コンテナ搬出待ち
		STORE_CONTAINER_RETRIEVED,	// コンテナ搬出完了
		STORE_READY_FOR_READING,	// 実際に補充を行う - 工具読み取り前
		STORE_READ_COMPLETED,		// 実際に補充を行う - 工具読み取り後

		/// 棚卸しモード
		RECONCILE,	// 移行用
		RECONCILE_WAITING_FOR_CONTAINER_SELECTION,	// コンテナ選択待ち
		RECONCILE_CONFIRMING_SELECTION,				// 選択したコンテナの確認
		RECONCILE_CONTAINER_RETRIEVING,				// コンテナ搬出待ち
		RECONCILE_CONTAINER_RETRIEVED,				// コンテナ搬出完了
		RECONCILE_READY_FOR_READING,				// 実際に棚卸しを行う(インサート以外) - 工具読み取り前
		RECONCILE_READ_COMPLETED,					// 実際に棚卸しを行う(インサート以外) - 工具読み取り後
		RECONCILE_INSERT_READY_FOR_READING,			// 実際に棚卸しを行う(インサート) - 工具読み取り前
		RECONCILE_INSERT_READ_COMPLETED,			// 実際に棚卸しを行う(インサート) - 工具読み取り後
	};

	/**
	 * @enum TextMode
	 * @brief TextMode UIの表示言語モードを定義する列挙型。
	 */
	enum class TextMode {
		KANJI,	// 漢字モード
		KANA	// かなモード
	};

	/**
	 * @enum PaginationAction
	 * @brief ページネーションUIの操作内容を定義する列挙型。
	 */
	enum class PaginationAction {
		UPDATE_UI_ONLY,	// ページは変更せず、UIの表示（ボタンの有効/無効など）だけを更新
		GO_TO_PREV,		// 前のページへ移動
		GO_TO_NEXT		// 次のページへ移動
	};

	/**
	 * @enum TableCellState
	 * @brief テーブルのセルの状態を定義する列挙型。
	 */
	enum class TableCellState {
		NONE,		// 変更なし
		NORMAL,		// 通常状態
		HIGHLIGHT,	// 強調表示
		FADED,		// 薄く表示
		SELECTED,	// 選択状態
		RECONCILE_MATCH,	// 棚卸しで一致した行
		RECONCILE_MISMATCH,	// 棚卸しで不一致の行
		RECONCILE_UNREAD	// 棚卸しで読み取りされなかった行
	};

	/**
	 * @struct MainTableData
	 * @brief メインテーブルの1行分のデータを保持する構造体。
	 */
	struct MainTableData {
		/// @brief ID
		std::int64_t id{};
		/// @brief 各列のデータ (6列固定)
		std::array<std::string, 6> data_columns{};
		/// @brief セルの状態 (0～4列目)
		TableCellState cell_state_0to4 = TableCellState::NORMAL;
		/// @brief セルの状態 (5列目)
		TableCellState cell_state_5 = TableCellState::NONE;
		/// @brief その行が表示している情報の元データ
		std::any raw_data{};
	};

	/**
	 * @enum WarningType
	 * @brief 警告の種類を定義する列挙型。
	 */
	enum class WarningType {
		NONE,						// 警告なし
		TOOL_NOT_FOUND,				// 未登録工具
		DIFFERENT_TOOL,				// 別の工具
		NOT_YET_STORED,				// 未だ補充前
		ALREADY_RETRIEVED,			// 既に取り出されていた
		ALREADY_STORED,				// 既に補充されていた
		RESTORE_RETRIEVED_INSERT,	// 一度取り出されたインサートを再補充しようとした
		SCAN_FAILURE,				// スキャン失敗
		UNSCANNED_OUT_FRAME,		// 未スキャン工具がフレーム外に出た
		SCANNED_IN_CONTAINER,		// スキャン済工具がコンテナ内に入った
		OTHER_IN_CONTAINER,			// その他工具がコンテナ内に入った
		END
	};

	/**
	 * @struct WarningListData
	 * @brief 警告一覧の1行分のデータを保持する構造体。
	 */
	struct WarningListData {
		/// @brief 警告番号
		WarningType type{};
		/// @brief その警告の発生件数
		int count{};
		/// @brief 行の状態
		TableCellState cell_state = TableCellState::NORMAL;
	};

	/**
	 * @struct UiText
	 * @brief UI要素の かな/漢字 両方のテキストを定義する構造体。
	 */
	struct UiText {
		using size_type = std::string::size_type;

		/// @brief 漢字テキスト
		std::string kanji{};
		/// @brief かなテキスト
		std::string kana{};

		/**
		 * @brief 両方のテキストの合計文字数を取得する。
		 * @return 合計文字数。
		 */
		size_type total_length() const {
			return kanji.length() + kana.length();
		}

		/**
		 * @brief 両方のテキストの最大文字数を取得する。
		 * @return 最大文字数。
		 */
		size_type max_length() const {
			return std::max(kanji.length(), kana.length());
		}

		/**
		 * @brief 両方のテキストの最小文字数を取得する。
		 * @return 最小文字数。
		 */
		size_type min_length() const {
			return std::min(kanji.length(), kana.length());
		}

		/**
		 * @brief 指定したモードのテキストの文字数を取得する。
		 * @param[in] mode 取得するテキストのモード。
		 * @return 指定したモードの文字数。
		 */
		size_type length_in_mode(TextMode mode) const noexcept {
			return (mode == TextMode::KANJI) ? kanji.length() : kana.length();
		}

		/**
		 * @brief 両方のテキストの合計文字数を取得する。
		 * @return 合計文字数。
		 */
		size_type length() const {
			return total_length();
		}

		/**
		 * @brief 両方のテキストの合計文字数を取得する。
		 * @return 合計文字数。
		 */
		size_type total_size() const {
			return total_length();
		}

		/**
		 * @brief 両方のテキストの最大文字数を取得する。
		 * @return 最大文字数。
		 */
		size_type max_size() const {
			return max_length();
		}

		/**
		 * @brief 両方のテキストの最小文字数を取得する。
		 * @return 最小文字数。
		 */
		size_type min_size() const {
			return min_length();
		}

		/**
		 * @brief 指定したモードのテキストの文字数を取得する。
		 * @param[in] mode 取得するテキストのモード。
		 * @return 指定したモードの文字数。
		 */
		size_type size_in_mode(TextMode mode) const noexcept {
			return length_in_mode(mode);
		}

		/**
		 * @brief 両方のテキストの合計文字数を取得する。
		 * @return 合計文字数。
		 */
		size_type size() const {
			return total_size();
		}

		/**
		 * @brief 両方のテキストが空かどうかを判定する。
		 * @return 両方のテキストが空の場合にtrue、それ以外の場合にfalseを返す。
		 */
		bool empty() const noexcept {
			return kanji.empty() && kana.empty();
		}

		/**
		 * @brief 両方のテキストをクリアする。
		 */
		void clear() noexcept {
			kanji.clear();
			kana.clear();
		}

		/**
		 * @brief 指定したモードのテキストを取得する。
		 * @param[in] mode 取得するテキストのモード。
		 * @return 指定したモードの文字列参照。
		 */
		const std::string& at(TextMode mode) const noexcept {
			return ((mode == TextMode::KANJI) ? (kanji) : (kana));
		}
	};

	/**
	 * @struct WidgetState
	 * @brief ウィジェット一式の表示状態を保持する構造体。
	 * @details std::optionalでラップされ、値を持たないメンバは「変更しない」ことを示す。
	 */
	struct WidgetState {
		/// @brief ウィジェットのテキスト (UiText)
		std::optional<UiText> text{};
		/// @brief ウィジェットのフォーカス可否 (true: 可能, false: 不可)
		std::optional<bool> can_focus{};
		/// @brief ウィジェットの感応 (true: 有効, false: 無効)
		std::optional<bool> is_sensitive{};
		/// @brief ウィジェットの表示 (true: 表示, false: 非表示)
		std::optional<bool> is_visible{};
		/// @brief エントリの編集可否 (GtkEntry限定, true: 可能, false: 不可)
		std::optional<bool> is_editable{};

		STDEX_DEFINE_MERGE_FUNC(WidgetState,
			&WidgetState::text,
			&WidgetState::can_focus,
			&WidgetState::is_sensitive,
			&WidgetState::is_visible,
			&WidgetState::is_editable
		)
	};

	/**
	 * @struct TableState
	 * @brief テーブルの表示状態を保持する構造体。
	 * @details std::optionalでラップされ、値を持たないメンバは「変更しない」ことを示す。
	 */
	struct TableState {
		/// @brief 列ヘッダーのテキストリスト (UiTextのvector)
		std::optional<std::vector<UiText>> column_headers{};
		/// @brief ツリービューの感応 (true: 有効, false: 無効)
		std::optional<bool> is_sensitive{};
		/// @brief ツリービューの表示 (true: 表示, false: 非表示)
		std::optional<bool> is_visible{};
		/// @brief ツリービューの選択可能性 (true: 選択可能, false: 選択不可)
		std::optional<bool> is_selectable{};

		STDEX_DEFINE_MERGE_FUNC(TableState,
			&TableState::column_headers,
			&TableState::is_sensitive,
			&TableState::is_visible,
			&TableState::is_selectable
		)
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
		std::vector<sgex::ListStoreViewInterface*> tables{};
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

		/// @brief 状態遷移時、UIリフレッシュの前に実行するカスタム初期化アクション
		std::function<void(MainWindow*)> on_prepare_state = nullptr;
		/// @brief 状態遷移直後に実行するアクション (nullptrなら何もしない)
		std::function<void(MainWindow*)> on_enter_state = nullptr;
		/// @brief いずれかのメインテーブルの選択が変更されたときのアクション (nullptrなら何もしない)
		std::function<void(MainWindow*, std::optional<size_t>)> on_mainTableSelection_changed = nullptr;
		/// @brief qrCodeEntry で Enter が押されたときのアクション (nullptrなら何もしない)
		std::function<void(MainWindow*)> on_qrCodeEntry_activate = nullptr;
		/// @brief falseButton がクリックされたときのアクション (nullptrなら何もしない)
		std::function<void(MainWindow*)> on_falseButton_clicked = nullptr;
		/// @brief trueButton がクリックされたときのアクション (nullptrなら何もしない)
		std::function<void(MainWindow*)> on_trueButton_clicked = nullptr;
		/// @brief PLCモニタースレッドから通知を受け取ったときのアクション (nullptrなら何もしない)
		std::function<void(MainWindow*, const plc::Notification&)> on_plc_notified = nullptr;
		/// @brief 工具追跡システムのリスナースレッドから通知を受け取ったときのアクション (nullptrなら何もしない)
		std::function<void(MainWindow*, const tracker::ToolTrackerClient::Notification&)> on_tracker_notified = nullptr;
	};

	//==========================================================================================
	// コンストラクタ [CONSTRUCTORS]
	//==========================================================================================

	/**
	 * @brief MainWindowのコンストラクタ。
	 * @details ウィジェットの初期化とイベントハンドラの登録を行います。
	 */
	MainWindow();

	//==========================================================================================
	// 公開メンバ関数 [PUBLIC_METHODS]
	//==========================================================================================

	/**
	 * @brief ウィンドウを強制的に閉じる。
	 */
	void force_close();

	/**
	 * @brief PLCモニタースレッドからタスク完了通知を受け取り、GUIスレッドで実行されるコールバック。
	 * @param[in] notification PLCモニタースレッドからの通知。
	 */
	void plc_on_notified(const plc::Notification& notification);

	/**
	 * @brief 工具追跡システムのリスナースレッドから通知を受け取り、GUIスレッドで実行されるコールバック。
	 * @param[in] notification 工具追跡システムのリスナースレッドからの通知。
	 */
	void tracker_on_notified(const tracker::ToolTrackerClient::Notification& notification);

	//==========================================================================================
	// 公開メンバ変数 (UIウィジェット) [PUBLIC_UI_WIDGETS]
	//==========================================================================================

	/// @brief GladeファイルからUI定義を読み込むためのオブジェクト
	Glade glade;

	// --- ヘッダー部 ---

	/// @brief 日時を表示するラベル (ID: dateTimeLabel)
	sgex::Label dateTimeLabel;
	/// @brief 操作モードを「取り出し」に切り替えるトグルボタン (ID: retrieveToggleButton)
	sgex::ToggleButton retrieveToggleButton;
	/// @brief 操作モードを「補充」に切り替えるトグルボタン (ID: storeToggleButton)
	sgex::ToggleButton storeToggleButton;
	/// @brief 操作モードを「棚卸し」に切り替えるトグルボタン (ID: reconcileToggleButton)
	sgex::ToggleButton reconcileToggleButton;
	/// @brief かな/漢字入力を切り替えるボタン (ID: kanaKanjiButton)
	sgex::Button kanaKanjiButton;
	/// @brief ログアウト処理を開始するボタン (ID: logoutButton)
	sgex::Button logoutButton;
	/// @brief ログイン中のユーザーIDを表示/入力するエントリ (ID: userIdEntry)
	sgex::Entry userIdEntry;

	// --- 中央操作部 ---

	/// @brief ユーザーへの指示などを表示するメッセージラベル (ID: messageLabel)
	sgex::Label messageLabel;
	/// @brief "取り出し", "補充", "棚卸し"の各操作パネルを切り替えるスタック (ID: operationStack)
	sgex::Stack operationStack;
	/// @brief (取り出し時) 「ライン番号：」のラベル (ID: lineLabel)
	sgex::Label lineLabel;
	/// @brief (取り出し時) ライン番号を入力するエントリ (ID: lineEntry)
	sgex::Entry lineEntry;
	/// @brief (取り出し時) 「プログラム番号：」のラベル (ID: programLabel)
	sgex::Label programLabel;
	/// @brief (取り出し時) プログラム番号を入力するエントリ (ID: programEntry)
	sgex::Entry programEntry;
	/// @brief (取り出し時) ツーリングリストを変更するボタン (ID: listChangeButton)
	sgex::Button listChangeButton;
	/// @brief (補充時) 補充する工具の情報を表示するテーブルのボックス (ID: storingToolTableBox)
	sgex::Box storingToolTableBox;
	/// @brief (補充時) 補充する工具の情報を表示するテーブル (ID: storingToolTable)
	sgex::TreeView storingToolTable;
	/// @brief (補充時) 補充する工具の情報を表示するテーブルのリストストア
	sgex::TreeViewListStoreModel<MstToolInfo> storingToolListstore;
	/// @brief (棚卸し時) 絞り込みダイアログを開くボタン (ID: filterButton)
	sgex::Button filterButton;
	/// @brief (棚卸し時) 「コンテナ番号：」のラベル (ID: containerLabel)
	sgex::Label containerLabel;
	/// @brief (棚卸し時) 現在操作中のコンテナ番号を表示するエントリ (ID: containerEntry)
	sgex::Entry containerEntry;

	// --- テーブル表示部 ---

	/// @brief 複数のテーブル表示形式を切り替えるスタック (ID: tableStack)
	sgex::Stack tableStack;
	/// @brief 3列表示テーブルのボックス (1) (ID: table3columns1stBox)
	sgex::Box table3columns1stBox;
	/// @brief 3列表示テーブル (1) (ID: table3columns1st)
	sgex::TreeView table3columns1st;
	/// @brief 3列表示テーブルのリストストア (1)
	sgex::TreeViewListStoreModel<MainTableData> liststore3columns1st;
	/// @brief 3列表示テーブルのボックス (2) (ID: table3columns2ndBox)
	sgex::Box table3columns2ndBox;
	/// @brief 3列表示テーブル (2) (ID: table3columns2nd)
	sgex::TreeView table3columns2nd;
	/// @brief 3列表示テーブルのリストストア (2)
	sgex::TreeViewListStoreModel<MainTableData> liststore3columns2nd;
	/// @brief 3列表示テーブルのボックス (3) (ID: table3columns3rdBox)
	sgex::Box table3columns3rdBox;
	/// @brief 3列表示テーブル (3) (ID: table3columns3rd)
	sgex::TreeView table3columns3rd;
	/// @brief 3列表示テーブルのリストストア (3)
	sgex::TreeViewListStoreModel<MainTableData> liststore3columns3rd;
	/// @brief 3列表示テーブルのページ操作可能なリストストア
	sgex::PaginatedListStoreModel<MainTableData> liststore3columnsAll;
	/// @brief 3列表示テーブルのページネーション(ページ送り)UIを格納するボックス (ID: paginationBox3columns)
	sgex::Box paginationBox3columns;
	/// @brief 3列表示テーブルの前のページへ移動するボタン (ID: prevButton3columns)
	sgex::Button prevButton3columns;
	/// @brief 3列表示テーブルの次のページへ移動するボタン (ID: nextButton3columns)
	sgex::Button nextButton3columns;
	/// @brief 3列表示テーブルの現在のページ番号を表示するラベル (ID: pageLabel3columns)
	sgex::Label pageLabel3columns;
	/// @brief 2列表示テーブルのボックス (1) (ID: table2columns1stBox)
	sgex::Box table2columns1stBox;
	/// @brief 2列表示テーブル (1) (ID: table2columns1st)
	sgex::TreeView table2columns1st;
	/// @brief 2列表示テーブルのリストストア (1)
	sgex::TreeViewListStoreModel<MainTableData> liststore2columns1st;
	/// @brief 2列表示テーブルのボックス (2) (ID: table2columns2ndBox)
	sgex::Box table2columns2ndBox;
	/// @brief 2列表示テーブル (2) (ID: table2columns2nd)
	sgex::TreeView table2columns2nd;
	/// @brief 2列表示テーブルのリストストア (2)
	sgex::TreeViewListStoreModel<MainTableData> liststore2columns2nd;
	/// @brief 2列表示テーブルのページ操作可能なリストストア
	sgex::PaginatedListStoreModel<MainTableData> liststore2columnsAll;
	/// @brief 2列表示テーブルのページネーション(ページ送り)UIを格納するボックス (ID: paginationBox2columns)
	sgex::Box paginationBox2columns;
	/// @brief 2列表示テーブルの前のページへ移動するボタン (ID: prevButton2columns)
	sgex::Button prevButton2columns;
	/// @brief 2列表示テーブルの次のページへ移動するボタン (ID: nextButton2columns)
	sgex::Button nextButton2columns;
	/// @brief 2列表示テーブルの現在のページ番号を表示するラベル (ID: pageLabel2columns)
	sgex::Label pageLabel2columns;
	/// @brief 2種表示テーブルのボックス (1種目) (ID: table2types1stBox)
	sgex::Box table2types1stBox;
	/// @brief 2種表示テーブル (1種目) (ID: table2types1st)
	sgex::TreeView table2types1st;
	/// @brief 2種表示テーブルのリストストア (1種目)
	sgex::TreeViewListStoreModel<MainTableData> liststore2types1st;
	/// @brief 2種表示テーブルのページ操作可能なリストストア (1種目)
	sgex::PaginatedListStoreModel<MainTableData> paginatedListstore2types1st;
	/// @brief 2種表示テーブルのページネーション(ページ送り)UIを格納するボックス (1種目) (ID: paginationBox2types1st)
	sgex::Box paginationBox2types1st;
	/// @brief 2種表示テーブルの前のページへ移動するボタン (1種目) (ID: prevButton2types1st)
	sgex::Button prevButton2types1st;
	/// @brief 2種表示テーブルの次のページへ移動するボタン (1種目) (ID: nextButton2types1st)
	sgex::Button nextButton2types1st;
	/// @brief 2種表示テーブルの現在のページ番号を表示するラベル (1種目) (ID: pageLabel2types1st)
	sgex::Label pageLabel2types1st;
	/// @brief 2種表示テーブルのボックス (2種目) (ID: table2types2ndBox)
	sgex::Box table2types2ndBox;
	/// @brief 2種表示テーブル (2種目) (ID: table2types2nd)
	sgex::TreeView table2types2nd;
	/// @brief 2種表示テーブルのリストストア (2種目)
	sgex::TreeViewListStoreModel<MainTableData> liststore2types2nd;
	/// @brief 2種表示テーブルのページ操作可能なリストストア (2種目)
	sgex::PaginatedListStoreModel<MainTableData> paginatedListstore2types2nd;
	/// @brief 2種表示テーブルのページネーション(ページ送り)UIを格納するボックス (2種目) (ID: paginationBox2types2nd)
	sgex::Box paginationBox2types2nd;
	/// @brief 2種表示テーブルの前のページへ移動するボタン (2種目) (ID: prevButton2types2nd)
	sgex::Button prevButton2types2nd;
	/// @brief 2種表示テーブルの次のページへ移動するボタン (2種目) (ID: nextButton2types2nd)
	sgex::Button nextButton2types2nd;
	/// @brief 2種表示テーブルの現在のページ番号を表示するラベル (2種目) (ID: pageLabel2types2nd)
	sgex::Label pageLabel2types2nd;

	// --- フッター部 ---

	/// @brief QRコード受付UIを格納するボックス (ID: qrCodeEntryBox)
	sgex::Box qrCodeEntryBox;
	/// @brief 「QRコード受付：」のラベル (ID: qrCodeEntryLabel)
	sgex::Label qrCodeEntryLabel;
	/// @brief QRコードを入力するエントリ (ID: qrCodeEntry)
	sgex::Entry qrCodeEntry;
	/// @brief 否定的なアクション（キャンセルなど）を行うボタン (ID: falseButton)
	sgex::Button falseButton;
	/// @brief 肯定的なアクション（次へ, 確定など）を行うボタン (ID: trueButton)
	sgex::Button trueButton;

	// --- ダイアログの親ウィジェット ---

	/// @brief インサート枚数確認ダイアログ (ID: confirmInsertCountDialog)
	sgex::Dialog confirmInsertCountDialog;
	/// @brief 再研磨確認ダイアログ (ID: confirmRegroundDialog)
	sgex::Dialog confirmRegroundDialog;
	/// @brief 確認ダイアログ (ID: confirmationDialog)
	sgex::Dialog confirmationDialog;
	/// @brief メッセージダイアログ (ID: messageDialog)
	sgex::Dialog messageDialog;
	/// @brief 強制実行確認ダイアログ (ID: overrideConfirmDialog)
	sgex::Dialog overrideConfirmDialog;
	/// @brief パスワード入力ダイアログ (ID: passwordEntryDialog)
	sgex::Dialog passwordEntryDialog;
	/// @brief 警告表示ダイアログ (ID: warningDialog)
	sgex::Dialog warningDialog;
	/// @brief 警告一覧ダイアログ (ID: warningListDialog)
	sgex::Dialog warningListDialog;
	/// @brief 棚卸し絞り込みダイアログ (ID: reconcileFilterDialog)
	sgex::Dialog reconcileFilterDialog;

	// --- インサート枚数確認ダイアログ内のウィジェット ---

	/// @brief ダイアログのメインメッセージラベル (ID: confirmInsertCountLabel)
	sgex::Label confirmInsertCountLabel;
	/// @brief 枚数を増やすボタン (ID: incrementButton)
	sgex::Button insertCountIncrementButton;
	/// @brief 枚数を減らすボタン (ID: decrementButton)
	sgex::Button insertCountDecrementButton;
	/// @brief 枚数を入力するエントリ (ID: countEntry)
	sgex::Entry insertCountEntry;
	/// @brief 個数自動検出ボタン (ID: insertCountDetectButton)
	sgex::Button insertCountDetectButton;
	/// @brief 「OK」ボタン (ID: insertCountOkButton)
	sgex::Button insertCountOkButton;

	// --- 再研磨確認ダイアログ内のウィジェット ---

	/// @brief ダイアログのメインメッセージラベル (ID: confirmRegroundLabel)
	sgex::Label confirmRegroundLabel;
	/// @brief 「いいえ」トグルボタン (ID: notRegroundButton)
	sgex::ToggleButton notRegroundButton;
	/// @brief 「はい」トグルボタン (ID: regroundButton)
	sgex::ToggleButton regroundButton;
	/// @brief 「OK」ボタン (ID: confirmRegroundOkButton)
	sgex::Button confirmRegroundOkButton;

	// --- 確認ダイアログ内のウィジェット ---

	/// @brief 確認メッセージを表示するラベル (ID: confirmationLabel)
	sgex::Label confirmationLabel;
	/// @brief 「いいえ」ボタン (ID: confirmationNoButton)
	sgex::Button confirmationNoButton;
	/// @brief 「はい」ボタン (ID: confirmationYesButton)
	sgex::Button confirmationYesButton;

	// --- メッセージダイアログ内のウィジェット ---

	/// @brief メッセージを表示するラベル (ID: messageDialogLabel)
	sgex::Label messageDialogLabel;

	// --- 強制実行確認ダイアログ内のウィジェット ---

	/// @brief 確認メッセージを表示するラベル (ID: overrideConfirmLabel)
	sgex::Label overrideConfirmLabel;
	/// @brief 「無視して続ける」ボタン (ID: overrideConfirmOverrideButton)
	sgex::Button overrideConfirmOverrideButton;
	/// @brief 「キャンセル」ボタン (ID: overrideConfirmCancelButton)
	sgex::Button overrideConfirmCancelButton;

	// --- パスワード入力ダイアログ内のウィジェット ---

	/// @brief ダイアログのメインメッセージラベル (ID: passwordEntryLabel)
	sgex::Label passwordEntryLabel;
	/// @brief パスワード入力欄 (ID: passwordEntry)
	sgex::Entry passwordEntry;
	/// @brief 「キャンセル」ボタン (ID: passwordCancelButton)
	sgex::Button passwordCancelButton;
	/// @brief 「確定」ボタン (ID: passwordEnterButton)
	sgex::Button passwordEnterButton;

	// --- 警告ダイアログ内のウィジェット ---

	/// @brief 警告メッセージを表示するラベル (ID: warningLabel)
	sgex::Label warningLabel;

	// --- 警告一覧ダイアログ内のウィジェット ---

	/// @brief 警告一覧のボックス (ID: warningListBox)
	sgex::Box warningListBox;
	/// @brief 警告一覧を表示するツリービュー (ID: warningList)
	sgex::TreeView warningList;
	/// @brief 警告一覧のリストストア
	sgex::TreeViewListStoreModel<WarningListData> warningListstore;
	/// @brief 「閉じる」ボタン (ID: warningListCloseButton)
	sgex::Button warningListCloseButton;
	/// @brief 「無視」ボタン (ID: warningListSuppressButton)
	sgex::Button warningListSuppressButton;

	// --- 棚卸し絞り込みダイアログ内のウィジェット ---

	/// @brief 「分類」ラベル (ID: reconcileToolTypeFilterLabel)
	sgex::Label reconcileToolTypeFilterLabel;
	/// @brief 「期間を指定」ラベル (ID: reconcilePeriodFilterLabel)
	sgex::Label reconcilePeriodFilterLabel;
	/// @brief 「決定」ボタン (ID: reconcileFilterEnterButton)
	sgex::Button reconcileFilterEnterButton;
	/// @brief 工具分類選択コンボボックス (ID: reconcileToolTypeFilterComboBox)
	sgex::ComboBox reconcileToolTypeFilterComboBox;
	/// @brief 工具分類選択コンボボックスのリストストア
	sgex::ComboBoxListStoreModel<std::string> reconcileToolTypeFilterComboBoxListstore;	// TODO: 型は自由に変更してください
	/// @brief 期間フィルタON/OFFスイッチ (ID: reconcilePeriodFilterSwitch)
	sgex::Switch reconcilePeriodFilterSwitch;
	/// @brief 期間指定用 「開始年月」ラベル (ID: reconcileFilterStartYearMonthLabel)
	sgex::Label reconcileFilterStartYearMonthLabel;
	/// @brief 期間指定用 開始年増加ボタン (ID: reconcileFilterStartYearIncrementButton)
	sgex::Button reconcileFilterStartYearIncrementButton;
	/// @brief 期間指定用 開始年入力エントリ (ID: reconcileFilterStartYearEntry)
	sgex::Entry reconcileFilterStartYearEntry;
	/// @brief 期間指定用 開始年減少ボタン (ID: reconcileFilterStartYearDecrementButton)
	sgex::Button reconcileFilterStartYearDecrementButton;
	/// @brief 期間指定用 開始年月の「年」ラベル (ID: reconcileFilterStartYearLabel)
	sgex::Label reconcileFilterStartYearLabel;
	/// @brief 期間指定用 開始月増加ボタン (ID: reconcileFilterStartMonthIncrementButton)
	sgex::Button reconcileFilterStartMonthIncrementButton;
	/// @brief 期間指定用 開始月入力エントリ (ID: reconcileFilterStartMonthEntry)
	sgex::Entry reconcileFilterStartMonthEntry;
	/// @brief 期間指定用 開始月減少ボタン (ID: reconcileFilterStartMonthDecrementButton)
	sgex::Button reconcileFilterStartMonthDecrementButton;
	/// @brief 期間指定用 開始年月の「月」ラベル (ID: reconcileFilterStartMonthLabel)
	sgex::Label reconcileFilterStartMonthLabel;
	/// @brief 期間指定用 「終了年月」ラベル (ID: reconcileFilterEndYearMonthLabel)
	sgex::Label reconcileFilterEndYearMonthLabel;
	/// @brief 期間指定用 終了年増加ボタン (ID: reconcileFilterEndYearIncrementButton)
	sgex::Button reconcileFilterEndYearIncrementButton;
	/// @brief 期間指定用 終了年入力エントリ (ID: reconcileFilterEndYearEntry)
	sgex::Entry reconcileFilterEndYearEntry;
	/// @brief 期間指定用 終了年減少ボタン (ID: reconcileFilterEndYearDecrementButton)
	sgex::Button reconcileFilterEndYearDecrementButton;
	/// @brief 期間指定用 終了年月の「年」ラベル (ID: reconcileFilterEndYearLabel)
	sgex::Label reconcileFilterEndYearLabel;
	/// @brief 期間指定用 終了月増加ボタン (ID: reconcileFilterEndMonthIncrementButton)
	sgex::Button reconcileFilterEndMonthIncrementButton;
	/// @brief 期間指定用 終了月入力エントリ (ID: reconcileFilterEndMonthEntry)
	sgex::Entry reconcileFilterEndMonthEntry;
	/// @brief 期間指定用 終了月減少ボタン (ID: reconcileFilterEndMonthDecrementButton)
	sgex::Button reconcileFilterEndMonthDecrementButton;
	/// @brief 期間指定用 終了年月の「月」ラベル (ID: reconcileFilterEndMonthLabel)
	sgex::Label reconcileFilterEndMonthLabel;

private:
	//==========================================================================================
	// プライベートメンバ関数 (ヘルパー関数) [PRIVATE_HELPERS]
	//==========================================================================================

	// --- 複数場面共通のヘルパー関数 ---

	/**
	 * @brief 現在のテキストモードに応じて表示用の文字列を取得します。
	 * @details m_text_mode メンバ変数に基づいて、ui_text の適切なフィールドを返します。
	 * @param[in] ui_text かな/漢字両方のテキストを持つUiText構造体。
	 * @return 表示用の文字列 (かな or 漢字)。
	 */
	const std::string& get_display_text(const UiText& ui_text) const {
		return ui_text.at(m_text_mode);
	}

	/**
	 * @brief 状態管理用の変数を初期状態にリセットする変数。
	 * @details GUI操作モードの変更時に呼び出されます。
	 */
	void reset_variables();

	/**
	 * @brief ヘッダーのボタン群を初期状態にリセットする変数。
	 * @details GUI操作モードの変更時に呼び出されます。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 */
	void reset_header_buttons(bool update_ui = true);

	/**
	 * @brief ウィジェットの状態を更新するヘルパー関数。
	 * @details state の各フィールドが std::nullopt でない場合にのみ、対応するウィジェットのプロパティを更新します。
	 * @param[in] widget 更新対象のウィジェット。
	 * @param[in] state 適用する状態 (WidgetState)。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 * @return ウィジェットの状態が更新された場合は true、そうでない場合は false。
	 */
	bool update_widget_state(sg::Widget* widget, const WidgetState& state, bool update_ui = true);

	/**
	 * @brief ウィジェットの状態をムーブ更新するヘルパー関数。
	 * @details state の各フィールドが std::nullopt でない場合にのみ、対応するウィジェットのプロパティを更新します。
	 * @param[in] widget 更新対象のウィジェット。
	 * @param[in] state ムーブ適用する状態 (WidgetState)。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 * @return ウィジェットの状態が更新された場合は true、そうでない場合は false。
	 */
	bool update_widget_state(sg::Widget* widget, WidgetState&& state, bool update_ui = true);

	/**
	 * @brief 複数のウィジェットの状態を更新するヘルパー関数。
	 * @details state の各フィールドが std::nullopt でない場合にのみ、対応するウィジェットのプロパティを更新します。
	 * @param[in] widgets 更新対象のウィジェットのリスト。
	 * @param[in] state 適用する状態 (WidgetState)。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 * @return いずれかのウィジェットの状態が更新された場合は true、全てのウィジェットの状態が更新されなかった場合は false。
	 */
	bool update_widget_state(const std::vector<sg::Widget*>& widgets, const WidgetState& state, bool update_ui = true);

	/**
	 * @brief テーブルの状態を更新するヘルパー関数。
	 * @details state の各フィールドが std::nullopt でない場合にのみ、対応するテーブルのプロパティを更新します。
	 * @param[in] table 更新対象のテーブルウィジェット。
	 * @param[in] state 適用する状態 (TableState)。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 * @return テーブルの状態が更新された場合は true、そうでない場合は false。
	 */
	bool update_table_state(sgex::ListStoreViewInterface* table, const TableState& state, bool update_ui = true);

	/**
	 * @brief テーブルの状態をムーブ更新するヘルパー関数。
	 * @details state の各フィールドが std::nullopt でない場合にのみ、対応するテーブルのプロパティを更新します。
	 * @param[in] table 更新対象のテーブルウィジェット。
	 * @param[in] state ムーブ適用する状態 (TableState)。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 * @return テーブルの状態が更新された場合は true、そうでない場合は false。
	 */
	bool update_table_state(sgex::ListStoreViewInterface* table, TableState&& state, bool update_ui = true);

	/**
	 * @brief 複数のテーブルの状態を更新するヘルパー関数。
	 * @details state の各フィールドが std::nullopt でない場合にのみ、対応するテーブルのプロパティを更新します。
	 * @param[in] tables 更新対象のテーブルウィジェットのリスト。
	 * @param[in] state 適用する状態 (TableState)。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 * @return いずれかのテーブルの状態が更新された場合は true、全てのテーブルの状態が更新されなかった場合は false。
	 */
	bool update_table_state(const std::vector<sgex::ListStoreViewInterface*>& tables, const TableState& state, bool update_ui = true);

	/**
	 * @brief UI全体の表示を、モデルの現在の状態に応じて再描画する。
	 * @param[in] force_redraw_all trueなら全ウィジェットを、falseなら変更があったウィジェットのみを更新。
	 */
	void refresh_ui(bool force_redraw_all = false);

	/**
	 * @brief アプリケーションの状態を更新し、UI（メッセージ、ボタン等）を再描画します。
	 * @details 状態遷移の唯一の窓口となる関数です。
	 * @param[in] state 新しいアプリケーションの状態。
	 */
	void set_app_state(AppState state);

	/**
	 * @brief アプリケーションの操作モードを設定します。
	 * @details この関数がモードに対応する全てのUI状態（モード変数、ボタン排他、画面表示）を更新します。
	 * @param[in] mode 設定したい新しいモード (RETRIEVE, STORE, RECONCILE)。
	 */
	void set_mode(GuiMode mode);

	/**
	 * @brief かな/漢字入力モードを設定します。
	 * @param[in] text_mode 設定したい新しいテキストモード (KANA, KANJI)。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 */
	void set_text_mode(TextMode text_mode, bool update_ui = true);

	/**
	 * @brief MainTableData構造体の内容をGtkListStoreに設定するセッター関数。
	 * @param[out] store 更新対象の sgex::ListStore。
	 * @param[out] iter 更新する行のイテレータ。
	 * @param[in] row 設定する MainTableData 構造体。
	 */
	void main_ListStore_setter(sgex::ListStore& store, GtkTreeIter* iter, const MainTableData& row);

	/**
	 * @brief ページネーションのUI更新と操作をまとめて行うヘルパー関数（ページ自動判別用）。
	 * @details 現在表示中のページに応じて、適切なヘルパー関数を呼び出します。
	 *
	 * @warning [使用上の注意]
	 * 互換性のために、PaginationAction を自由に指定できるようにしていますが、
	 * 基本的に PaginationAction::UPDATE_UI_ONLY だけを使うことを想定しています。
	 * なぜなら、2種類表示ページで両方の表が同時にページ遷移するのはおかしいためです。
	 * また、2種類表示ページで、左右で項目数が異なる場合、この関数で同時にページ遷移すると、
	 * ページ番号が左右でずれてしまう可能性があります。
	 *
	 * @param[in] action (オプション・デフォルト UPDATE_UI_ONLY) 実行したい操作の種類。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 */
	void update_and_handle_pagination(PaginationAction action = PaginationAction::UPDATE_UI_ONLY, bool update_ui = true);

	/**
	 * @brief ページネーションのUI更新と操作をまとめて行うヘルパー関数（3段表示ページ用）。
	 * @param[in] action 実行したい操作の種類。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 */
	void update_and_handle_pagination_3columns(PaginationAction action, bool update_ui = true);

	/**
	 * @brief ページネーションのUI更新と操作をまとめて行うヘルパー関数（2段表示ページ用）。
	 * @param[in] action 実行したい操作の種類。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 */
	void update_and_handle_pagination_2columns(PaginationAction action, bool update_ui = true);

	/**
	 * @brief ページネーションのUI更新と操作をまとめて行うヘルパー関数（2種表示ページの1種目用）。
	 * @param[in] action 実行したい操作の種類。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 */
	void update_and_handle_pagination_2types1st(PaginationAction action, bool update_ui = true);

	/**
	 * @brief ページネーションのUI更新と操作をまとめて行うヘルパー関数（2種表示ページの2種目用）。
	 * @param[in] action 実行したい操作の種類。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 */
	void update_and_handle_pagination_2types2nd(PaginationAction action, bool update_ui = true);

	/**
	 * @brief 確認ダイアログを用いて、ユーザーに確認を取るヘルパー関数。
	 * @param[in] message ダイアログに設定するメッセージ。
	 * @return ユーザーが「はい」を選択した場合はtrue、「いいえ」を選択した場合はfalseを返します。
	 */
	bool confirm_to_user(const UiText& message);

	/**
	 * @brief 確認ダイアログを用いて、ユーザーに確認を取るヘルパー関数。(ムーブ版)
	 * @param[in] message ダイアログにムーブ設定するメッセージ。
	 * @return ユーザーが「はい」を選択した場合はtrue、「いいえ」を選択した場合はfalseを返します。
	 */
	bool confirm_to_user(UiText&& message);

	/**
	 * @brief 強制実行確認ダイアログを用いて、ユーザーに確認を取るヘルパー関数。
	 * @param[in] message ダイアログに設定するメッセージ。
	 * @return ユーザーが「無視して続ける」を選択した場合はtrue、「キャンセル」を選択した場合はfalseを返します。
	 */
	bool override_confirm_to_user(const UiText& message);

	/**
	 * @brief 強制実行確認ダイアログを用いて、ユーザーに確認を取るヘルパー関数。(ムーブ版)
	 * @param[in] message ダイアログにムーブ設定するメッセージ。
	 * @return ユーザーが「無視して続ける」を選択した場合はtrue、「キャンセル」を選択した場合はfalseを返します。
	 */
	bool override_confirm_to_user(UiText&& message);

	/**
	 * @brief インサート枚数確認ダイアログを非同期に表示します。
	 * @param[in] callback ダイアログが閉じた後に呼び出されるコールバック関数。
	 */
	void confirm_insert_count(int init_count, int max_count, std::function<void(int)> callback);

	/**
	 * @brief 現在の状態に応じた update_**_read_completed_buttons を呼び出す。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 */
	void update_read_completed_buttons(bool update_ui = true);

	// --- 警告管理用のヘルパー関数 ---

	/**
	 * @brief 指定した警告の種類が、状態に対する警告かを確認します。
	 * @param[in] type 確認する警告の種類。
	 * @return 状態に対する警告であればtrue、そうでなければfalseを返します。
	 */
	static bool is_state_warning(WarningType type) {
		switch (type) {
		case WarningType::UNSCANNED_OUT_FRAME:
		case WarningType::SCANNED_IN_CONTAINER:
		case WarningType::OTHER_IN_CONTAINER:
			return true;	// ダイアログを出すべき重要警告
		default:
			return false;	// メッセージラベルで済ませるスキャン警告
		}
	}

	/**
	 * @brief 警告表示を更新します。
	 * @details 現在の警告状態に基づいて、メッセージラベルで済ませるか警告ダイアログで表示するかを判定し、内容を更新します。
	 * @return 警告ダイアログを表示した場合はtrue、メッセージラベルで済ませた場合はfalseを返します。
	 */
	bool update_warning_display();

	/**
	 * @brief 警告を追加します。
	 * @param[in] type 追加する警告の種類。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 */
	void add_warning(WarningType type, bool update_ui = true);

	/**
	 * @brief 警告を削除します。
	 * @param[in] type 削除する警告の種類。
	 * @param[in] force_clear (オプション・デフォルト false) trueの場合、カウントに関わらず即座に削除します。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 * @return 減らしたcountと、現在の警告のcountをペアで返します（first: 減らしたcount, second: 現在のcount）。
	 */
	std::pair<int, int> remove_warning(WarningType type, bool force_clear = false, bool update_ui = true);

	/**
	 * @brief スキャン警告をすべて削除します。
	 * @details
	 * WarningType の追加時に、削除する警告種類の追加を is_state_warning と一緒に忘れずできるよう、
	 * ヘッダで定義します (実装も順番にいくつか関数を呼び出すだけで軽量なため)。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 */
	void clear_scan_warnings(bool update_ui = true) {
		remove_warning(WarningType::TOOL_NOT_FOUND, true, false);
		remove_warning(WarningType::DIFFERENT_TOOL, true, false);
		remove_warning(WarningType::NOT_YET_STORED, true, false);
		remove_warning(WarningType::ALREADY_RETRIEVED, true, false);
		remove_warning(WarningType::ALREADY_STORED, true, false);
		remove_warning(WarningType::RESTORE_RETRIEVED_INSERT, true, false);
		if (update_ui) { update_warning_display(); }
	}

	/**
	 * @brief すべての警告をクリアします。
	 */
	void clear_warnings(bool update_ui = true);

	// --- 認証用のヘルパー関数 ---

	/**
	 * @brief [認証用] パスワード認証ダイアログを表示し、認証を行います。
	 * @return 認証に成功した場合はtrue、失敗またはキャンセルの場合はfalseを返します。
	 */
	bool authenticate_with_password();

	// --- 外部連携用のヘルパー関数 ---

	/**
	 * @brief コンテナを搬入できるかチェックし、できる場合は搬入し、できない場合に適切なエラーメッセージを表示する関数。
	 * @return 搬入可能な場合はtrue、不可能な場合はfalseを返します。
	 */
	bool check_and_store_container();

	/**
	 * @brief MediaServerを起動します。
	 * @return 起動に成功した場合はtrue、失敗した場合はfalseを返します。
	 */
	bool start_MediaServer() {
#ifdef PRODUCTION
		return runner.start("live555MediaServer");
//		return true;
#else
		return true;
#endif
	}

	/**
	 * @brief MediaServerを停止します。
	 */
	void stop_MediaServer() {
		runner.stop();
	}

	// --- 取り出しモード用のヘルパー関数 ---

	/**
	 * @brief [取り出し用] ツール割当情報のリストをテーブル表示用データに変換してページネーションモデルにセットする
	 * @param[out] list_store_model セット対象のページネーションモデル
	 * @param[in] tool_assignments ツール割当情報のリスト
	 */
	void set_tool_assignments(sgex::PaginatedListStoreModel<gui::MainWindow::MainTableData>& list_store_model, const std::vector<ToolAssignmentInfo> &tool_assignments);

	/**
	 * @brief [取り出し用] 3つのシステム(GUI, PLC, Tracker)の状態をチェックし、
	 * RETRIEVE_READ_COMPLETED 状態での操作ボタンの感応状態を一括更新する。
	 * @param[in] update_ui (オプション・デフォルト true) trueのとき、UIを更新する。
	 */
	void update_retrieve_read_completed_buttons(bool update_ui = true);

	// --- 補充モード用のヘルパー関数 ---

	/**
	 * @brief [補充用] 再研磨された工具かを確認するダイアログを表示し、結果を取得します。
	 * @param[in] default_value (オプション・デフォルト true) デフォルトで「はい」/「いいえ」のどちらを選択した状態にするかを指定。
	 * @param[in] message (オプション・デフォルト "一度取り出された工具を検出しました\n再研磨された工具ですか？")
	 * ダイアログに表示するメッセージを指定。
	 * @return 再研磨された場合はtrue、されていない場合はfalseを返します。
	 */
	bool confirm_if_reground(bool default_value = true, UiText message = UiText{
		"一度取り出された工具を検出しました\n再研磨された工具ですか？",
		"いちど とりだされた こうぐを けんしゅつしました\nさいけんま された こうぐですか？"
	});

	//==========================================================================================
	// プライベートメンバ関数 (イベントハンドラ) [PRIVATE_EVENT_HANDLERS]
	//==========================================================================================

	/**
	 * @brief ウィンドウが初回表示されたときに一度だけ呼び出されるハンドラ
	 * @details ウィンドウの初期化を行う。
	 */
	void on_window_show();

	/**
	 * @brief set_timeoutで設定された間隔ごとに呼び出されるタイマーハンドラ。
	 * @return trueを返すとタイマーが継続し、falseを返すと停止します。
	 */
	bool on_timeout() override;

	/**
	 * @brief ウィンドウの閉じるボタンが押されたときに呼び出されるハンドラ。
	 * 各種リソースの解放処理を行う。
	 * @return trueを返すとウィンドウの閉鎖がキャンセルされ、falseを返すと閉鎖が続行される。
	 */
	bool on_delete_event(GtkWidget* self, GdkEvent* event) override;

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
	 * @brief メインテーブルの Selection オブジェクトの "changed" シグナルに対応するハンドラ。
	 * @details メインテーブルの選択が変更されたときに呼び出される。現在の状態に応じたコールバックを実行する。
	 */
	void mainTableSelection_on_changed(std::optional<size_t> selected_index_opt);

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
	 * @brief confirmInsertCountDialog の "show" シグナルに対応するハンドラ。
	 * @details ダイアログが表示された直後に呼び出され、insertCountEntry に初期値を設定する。
	 */
	void confirmInsertCountDialog_on_show();

	/**
	 * @brief passwordEntryDialog の "show" シグナルに対応するハンドラ。
	 * @details ダイアログが表示された直後に呼び出され、パスワード入力欄にフォーカスを当てる。
	 */
	void passwordEntryDialog_on_show();

	/**
	 * @brief warningListDialog の "response" シグナルに対応するハンドラ。
	 * @details ダイアログにレスポンスシグナルが発生した直後に呼び出され、warningListStore をクリアする。
	 * @param response_id 発行されたレスポンスID
	 */
	void warningListDialog_on_response(gint response_id);

	/**
	 * @brief insertCountIncrementButton の "clicked" シグナルに対応するハンドラ。
	 * @details insertCountEntry の数値をインクリメントする。
	 */
	void insertCountIncrementButton_on_clicked();

	/**
	 * @brief insertCountDecrementButton の "clicked" シグナルに対応するハンドラ。
	 * @details insertCountEntry の数値をデクリメントする。
	 */
	void insertCountDecrementButton_on_clicked();

	/**
	 * @brief insertCountDetectButton の "clicked" シグナルに対応するハンドラ。
	 * @details インサート個数検出サーバーに問い合わせた結果を、insertCountEntry に適用する。
	 */
	void insertCountDetectButton_on_clicked();

	/**
	 * @brief notRegroundButton の "toggled" シグナルに対応するハンドラ。
	 * @details 「いいえ」が選択された状態への切り替えを試みる。
	 */
	void notRegroundButton_on_toggled();

	/**
	 * @brief regroundButton の "toggled" シグナルに対応するハンドラ。
	 * @details 「はい」が選択された状態への切り替えを試みる。
	 */
	void regroundButton_on_toggled();

	/**
	 * @brief warningList の "map" シグナルに対応するハンドラ。
	 * @details 表を再描画する。
	 */
	void warningList_on_map();

	/**
	 * @brief warningListCloseButton の "clicked" シグナルに対応するハンドラ。
	 * @details 警告ダイアログを閉じる。
	 */
	void warningListCloseButton_on_clicked();

	/**
	 * @brief warningListSuppressButton の "clicked" シグナルに対応するハンドラ。
	 * @details 警告ダイアログを閉じ、警告を無視して処理を続行する。
	 */
	void warningListSuppressButton_on_clicked();

	//==========================================================================================
	// プライベートメンバ関数 (状態毎の関数) [PRIVATE_STATE_METHODS]
	//==========================================================================================

	/// @brief WAITING_FOR_USER_LOGIN 状態に入ったときの処理
	void on_enter_waiting_for_user_login();

	// --- 取り出しモードの状態遷移関数 ---
	/**
	 * @brief RETRIEVE_WAITING_FOR_PROGRAM_QR 状態に入ったときの処理。
	 * @details プログラム割当一覧表の読み取り準備を行う。
	 */
	void on_enter_retrieve_waiting_for_program_qr();

	/**
	 * @brief RETRIEVE_WAITING_FOR_PROGRAM_QR 状態の qrCodeEntry.on_activate の処理。
	 * @details プログラム割当一覧表の情報をDBから取得して表示する。
	 */
	void on_qrCodeEntry_retrieve_waiting_for_program_qr();

	/**
	 * @brief RETRIEVE_SELECTING_TOOLS 状態に入った時の処理。
	 * @details QRコードで取得した情報をリストに表示する。
	 */
	void on_enter_retrieve_selecting_tools();

	/**
	 * @brief RETRIEVE_SELECTING_TOOLS 状態の mainTableSelection_on_changed の処理。
	 * @details 選択された行の選択/非選択の状態を切り替える。
	 */
	void on_mainTableSelection_retrieve_selecting_tools(std::optional<size_t> selected_index_opt);

	/**
	 * @brief brief RETRIEVE_SELECTING_TOOLS 状態の falseButton.on_clicked の処理
	 * @details 選択状態の行を非選択状態に戻す。
	 */
	void on_falseButton_retrieve_selecting_tools();

	/**
	 * @brief RETRIEVE_CONFIRMING_SELECTION 状態に入った時の処理。
	 * @details 選択状態の行を抽出し、リストに表示する。
	 */
	void on_enter_retrieve_confirming_selection();

	/**
	 * @brief RETRIEVE_CONFIRMING_SELECTION 状態の falseButton.on_clicked の処理
	 * @details 選択されたリストの行の背景色の状態を元の状態に変更しリストを更新する。
	 */
	void on_falseButton_retrieve_confirming_selection();

	/**
	 * @brief RETRIEVE_CONFIRMING_SELECTION 状態の trueButton.on_clicked の処理
	 * @details 確定された選択済み工具を保存し、次の画面に遷移する。
	 */
	void on_trueButton_retrieve_confirming_selection();

	/**
	 * @brief RETRIEVE_CONTAINER_RETRIEVING 状態に入ったときの処理。
	 * @details 取り出す工具が入ったコンテナを搬出し、搬出中のコンテナを強調表示する。
	 */
	void on_enter_retrieve_container_retrieving();

	/**
	 * @brief RETRIEVE_CONTAINER_RETRIEVING 状態の on_plc_notified の処理。
	 * @details PLCからの通知を受け取り、搬出が完了すれば次の画面に遷移する。
	 * @param[in] notification PLCモニタースレッドからの通知。
	 */
	void on_plc_retrieve_container_retrieving(const plc::Notification& notification);

	/**
	 * @brief RETRIEVE_CONTAINER_RETRIEVED 状態に入ったときの処理。
	 * @details 利用者が扉を開けるまで待機する。
	 */
	void on_enter_retrieve_container_retrieved();

	/**
	 * @brief RETRIEVE_CONTAINER_RETRIEVED 状態の on_plc_notified の処理。
	 * @details PLCからの通知を受け取り、扉が開かれたら次の画面に遷移する。
	 * @param[in] notification PLCモニタースレッドからの通知。
	 */
	void on_plc_retrieve_container_retrieved(const plc::Notification& notification);

	/**
	 * @brief RETRIEVE_READY_FOR_READING 状態に入ったときの処理
	 * @details 表をクリアして次の読み取り準備を行い、PLC監視スレッドに搬入準備完了状態の監視を命令する。
	 */
	void on_enter_retrieve_ready_for_reading();

	/**
	 * @brief RETRIEVE_READY_FOR_READING 状態の qrCodeEntry.on_activate の処理
	 * @details 読み取られた工具情報をDBから取得して、正しければ追跡システムに正しいスキャンか問い合わせる。
	 */
	void on_qrCodeEntry_retrieve_ready_for_reading();

	/**
	 * @brief RETRIEVE_READY_FOR_READING 状態の falseButton と trueButton の on_clicked の共通処理
	 * @return 操作がキャンセルされた場合は false、そうでなければ true を返す。
	 */
	bool on_false_and_true_Button_retrieve_ready_for_reading();

	/**
	 * @brief RETRIEVE_READY_FOR_READING 状態の falseButton.on_clicked の処理
	 * @details 取り出し機能に用いていた変数を初期化し、取り出し機能を終了する。
	 */
	void on_falseButton_retrieve_ready_for_reading();

	/**
	 * @brief RETRIEVE_READY_FOR_READING 状態の trueButton.on_clicked の処理
	 * @details 次の工具の取り出しへ遷移、もしくは、取り出し機能の最初の画面に戻る。
	 */
	void on_trueButton_retrieve_ready_for_reading();

	/**
	 * @brief RETRIEVE_READY_FOR_READING 状態の on_plc_notified の処理。
	 * @details PLCからの通知を受け取り、扉が閉じられたらtrue/falseボタンを有効にする。
	 * @param[in] notification PLCモニタースレッドからの通知。
	 */
	void on_plc_retrieve_ready_for_reading(const plc::Notification& notification);

	/**
	 * @brief RETRIEVE_READY_FOR_READING 状態の on_tracker_notified の処理。
	 * @details 追跡システムからの通知を受け取り、通知内容に応じてGUIを操作する。
	 * @param[in] notification 追跡システムのリスナースレッドからの通知。
	 */
	void on_tracker_retrieve_ready_for_reading(const tracker::ToolTrackerClient::Notification& notification);

	// --- 補充モードの状態遷移関数 ---
	/**
	 * @brief STORE_WAITING_FOR_TOOL_QR 状態に入ったときの処理。
	 * @details 補充する工具の読み取り準備を行う。
	 */
	void on_enter_store_waiting_for_tool_qr();

	/**
	 * @brief STORE_WAITING_FOR_TOOL_QR 状態の qrCodeEntry.on_activate の処理。
	 * @details これから補充する工具の情報をDBから取得する。
	 */
	void on_qrCodeEntry_store_waiting_for_tool_qr();

	/**
	 * @brief STORE_CONTAINER_RETRIEVING 状態に入ったときの処理。
	 * @details 工具を補充するコンテナを搬出する。
	 */
	void on_enter_store_container_retrieving();

	/**
	 * @brief STORE_CONTAINER_RETRIEVING 状態の on_plc_notified の処理。
	 * @details PLCからの通知を受け取り、搬出が完了すれば次の画面に遷移する。
	 * @param[in] notification PLCモニタースレッドからの通知。
	 */
	void on_plc_store_container_retrieving(const plc::Notification& notification);

	/**
	 * @brief STORE_CONTAINER_RETRIEVED 状態に入ったときの処理。
	 * @details 利用者が扉を開けるまで待機する。
	 */
	void on_enter_store_container_retrieved();

	/**
	 * @brief STORE_CONTAINER_RETRIEVED 状態の on_plc_notified の処理。
	 * @details PLCからの通知を受け取り、扉が開かれたら次の画面に遷移する。
	 * @param[in] notification PLCモニタースレッドからの通知。
	 */
	void on_plc_store_container_retrieved(const plc::Notification& notification);

	/**
	 * @brief STORE_READY_FOR_READING 状態に入ったときの処理
	 * @details 表をクリアして、次の読み取り準備を行う。
	 */
	void on_enter_store_ready_for_reading();

	/**
	 * @brief STORE_READY_FOR_READING 状態の qrCodeEntry.on_activate の処理
	 * @details on_qrCodeEntry_store_read_completed() の処理に加え、
	 * 読み取りに成功した場合は次の画面に遷移する。
	 */
	void on_qrCodeEntry_store_ready_for_reading();

	/**
	 * @brief STORE_READY_FOR_READING 状態の falseButton と trueButton の on_clicked の共通処理
	 * @return 操作がキャンセルされた場合は false、そうでなければ true を返す。
	 */
	bool on_false_and_true_Button_store_ready_for_reading();

	/**
	 * @brief STORE_READY_FOR_READING 状態の falseButton.on_clicked の処理
	 * @details 補充機能を終了する。
	 */
	void on_falseButton_store_ready_for_reading();

	/**
	 * @brief STORE_READY_FOR_READING 状態の trueButton.on_clicked の処理
	 * @details 補充機能の最初の画面に戻る。
	 */
	void on_trueButton_store_ready_for_reading();

	/**
	 * @brief STORE_READY_FOR_READING 状態の on_plc_notified の処理。
	 * @details PLCからの通知を受け取り、扉が閉じられたらtrue/falseボタンを有効にする。
	 * @param[in] notification PLCモニタースレッドからの通知。
	 */
	void on_plc_store_ready_for_reading(const plc::Notification& notification);

	// --- 棚卸しモードの状態遷移関数 ---
	/**
	 * @brief RECONCILE_WAITING_FOR_CONTAINER_SELECTION 状態に入ったときの処理。
	 * @details DBから取得したコンテナリストをテーブルに表示する。
	 */
	void on_enter_reconcile_waiting_for_container_selection();

	/**
	 * @brief RECONCILE_WAITING_FOR_CONTAINER_SELECTION 状態の mainTableSelection_on_changed の処理。
	 * @details 選択中の状態に選択行を変化させ、trueButton を有効化する。
	 */
	void on_mainTableSelection_reconcile_waiting_for_container_selection(std::optional<size_t> selected_index_opt);

	/**
	 * @brief RECONCILE_CONTAINER_RETRIEVING 状態に入ったときの処理。
	 * @details 工具を補充するコンテナを搬出する。
	 */
	void on_enter_reconcile_container_retrieving();

	/**
	 * @brief RECONCILE_CONTAINER_RETRIEVING 状態の on_plc_notified の処理。
	 * @details PLCからの通知を受け取り、搬出が完了すれば次の画面に遷移する。
	 * @param[in] notification PLCモニタースレッドからの通知。
	 */
	void on_plc_reconcile_container_retrieving(const plc::Notification& notification);

	/**
	 * @brief RECONCILE_CONTAINER_RETRIEVED 状態に入ったときの処理。
	 * @details 利用者が扉を開けるまで待機する。
	 */
	void on_enter_reconcile_container_retrieved();

	/**
	 * @brief RECONCILE_CONTAINER_RETRIEVED 状態の on_plc_notified の処理。
	 * @details PLCからの通知を受け取り、扉が開かれたら次の画面に遷移する。
	 * @param[in] notification PLCモニタースレッドからの通知。
	 */
	void on_plc_reconcile_container_retrieved(const plc::Notification& notification);

	/**
	 * @brief RECONCILE_READY_FOR_READING 状態に入ったときの処理。
	 * @details DBからコンテナの在庫リストを取得し、テーブルに「未読」状態で表示
	 */
	void on_enter_reconcile_ready_for_reading();

	/**
	 * @brief RECONCILE_READY_FOR_READING 状態の qrCodeEntry.on_activate の処理
	 * @details on_qrCodeEntry_reconcile_read_completed() の処理に加え、
	 * 読み取りに成功した場合に次の画面に遷移する。
	 */
	void on_qrCodeEntry_reconcile_ready_for_reading();

	/**
	 * @brief RECONCILE_READ_COMPLETED 状態に入ったときの処理。
	 * @details 扉を閉めるまで待機するよう、PLC連携用スレッドに命令する。
	 */
	void on_enter_reconcile_read_completed();

	/**
	 * @brief RECONCILE_READ_COMPLETED 状態の qrCodeEntry.on_activate の処理
	 * @details 読み取られた工具情報をDBから取得し、テーブル内の該当工具を検索して、
	 * 見つかれば「一致」状態に更新し、見つからなければ「不一致」として新規行を追加する。
	 * @return 読み取った工具をDBから取得できた場合true、取得できなかった場合false。
	 */
	bool on_qrCodeEntry_reconcile_read_completed();

	/**
	 * @brief RECONCILE_READ_COMPLETED 状態の falseButton.on_clicked の処理
	 * @details 棚卸し機能を終了する。
	 */
	void on_falseButton_reconcile_read_completed();

	/**
	 * @brief RECONCILE_READ_COMPLETED 状態の trueButton.on_clicked の処理
	 * @details 棚卸し機能の最初の画面に戻る。
	 */
	void on_trueButton_reconcile_read_completed();

	// TODO: 棚卸しモードの状態遷移関数をここに追加

	//==========================================================================================
	// プライベートメンバ変数 (状態管理等) [PRIVATE_VARS]
	//==========================================================================================

	// --- 外部連携用 ---
	/// @brief PLCに接続されているかどうかのフラグ。
	bool m_plc_connected;
	/// @brief PLCメンテナンスモード中かどうかのフラグ。
	bool m_plc_maintaining;
	/// @brief PLCでエラー発生中かどうかのフラグ。
	bool m_plc_has_error;
	/// @brief 追跡システムのカメラサーバ起動に使う、バックグラウンドプロセス管理者
	gutils::ProcessRunner runner;

	// --- 二重処理防止用 ---

	/// @brief ウィンドウが初回表示かどうかを判定するフラグ
	bool m_is_first_show;

	/**
	 * @brief プログラムが内部的にトグルボタンの状態を変更しているかを示すフラグ。
	 * @details trueの間、toggled イベントハンドラは何もしない。
	 */
	bool m_is_programmatically_changing_toggle;

	// --- 状態管理用 ---
	/// @brief パスワード認証が成功したかどうかのフラグ。
	bool m_password_authenticated;
	/// @brief 現在のGUI操作モードを保持するメンバ変数。
	GuiMode m_gui_mode;
	/// @brief 現在の状態を保持するメンバ変数。
	AppState m_app_state;
	/// @brief インサートかどうかで分岐する必要がある場合に使用するフラグ。
	bool m_is_insert_operation;
	/// @brief 現在のテキストモード(かな/漢字)を保持するメンバ変数。
	TextMode m_text_mode;

	/// @brief ウィジェットの状態を保持し、変更があったウィジェットを追跡するマップ。
	utils::TrackedMap<sg::Widget*, WidgetStateUpdater> m_widget_states;

	/// @brief 各テーブルの状態を保持し、変更があった要素を追跡するマップ。
	utils::TrackedMap<sgex::ListStoreViewInterface*, TableState> m_table_states;

	/**
	 * @brief アプリケーション状態とウィジェット設定を紐付けるマップ。
	 * @details アプリケーションの全状態の「見た目」と「振る舞い」をここで一元管理します。
	 */
	std::map<AppState, WidgetStateConfig> g_app_state_configs;

	/// @brief 各テーブル行の状態に対応する背景色を保持するマップ。
	static const std::map<TableCellState, GdkRGBA> g_table_row_colors;

	/// @brief 各警告種類に対応する警告メッセージを保持するマップ。
	static const std::map<WarningType, UiText> g_warning_type_messages;

	// --- ウィジェット制御用 ---

	/// @brief insertCountEntry の現在個数を格納しておくダイアログ。
	int m_insertCountEntry_current_count;

	/// @brief confirmInsertCountDialog の最大個数を格納しておくダイアログ。0未満の時は最大値無し。
	int m_confirmInsertCountDialog_max_count;

	// --- 操作中の状態保持用 (複数場面共通) ---

	/// @brief 操作している保管領域。
	std::optional<StorageAreaInfo> m_operating_area;

	/// @brief 操作している工具の基本情報
	std::optional<MstToolInfo> m_operating_tool;

	/// @brief 操作した工具ぞれぞれの操作記録をまとめておく vector。
	std::vector<RecOperationTable> m_operated_records;

	/**
	 * @brief 追跡システムからのスキャン結果応答を待機している工具の情報。
	 * @details on_qrCodeEntry... でセットされ、on_tracker... でスキャン結果応答を受け取ると消費されます。
	 */
	std::optional<UniqueToolInfo> m_pending_tool_info;

	/// @brief PLCの操作許可状態 (例: 扉が閉まっているか)
	bool m_plc_ready_for_action;

	/// @brief ユーザーによって警告が閉じられたかどうか
	bool m_warning_closed_by_user;

	/// @brief その回の操作で、今後すべての警告を無視するかどうか
	bool m_ignore_all_warnings;

	// --- 操作中の状態保持用 (取り出し専用) ---

	/// @brief [取り出し用] 現在取り出し中のツーリングリスト
	std::optional<ToolingListInfo> m_retrieving_list;

	/// @brief [取り出し用] 確認画面移行前の表の状態を退避しておく vector。
	std::vector<MainTableData> m_retrieve_backup_table;

	/// @brief [取り出し用] 取り出し中工具のリスト。
	std::vector<ToolAssignmentInfo> m_retrieving_tools;

	/// @brief [取り出し用] m_retrieving_tools の中で、現在どの工具の取り出しを行っているか、インデックスを格納する。
	std::optional<size_t> m_retrieving_index;

	/// @brief [取り出し用] 現在場にある未スキャン工具の数
	int m_unscanned_count;

	// --- 操作中の状態保持用 (補充専用) ---

	/// @brief [補充用] 再研磨工具のIDリスト。
	std::set<std::int64_t> m_reground_tools;

	// --- ブランチで末尾に追加するための仕切り ---	// TODO: 最後にこの行を削除
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
} // namespace gui
