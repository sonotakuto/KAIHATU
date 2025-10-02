/**
 * @file SimpleGtk_extension.hpp
 * @brief SimpleGtkを拡張する、gtkのラッパークラスをまとめたヘッダファイル。
 * @details ToggleButton, Stack, Boxの3クラスを提供します。
 */
#pragma once
#include <SimpleGtk> // インストール済みのSimpleGtkライブラリをインクルード

/**
 * @namespace tool_io
 * @brief この「工具入出庫システム」プロジェクト固有のウィジェットやクラスを格納する名前空間。
 */
namespace tool_io {

/**
 * @class Window
 * @brief sg::Windowを拡張し、機能を追加したクラス。
 */
class Window : public sg::Window {
public:
	using sg::Window::Window; // 基底クラスのコンストラクタを継承

	// メソッド

	// プロパティ

	// シグナル
	DEFINE_SIGNAL_HANDLER("show", show, Window, GtkWindow* self)
};

/**
 * @class Dialog
 * @brief sg::Dialogを拡張し、機能を追加したクラス。
 * @details "show"シグナルは、ダイアログが表示された直後に発生します。
 */
class Dialog : public sg::Dialog {
public:
	// 基底クラスのコンストラクタを継承
	using sg::Dialog::Dialog;

	// メソッド

	// プロパティ

	// シグナル
	DEFINE_SIGNAL_HANDLER("show", show, Dialog, GtkWidget* self)
};

/**
 * @class Box
 * @brief GtkBoxのラッパークラス。ウィジェットを水平または垂直に配置するためのコンテナです。
 * @details sg::Containerを継承し、ウィジェットのパッキング（追加）機能などを提供します。
 */
class Box : public sg::Container {
public:
	// コンストラクタ
	Box(Glade& glade, const char* id) : Container(glade, id) {}
	virtual ~Box() = default;

	// コピーは禁止
	Box(const Box&) = delete;
	Box& operator=(const Box&) = delete;
	// ムーブは許可
	Box(Box&& other) noexcept : Container(std::move(other)) {}
	Box& operator=(Box&& other) noexcept {
		Container::operator=(std::move(other));
		return *this;
	}

	// GtkBox*へのキャスト演算子
	operator GtkBox*() const { return GTK_BOX(this->object); }

	// メソッド
	DEFINE_METHOD_WITH_ARGS(box, pack_start, void, (GtkWidget* child, bool expand, bool fill, guint padding), child, expand, fill, padding)
	DEFINE_METHOD_WITH_ARGS(box, pack_end, void, (GtkWidget* child, bool expand, bool fill, guint padding), child, expand, fill, padding)

	// プロパティ
	DEFINE_PROPERTY_ACCESSORS(Widget, visible, bool)

	// シグナル
};

/**
 * @class Stack
 * @brief GtkStackのラッパークラス。複数のページを重ねて管理し、一度に1つだけ表示します。
 * @details sg::Containerを継承し、表示するページを名前で切り替える機能などを提供します。
 */
class Stack : public sg::Container {
public:
	// コンストラクタ
	Stack(Glade& glade, const char* id) : Container(glade, id) {}
	virtual ~Stack() = default;

	// コピーは禁止
	Stack(const Stack&) = delete;
	Stack& operator=(const Stack&) = delete;
	// ムーブは許可
	Stack(Stack&& other) noexcept : Container(std::move(other)) {}
	Stack& operator=(Stack&& other) noexcept {
		Container::operator=(std::move(other));
		return *this;
	}

	// GtkStack*へのキャスト演算子
	operator GtkStack*() const { return GTK_STACK(this->object); }

	// メソッド
	DEFINE_METHOD_WITH_ARGS(stack, set_visible_child_name, void, (const char* name), name)
	DEFINE_METHOD(stack, get_visible_child_name, const char*)

	// プロパティ
	DEFINE_PROPERTY_ACCESSORS(Widget, visible, bool)

	// シグナル
};

/**
 * @class Button
 * @brief sg::Buttonを拡張し、機能を追加したクラス。
 */
class Button : public sg::Button {
public:
	using sg::Button::Button; // 基底クラスのコンストラクタを継承

	// メソッド

	// プロパティ
	DEFINE_PROPERTY_ACCESSORS(Widget, visible, bool)

	// シグナル
};

/**
 * @class Entry
 * @brief sg::Entryを拡張し、機能を追加したクラス。
 */
class Entry : public sg::Entry {
public:
	// 基底クラスのコンストラクタを継承
	using sg::Entry::Entry;

	// メソッド

	// プロパティ
	DEFINE_PROPERTY_ACCESSORS(Widget, sensitive, bool)
	DEFINE_PROPERTY_ACCESSORS(Widget, visible, bool)

	// シグナル
	DEFINE_SIGNAL_HANDLER("activate", activate, Entry, GtkEntry* self)
};

/**
 * @class ToggleButton
 * @brief GtkToggleButtonのラッパークラス。オン/オフの状態を持つボタンを提供します。
 * @details sg::Buttonを継承し、アクティブ状態の取得/設定やtoggledシグナルの機能を追加します。
 */
class ToggleButton : public tool_io::Button {
public:
	// コンストラクタ
	ToggleButton(Glade& glade, const char* id) : Button(glade, id) {}
	virtual ~ToggleButton() = default;

	// コピーは禁止
	ToggleButton(const ToggleButton&) = delete;
	ToggleButton& operator=(const ToggleButton&) = delete;
	// ムーブは許可
	ToggleButton(ToggleButton&& other) noexcept : Button(std::move(other)) {}
	ToggleButton& operator=(ToggleButton&& other) noexcept {
		Button::operator=(std::move(other));
		return *this;
	}

	// GtkToggleButton*へのキャスト演算子
	operator GtkToggleButton*() const { return GTK_TOGGLE_BUTTON(this->object); }

	// メソッド
	DEFINE_METHOD_WITH_ARGS(toggle_button, set_active, void, (bool is_active), is_active)
	DEFINE_METHOD(toggle_button, get_active, bool)

	// プロパティ

	// シグナル
	DEFINE_SIGNAL_HANDLER("toggled", toggled, ToggleButton, GtkToggleButton* self)
};

/**
 * @class Switch
 * @brief GtkSwitchのラッパークラス。オン/オフを切り替えるスイッチを提供します。
 */
class Switch : public sg::Widget {
public:
	using sg::Widget::Widget;	// 基底クラスのコンストラクタを継承
	operator GtkSwitch*() const { return GTK_SWITCH(this->object); }

	// メソッド
	DEFINE_METHOD(switch, get_active, bool)
	DEFINE_METHOD_WITH_ARGS(switch, set_active, void, (bool is_active), is_active)

	// プロパティ

	// シグナル
	DEFINE_SIGNAL_HANDLER("state-set", state_set, Switch, GtkSwitch* self, bool state)
};

/**
 * @class ComboBox
 * @brief GtkComboBoxのラッパークラス。ドロップダウンリストを提供します。
 */
class ComboBox : public sg::Widget {
public:
	using sg::Widget::Widget;	// 基底クラスのコンストラクタを継承
	operator GtkComboBox*() const { return GTK_COMBO_BOX(this->object); }

	// メソッド
	DEFINE_METHOD(combo_box, get_active, gint)
	DEFINE_METHOD_WITH_ARGS(combo_box, set_active, void, (gint index), index)

	// プロパティ

	// シグナル
	DEFINE_SIGNAL_HANDLER("changed", changed, ComboBox, GtkComboBox* self)
};

/**
 * @class Label
 * @brief sg::Labelを拡張し、機能を追加したクラス。
 */
class Label : public sg::Label {
public:
	using sg::Label::Label; // 基底クラスのコンストラクタを継承

	// メソッド

	// プロパティ
	DEFINE_PROPERTY_ACCESSORS(Widget, visible, bool)

	// シグナル
};

/**
 * @class TreeView
 * @brief sg::TreeViewを拡張し、機能を追加したクラス。
 */
class TreeView : public sg::TreeView {
public:
	using sg::TreeView::TreeView; // 基底クラスのコンストラクタを継承

	// メソッド
	DEFINE_METHOD_WITH_ARGS(tree_view, set_model, void, (GtkTreeModel* model), model)
	DEFINE_METHOD_WITH_ARGS(tree_view, append_column, gint, (GtkTreeViewColumn* column), column)

	// プロパティ
	DEFINE_PROPERTY_ACCESSORS(Widget, sensitive, bool)
	DEFINE_PROPERTY_ACCESSORS(Widget, visible, bool)

	// シグナル
};

/**
 * @class ListStoreModelBase
 * @brief GtkListStoreとstd::vector<RowStruct>を同期させる、汎用的なデータモデルの基底クラス。
 * @details このクラスはTreeViewやComboBoxなどの特定のウィジェットには依存せず、
 * GtkListStoreそのものの生成と、C++側のデータモデル(std::vector)の基本的な
 * CRUD(Create, Read, Update, Delete)操作のみを提供します。
 * @tparam RowStruct テーブルの1行分のデータを表現する構造体の型。
 */
template<typename RowStruct>
class ListStoreModelBase {
protected:
	/// @brief (View) SimpleGtkでラップされたGtkListStoreの所有権を持つポインタ。
	std::unique_ptr<sg::ListStore> m_list_store_ptr;
	/// @brief (Model) C++側のデータ本体。アプリケーションロジックの「信頼できる唯一の情報源」。
	std::vector<RowStruct> m_data_model;
	/// @brief RowStructからGtkListStoreの行データに変換するためのセッター関数。
	std::function<void(GtkListStore*, GtkTreeIter*, const RowStruct&)> m_row_setter;

	using size_type = typename std::vector<RowStruct>::size_type;

	/**
	 * @brief sg::ListStore へのポインタを取得します。
	 * @details ComboBox や TreeView にモデルとして設定する際に使用します。
	 * @return 内部で管理している sg::ListStore へのポインタ。
	 */
	sg::ListStore* get_sg_store() const { return m_list_store_ptr.get(); }

	/**
	 * @brief 内部のListStoreをGtkListStore*として取得するヘルパー関数。
	 * @details 複雑なキャストをこの関数内に隠蔽します。
	 * @return 内部で管理している sg::ListStore から取得した GtkListStore へのポインタ。
	 */
	GtkListStore* get_gtk_store() const {
		if (!m_list_store_ptr) return nullptr;
		// 複雑なキャストはこの一箇所に集約
		return (GtkListStore*)(*m_list_store_ptr);
	}

	/**
	 * @brief 内部のListStoreをGtkTreeModel*として取得するヘルパー関数。
	 * @details 複雑なキャストをこの関数内に隠蔽します。
	 * @return 内部で管理している sg::ListStore から取得した GtkTreeModel へのポインタ。
	 */
	GtkTreeModel* get_gtk_model() const {
		if (!m_list_store_ptr) return nullptr;
		// 複雑なキャストはこの一箇所に集約
		return GTK_TREE_MODEL(get_gtk_store());
	}

	/**
	 * @brief 保持しているセッター関数(m_row_setter)を呼び出して、GtkListStoreの行に値を設定します。
	 * @param iter 値を設定する行を指すGtkTreeIter。
	 * @param row_data セットするデータを持つRowStruct。
	 */
	void set_row_to_store(GtkTreeIter* iter, const RowStruct& row_data) {
		if (m_row_setter) {
			m_row_setter(get_gtk_store(), iter, row_data);
		}
	}

	public:
	/**
	 * @brief ListStoreModelBaseのコンストラクタ。
	 * @param column_types GtkListStoreが持つ列のGTypeのリスト。
	 * @param row_setter RowStructのデータをGtkListStoreの行に変換・設定する関数。
	 */
	ListStoreModelBase(
		const std::vector<GType>& column_types,
		std::function<void(GtkListStore*, GtkTreeIter*, const RowStruct&)> row_setter
	) : m_row_setter(row_setter)
	{
		// GTypeのリストに基づいて、新しいGtkListStoreをプログラムで生成します。
		GtkListStore* new_gtk_store = gtk_list_store_newv(column_types.size(), (GType*)column_types.data());
		// 生成したGtkListStoreをSimpleGtkラッパーでラップし、unique_ptrで所有権を管理します。
		m_list_store_ptr = std::make_unique<sg::ListStore>(new_gtk_store);
	}

	/**
	 * @brief 仮想デストラクタ。
	 * @details このクラスが継承されるため、派生クラスのデストラクタが正しく呼び出されるように
	 * 仮想デストラクタを定義しておくのが安全です。
	 */
	virtual ~ListStoreModelBase() = default;

	// --- 基本的なデータ操作（単一行） ---

	/// @brief C++モデルとGtkListStoreの両方から全ての行を削除します。
	void clear() {
		m_data_model.clear();
		m_list_store_ptr->clear();
	}

	/**
	 * @brief 末尾に単一の行を追加します。
	 * @param row_data 追加する行のデータ。
	 * @return bool 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
	 */
	bool append(const RowStruct& row_data) {
		if (size() >= (size_type)((size_type)(std::numeric_limits<int>::max()))) {
			return false; // 失敗: 行数がintの最大値を超える
		}
		m_data_model.push_back(row_data);
		GtkTreeIter iter;
		gtk_list_store_append(get_gtk_store(), &iter);
		set_row_to_store(&iter, row_data);
		return true; // 成功
	}

	/**
	 * @brief 末尾に複数の行をまとめて追加します。
	 * @param rows 追加する行データのvector。
	 */
	bool append(const std::vector<RowStruct>& rows) {
		if (size() + rows.size() > (size_type)(std::numeric_limits<int>::max())) {
			return false; // 失敗: 行数がintの最大値を超える
		}
		for (const auto& row : rows) {
			append(row); // 単行appendをループで呼び出す
		}
		return true; // 成功
	}

	/**
	 * @brief 指定した位置に単一の行を挿入します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * @param position 0から始まる挿入位置のインデックス。
	 * @param row_data 挿入する行のデータ。
	 * @return bool 指定された位置通りに挿入できればtrue、位置が丸められた場合はfalse。
	 */
	bool insert(size_type position, const RowStruct& row_data) {
		size_type original_position = position;
		bool position_was_clamped = false;

		if (position > m_data_model.size()) {
			position = m_data_model.size();
		}
		if (size() >= (size_type)(std::numeric_limits<int>::max())) {
			return false; // 失敗: 行数がintの最大値を超える
		}

		// 要求された位置と、実際の挿入位置が違うか判定
		if (position != original_position) {
			position_was_clamped = true;
		}

		m_data_model.insert(m_data_model.begin() + position, row_data);
		GtkTreeIter iter;
		gtk_list_store_insert(get_gtk_store(), &iter, position);
		set_row_to_store(&iter, row_data);

		return !position_was_clamped;
	}

	/**
	 * @brief 指定した位置に複数の行をまとめて挿入します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * size()以上の値を指定した場合は、末尾に追加(append)されます。
	 * @param position 0から始まる挿入位置のインデックス。
	 * @param rows 挿入する行データのvector。
	 * @return bool 指定された開始位置が有効であればtrue、丸められた場合はfalse。
	 */
	bool insert(size_type position, const std::vector<RowStruct>& rows) {
		if (size() + rows.size() > (size_type)(std::numeric_limits<int>::max())) {
			return false; // 失敗: 行数がintの最大値を超える
		}
		// 開始位置が有効範囲内([0, size()])にあるかチェック
		bool position_was_valid = (position <= size());

		int current_pos = position;
		for (const auto& row : rows) {
			// 単行insertは自動で位置を丸めてくれる
			insert(current_pos++, row);
		}
		return position_was_valid;
	}

	/**
	 * @brief 指定した位置の行を削除します。
	 * @param position 削除位置のインデックス。
	 * @return bool 削除に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool erase(size_type position) {
		if (position >= m_data_model.size()) {
			return false; // 失敗: インデックスが範囲外
		}

		m_data_model.erase(m_data_model.begin() + position);
		GtkTreeIter iter;
		if (gtk_tree_model_iter_nth_child(get_gtk_model(), &iter, nullptr, position)) {
			gtk_list_store_remove(get_gtk_store(), &iter);
		}

		return true; // 成功
	}

	/**
	 * @brief 指定した複数の位置の行をまとめて削除します。
	 * @param positions 削除する行のインデックスのvector。
	 * @return bool 指定されたインデックスが全て有効な場合のみtrue。一つでも無効なら処理せずfalse。
	 */
	bool erase(std::vector<size_type> positions) {
		// ★ 先に全てのインデックスが有効かチェック
		for (size_type pos : positions) {
			if (pos >= size()) {
				return false; // 一つでも無効なインデックスがあれば失敗
			}
		}

		std::sort(positions.rbegin(), positions.rend());
		for (int pos : positions) {
			erase(pos);
		}
		return true; // 全て成功
	}

	/**
	 * @brief 指定した位置の行を上書きします。
	 * @param position 上書き位置のインデックス。
	 * @param row_data 新しい行データ。
	 * @return bool 更新に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool update(size_type position, const RowStruct& row_data) {
		if (position >= m_data_model.size()) {
			return false; // 失敗: インデックスが範囲外
		}

		m_data_model[position] = row_data;
		GtkTreeIter iter;
		if (gtk_tree_model_iter_nth_child(get_gtk_model(), &iter, nullptr, position)) {
			set_row_to_store(&iter, row_data);
		}

		return true; // 成功
	}

	/**
	 * @brief 指定した複数の位置の行をまとめて上書きします。
	 * @param updates 上書きする位置のインデックスと、新しい行データのmap。
	 * @return bool 指定されたインデックスが全て有効な場合のみtrue。一つでも無効なら処理せずfalse。
	 */
	bool update(const std::map<size_type, RowStruct>& updates) {
		// ★ 先に全てのインデックスが有効かチェック
		for (const auto& [pos, row_data] : updates) {
			if (pos >= size()) {
				return false; // 一つでも無効なインデックスがあれば失敗
			}
		}

		for (const auto& [pos, row_data] : updates) {
			update(pos, row_data);
		}
		return true; // 全て成功
	}

	/**
	 * @brief 指定した位置の行データを取得します。
	 * @param position 0から始まる取得位置のインデックス。
	 * @return const RowStruct& 行データへのconst参照。
	 */
	const RowStruct& get_row(size_type position) const {
		return m_data_model.at(position);
	}

	/**
	 * @brief C++モデル内の現在の行数を返します。
	 * @return size_t 現在の行数。
	 */
	size_type size() const {
		return m_data_model.size();
	}

	/**
	 * @brief 条件に一致する全ての行のインデックスを検索します。
	 * @param predicate 検索条件となる述語関数。
	 * @return std::vector<int> 条件に一致した行のインデックスのリスト。
	 */
	std::vector<int> find_indices(std::function<bool(const RowStruct&)> predicate) const {
		std::vector<int> found_indices;
		for (size_t i = 0; i < m_data_model.size(); ++i) {
			if (predicate(m_data_model[i])) {
				found_indices.push_back(i);
			}
		}
		return found_indices;
	}

	/**
	 * @brief 条件に一致する最初の行のインデックスを検索します。
	 * @param predicate 検索条件となる述語関数。
	 * @return std::optional<int> 条件に一致した最初の行のインデックス。
	 */
	std::optional<int> find_first_index(std::function<bool(const RowStruct&)> predicate) const {
		for (size_t i = 0; i < m_data_model.size(); ++i) {
			if (predicate(m_data_model[i])) {
				return i;
			}
		}
		return std::nullopt;
	}
};


/**
 * @class TreeViewListStoreModel
 * @brief ListStoreModelBaseを継承し、GtkTreeViewに特化した機能を追加した派生クラス。
 * @details 選択行の取得や、パフォーマンスを考慮した複数行の一括更新機能などを提供します。
 * @tparam RowStruct テーブルの1行分のデータを表現する構造体の型。
 */
template<typename RowStruct>
class TreeViewListStoreModel : public ListStoreModelBase<RowStruct> {
private:
	/// @brief このモデルが関連付けられているTreeViewウィジェット。
	tool_io::TreeView& m_tree_view;

public:
	/**
	 * @brief TreeViewListStoreModelのコンストラクタ。
	 * @param tree_view 管理対象のTreeViewウィジェット。
	 * @param column_types GtkListStoreが持つ列のGTypeのリスト。
	 * @param row_setter RowStructのデータをGtkListStoreの行に変換・設定する関数。
	 */
	TreeViewListStoreModel(
		tool_io::TreeView& tree_view,
		const std::vector<GType>& column_types,
		std::function<void(GtkListStore*, GtkTreeIter*, const RowStruct&)> row_setter
	) : ListStoreModelBase<RowStruct>(column_types, row_setter), // 基底クラスのコンストラクタを呼び出し
		m_tree_view(tree_view)
	{
		// TreeViewに、基底クラスで生成された新しいListStoreモデルをセットします。
		m_tree_view.set_model(this->get_gtk_model());
	}

	// --- TreeView特化の機能 ---

	/**
	 * @brief 現在TreeViewで選択されている行のインデックスを取得します。
	 * @return int 選択されている行のインデックス。何も選択されていない場合は-1。
	 */
	int get_selected_index() const {
		sg::TreeSelection selection = m_tree_view.get_selection();
		GtkTreeIter iter;
		if (gtk_tree_selection_get_selected(selection, nullptr, &iter)) {
			GtkTreePath* path = gtk_tree_model_get_path(this->get_gtk_model(), &iter);
			int index = gtk_tree_path_get_indices(path)[0];
			gtk_tree_path_free(path);
			return index;
		}
		return -1;
	}

	/**
	 * @brief 現在選択されている行のデータをC++モデルから取得します。
	 * @return std::optional<RowStruct> 選択されていれば行データ、なければ空のoptional。
	 */
	std::optional<RowStruct> get_selected_row() const {
		int index = get_selected_index();
		if (index >= 0) {
			return this->get_row(index);
		}
		return std::nullopt;
	}
};
} // namespace tool_io