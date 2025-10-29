/**
 * @file SimpleGtk_extension.hpp
 * @brief SimpleGtk を拡張する、gtkのラッパークラスをまとめたヘッダファイル。
 * @details ToggleButton, Stack, Boxの3クラスを提供します。
 */
#pragma once
#include <SimpleGtk> // インストール済みのSimpleGtkライブラリをインクルード

/**
 * @brief 戻り値を持つシグナルのハンドラを定義するマクロ。
 * @param signal_name GTKシグナル名 (文字列)。
 * @param c_signal_name C++の関数名として使用する名前。
 * @param return_type ハンドラの戻り値の型 (例: gboolean)。
 * @param para_static 静的コールバック関数の引数リスト。
 * @param function std::function の型。
 * @param ... std::function に渡す引数。
 */
#define DEFINE_SIGNAL_HANDLER_WITH_RETURN(signal_name, c_signal_name, return_type, para_static, function, ...) \
public: \
void on_##c_signal_name(function handler) { \
	on_##c_signal_name##_handler = handler; \
	if (on_##c_signal_name##_handler != nullptr) \
		set_handler(signal_name, G_CALLBACK(on_##c_signal_name##_static), this); \
} \
private: \
function on_##c_signal_name##_handler = nullptr; \
static return_type on_##c_signal_name##_static para_static { \
	if (thiz->on_##c_signal_name##_handler != nullptr) { \
		return thiz->on_##c_signal_name##_handler(__VA_ARGS__); \
	} \
	return FALSE; \
}

/**
 * @brief GTKプロパティ名にハイフンが含まれる場合に対応した、プロパティのゲッターとセッターを定義するマクロ。
 * @details
 * g_object_get/set関数はプロパティ名を文字列で受け取るためハイフンがあっても動作しますが、
 * C++の関数名（ゲッター/セッター名）にはハイフンを使えません。
 * このマクロは、GTKプロパティ名とC++関数名の一部を別々に指定できるようにすることで、この問題を解決します。
 *
 * @param gtk_property_name GTKオブジェクトのプロパティ名 (例: "can-focus")。文字列リテラルで指定します。
 * @param cpp_suffix 生成されるC++関数名の一部 (例: can_focus)。識別子として有効な名前を指定します。
 * ゲッターは `get_<cpp_suffix>()`、セッターは `set_<cpp_suffix>()` という名前になります。
 * @param PropertyType プロパティのC++におけるデータ型 (例: bool, int, const char*)。
 *
 * @par 使用例:
 * @code
 * // クラス定義内で使用
 * DEFINE_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus, bool)
 * // これにより、以下のメンバ関数が自動的に定義されます:
 * // bool get_can_focus() const;
 * // void set_can_focus(bool value);
 * @endcode
 */
#define DEFINE_PROPERTY_ACCESSORS_WITH_NAME(gtk_property_name, cpp_suffix, PropertyType) \
	PropertyType get_##cpp_suffix() const { \
		PropertyType value; \
		g_object_get(G_OBJECT(this->object), gtk_property_name, &value, NULL); \
		return value; \
	} \
	void set_##cpp_suffix(PropertyType value) { \
		g_object_set(G_OBJECT(this->object), gtk_property_name, value, NULL); \
	}

/**
 * @namespace sgex
 * @brief SimpleGtk を拡張する、gtkのラッパークラスを格納する名前空間。
 */
namespace sgex {

/**
 * @class Window
 * @brief sg::Windowを拡張し、機能を追加したクラス。
 */
class Window : public sg::Window {
public:
	using sg::Window::Window; // 基底クラスのコンストラクタを継承

	// メソッド
	DEFINE_METHOD(window, fullscreen, void)

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
	DEFINE_METHOD_WITH_ARGS(dialog, response, void, (gint response_id), response_id)

	// プロパティ

	// シグナル
	DEFINE_SIGNAL_HANDLER("show", show, Dialog, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER_WITH_RETURN(
		"delete-event",
		delete_event,
		gboolean,
		(GtkWidget* self, GdkEvent* event, Dialog* thiz),
		std::function<bool(GdkEvent* event)>,
		event
	)
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
	DEFINE_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus, bool)
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
	DEFINE_METHOD(widget, grab_focus, void)

	// プロパティ
	DEFINE_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus, bool)
	DEFINE_PROPERTY_ACCESSORS(Widget, sensitive, bool)
	DEFINE_PROPERTY_ACCESSORS(Widget, visible, bool)
	DEFINE_PROPERTY_ACCESSORS(Widget, editable, bool)

	// シグナル
	DEFINE_SIGNAL_HANDLER("activate", activate, Entry, GtkEntry* self)
};

/**
 * @class ToggleButton
 * @brief GtkToggleButtonのラッパークラス。オン/オフの状態を持つボタンを提供します。
 * @details sg::Buttonを継承し、アクティブ状態の取得/設定やtoggledシグナルの機能を追加します。
 */
class ToggleButton : public sgex::Button {
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
	DEFINE_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus, bool)

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
	DEFINE_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus, bool)

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
	DEFINE_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus, bool)

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
	DEFINE_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus, bool)
	DEFINE_PROPERTY_ACCESSORS(Widget, visible, bool)

	// シグナル
};

/**
 * @class ListStore
 * @brief sg::ListStoreを拡張し、機能を追加したクラス。
 */
class ListStore : public sg::ListStore {
public:
	using sg::ListStore::ListStore; // 基底クラスのコンストラクタを継承

	operator GtkTreeModel*() const { return GTK_TREE_MODEL((GtkListStore*)(*this)); }

	// メソッド
	DEFINE_METHOD_WITH_ARGS(tree_model, get_path, GtkTreePath*, (GtkTreeIter  *iter), iter)
	DEFINE_METHOD_WITH_ARGS(tree_model, iter_nth_child, gboolean, (GtkTreeIter  *iter, GtkTreeIter *parent, gint n), iter, parent, n)
	DEFINE_METHOD_WITH_ARGS(list_store, remove, gboolean, (GtkTreeIter *iter), iter)
	DEFINE_METHOD_WITH_ARGS(list_store, insert, void, (GtkTreeIter *iter, gint position), iter, position)
	DEFINE_METHOD_WITH_ARGS(list_store, append, void, (GtkTreeIter *iter), iter)

	/**
	 * @brief gtk_list_store_set の可変長テンプレートラッパー
	 * @details ラッパー内で args... の末尾に -1 を自動で付与するため、呼び出し側で末尾の -1 は不要
	 * @param iter 更新する行のイテレータ
	 * @param args GtkListStore に設定する列インデックスと値のペアの可変長引数
	 */
	template<typename... Args>
	void set(GtkTreeIter* iter, Args... args) {
		gtk_list_store_set((GtkListStore*)(*this), iter, args..., -1);
	}

	/**
	 * @brief sg::ListStore::append の可変長テンプレートラッパー
	 * @details ラッパー内で args... の末尾に -1 を自動で付与するため、呼び出し側で末尾の -1 は不要
	 */
	template<typename... Args>
	void append(Args... args) {
		sg::ListStore::append(args..., -1);
	}

	// プロパティ
	DEFINE_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus, bool)

	// シグナル
};

/**
 * @class TreePath
 * @brief GtkTreePathを管理するRAIIラッパークラス。
 * @details オブジェクトが破棄される際に自動的にgtk_tree_path_free()を呼び出します。
 */
class TreePath {
private:
	GtkTreePath* m_path;

	// メソッド
	DEFINE_METHOD(tree_path, free, void)

public:
	// GtkTreePath*からオブジェクトを構築
	TreePath(GtkTreePath* path) : m_path(path) {}

	// デストラクタでメモリを自動解放
	~TreePath() {
		if (m_path) {
			free();
		}
	}

	// コピーを禁止
	TreePath(const TreePath&) = delete;
	TreePath& operator=(const TreePath&) = delete;

	// ムーブを許可
	TreePath(TreePath&& other) noexcept : m_path(other.m_path) {
		other.m_path = nullptr; // 所有権を移動
	}
	TreePath& operator=(TreePath&& other) noexcept {
		if (this != &other) {
			if (m_path) free();
			m_path = other.m_path;
			other.m_path = nullptr;
		}
		return *this;
	}

	// GtkTreePath*へのキャスト演算子
	operator GtkTreePath*() const {
		return m_path;
	}

	// メソッド
	DEFINE_METHOD(tree_path, get_indices, gint*)

	// プロパティ

	// シグナル
};

/**
 * @class TreeSelection
 * @brief sg::TreeSelectionを拡張し、機能を追加したクラス。
 */
class TreeSelection : public sg::TreeSelection {
public:
	using sg::TreeSelection::TreeSelection; // 基底クラスのコンストラクタを継承

	// メソッド
	DEFINE_METHOD_WITH_ARGS(tree_selection, get_selected, gboolean, (GtkTreeModel **model, GtkTreeIter *iter), model, iter)

	// プロパティ

	// シグナル
};

/**
 * @class TreeViewColumn
 * @brief GtkTreeViewColumnのRAIIラッパークラス。
 * @details sg::Objectを継承し、カラムの生成とプロパティ設定をカプセル化します。
 */
class TreeViewColumn : public sg::Object {
public:
	// コンストラクタ
	TreeViewColumn() : sg::Object(G_OBJECT(gtk_tree_view_column_new())) {}
	TreeViewColumn(GtkTreeViewColumn* column) : sg::Object(G_OBJECT(column)) {}
	virtual ~TreeViewColumn() = default;

	// コピーは禁止
	TreeViewColumn(const TreeViewColumn&) = delete;
	TreeViewColumn& operator=(const TreeViewColumn&) = delete;

	// ムーブは許可
	TreeViewColumn(TreeViewColumn&& other) noexcept : sg::Object(std::move(other)) {}
	TreeViewColumn& operator=(TreeViewColumn&& other) noexcept {
		sg::Object::operator=(std::move(other));
		return *this;
	}

	// GtkTreeViewColumn*へのキャスト演算子
	operator GtkTreeViewColumn*() const {
		return GTK_TREE_VIEW_COLUMN(this->object);
	}

	// --- メソッド ---
	DEFINE_METHOD_WITH_ARGS(tree_view_column, set_title, void, (const char* title), title)
	DEFINE_METHOD_WITH_ARGS(tree_view_column, set_alignment, void, (gfloat xalign), xalign)
	DEFINE_METHOD_WITH_ARGS(tree_view_column, set_fixed_width, void, (int fixed_width), fixed_width)
	DEFINE_METHOD_WITH_ARGS(tree_view_column, pack_start, void, (GtkCellRenderer* cell, bool expand), cell, expand)
	DEFINE_METHOD_WITH_ARGS(tree_view_column, pack_end, void, (GtkCellRenderer* cell, bool expand), cell, expand)
	DEFINE_METHOD_WITH_ARGS(tree_view_column, add_attribute, void, (GtkCellRenderer* cell, const char* attribute, int column), cell, attribute, column)
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
	DEFINE_METHOD_WITH_ARGS(tree_view, get_column, GtkTreeViewColumn*, (gint n), n)
	DEFINE_METHOD_WITH_ARGS(tree_view, set_grid_lines, void, (GtkTreeViewGridLines grid_lines), grid_lines)

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
public:
	/// @brief インデックスやサイズを表すための型エイリアス
	using size_type = typename std::vector<RowStruct>::size_type;

protected:
	/// @brief (View) SimpleGtkでラップされたGtkListStoreの所有権を持つポインタ。
	std::unique_ptr<sgex::ListStore> m_list_store_ptr;
	/// @brief (Model) C++側のデータ本体。アプリケーションロジックの「信頼できる唯一の情報源」。
	std::vector<RowStruct> m_data_model;
	/// @brief RowStructからGtkListStoreの行データに変換するためのセッター関数。
	std::function<void(sgex::ListStore*, GtkTreeIter*, const RowStruct&)> m_row_setter;

	/**
	 * @brief sgex::ListStore へのポインタを取得します。
	 * @details ComboBox や TreeView にモデルとして設定する際に使用します。
	 * @return 内部で管理している sgex::ListStore へのポインタ。
	 */
	sgex::ListStore* get_sg_store() const { return m_list_store_ptr.get(); }

	/**
	 * @brief 内部のListStoreをGtkListStore*として取得するヘルパー関数。
	 * @details 複雑なキャストをこの関数内に隠蔽します。
	 * @return 内部で管理している sgex::ListStore から取得した GtkListStore へのポインタ。
	 */
	GtkListStore* get_gtk_store() const {
		if (!m_list_store_ptr) return nullptr;
		// 複雑なキャストはこの一箇所に集約
		return (GtkListStore*)(*m_list_store_ptr);
	}

	/**
	 * @brief 内部のListStoreをGtkTreeModel*として取得するヘルパー関数。
	 * @details 複雑なキャストをこの関数内に隠蔽します。
	 * @return 内部で管理している sgex::ListStore から取得した GtkTreeModel へのポインタ。
	 */
	GtkTreeModel* get_gtk_model() const {
		if (!m_list_store_ptr) return nullptr;
		// 複雑なキャストはこの一箇所に集約
		return (GtkTreeModel*)(*m_list_store_ptr);
	}

	/**
	 * @brief 保持しているセッター関数(m_row_setter)を呼び出して、GtkListStoreの行に値を設定します。
	 * @param iter 値を設定する行を指すGtkTreeIter。
	 * @param row_data セットするデータを持つRowStruct。
	 */
	void set_row_to_store(GtkTreeIter* iter, const RowStruct& row_data) {
		if (m_row_setter) {
			m_row_setter(get_sg_store(), iter, row_data);
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
		std::function<void(sgex::ListStore*, GtkTreeIter*, const RowStruct&)> row_setter
	) : m_row_setter(row_setter)
	{
		// GTypeのリストに基づいて、新しいGtkListStoreをプログラムで生成します。
		GtkListStore* new_gtk_store = gtk_list_store_newv(column_types.size(), (GType*)column_types.data());
		// 生成したGtkListStoreをSimpleGtkラッパーでラップし、unique_ptrで所有権を管理します。
		m_list_store_ptr = std::make_unique<sgex::ListStore>(new_gtk_store);
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
	 * @return 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
	 */
	bool append(const RowStruct& row_data) {
		if (size() >= (size_type)((size_type)(std::numeric_limits<int>::max()))) {
			return false; // 失敗: 行数がintの最大値を超える
		}
		m_data_model.push_back(row_data);
		GtkTreeIter iter;
		get_sg_store()->append(&iter);
		set_row_to_store(&iter, row_data);
		return true; // 成功
	}

	/**
	 * @brief 末尾に複数の行をまとめて追加します。
	 * @param rows 追加する行データのvector。
	 * @return 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
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
	 * size()以上の値を指定した場合は、末尾に追加(append)されます。
	 * @param position 0から始まる挿入位置のインデックス。
	 * @param row_data 挿入する行のデータ。
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool insert(size_type position, const RowStruct& row_data) {
		size_type original_position = position;
		bool position_was_clamped = false;

		if (size() >= (size_type)(std::numeric_limits<int>::max())) {
			return false; // 失敗: 行数がintの最大値を超える
		}
		if (position > m_data_model.size()) {
			position = m_data_model.size();
		}

		// 要求された位置と、実際の挿入位置が違うか判定
		if (position != original_position) {
			position_was_clamped = true;
		}

		m_data_model.insert(m_data_model.begin() + position, row_data);
		GtkTreeIter iter;
		get_sg_store()->insert(&iter, position);
		set_row_to_store(&iter, row_data);

		return !position_was_clamped;
	}

	/**
	 * @brief 指定した位置に複数の行をまとめて挿入します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * size()以上の値を指定した場合は、末尾に追加(append)されます。
	 * @param position 0から始まる挿入位置のインデックス。
	 * @param rows 挿入する行データのvector。
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool insert(size_type position, const std::vector<RowStruct>& rows) {
		if (size() + rows.size() > (size_type)(std::numeric_limits<int>::max())) {
			return false; // 失敗: 行数がintの最大値を超える
		}
		// 開始位置が有効範囲内([0, size()])にあるかチェック
		bool position_was_valid = (position <= size());

		size_type current_pos = position;
		for (const auto& row : rows) {
			// 単行insertは自動で位置を丸めてくれる
			insert(current_pos++, row);
		}
		return position_was_valid;
	}

	/**
	 * @brief 指定した位置の行を削除します。
	 * @param position 削除位置のインデックス。
	 * @return 削除に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool erase(size_type position) {
		if (position >= m_data_model.size()) {
			return false; // 失敗: インデックスが範囲外
		}

		m_data_model.erase(m_data_model.begin() + position);
		GtkTreeIter iter;
		if (get_sg_store()->iter_nth_child(&iter, nullptr, position)) {
			get_sg_store()->remove(&iter);
		}
		return true; // 成功
	}

	/**
	 * @brief 指定した複数の位置の行をまとめて削除します。
	 * @param positions 削除する行のインデックスのvector。
	 * @return 指定されたインデックスが全て有効な場合のみtrue。一つでも無効なら処理せずfalse。
	 */
	bool erase(std::vector<size_type> positions) {
		// ★ 先に全てのインデックスが有効かチェック
		for (size_type pos : positions) {
			if (pos >= size()) {
				return false; // 一つでも無効なインデックスがあれば失敗
			}
		}

		std::sort(positions.rbegin(), positions.rend());
		for (size_type pos : positions) {
			erase(pos);
		}
		return true; // 全て成功
	}

	/**
	 * @brief 指定した位置の行を上書きします。
	 * @param position 上書き位置のインデックス。
	 * @param row_data 新しい行データ。
	 * @return 更新に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool update(size_type position, const RowStruct& row_data) {
		if (position >= m_data_model.size()) {
			return false; // 失敗: インデックスが範囲外
		}

		m_data_model[position] = row_data;
		GtkTreeIter iter;
		if (get_sg_store()->iter_nth_child(&iter, nullptr, position)) {
			set_row_to_store(&iter, row_data);
		}

		return true; // 成功
	}

	/**
	 * @brief 指定した複数の位置の行をまとめて上書きします。
	 * @param updates 上書きする位置のインデックスと、新しい行データのmap。
	 * @return 指定されたインデックスが全て有効な場合のみtrue。一つでも無効なら処理せずfalse。
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
	 * @return 行データへのconst参照。
	 */
	const RowStruct& get_row(size_type position) const {
		return m_data_model.at(position);
	}

	/**
	 * @brief C++モデル内の全ての行データを取得します。
	 * @return 全ての行データを含むvectorのコピー。
	 */
	const std::vector<RowStruct>& get_data_model() const {
		return m_data_model;
	}

	/**
	 * @brief C++モデル内の現在の行数を返します。
	 * @return 現在の行数。
	 */
	size_type size() const {
		return m_data_model.size();
	}

	/**
	 * @brief 条件に一致する全ての行のインデックスを検索します。
	 * @param predicate 検索条件となる述語関数。
	 * @return 条件に一致した行のインデックスのリスト。
	 */
	std::vector<size_type> find_indices(std::function<bool(const RowStruct&)> predicate) const {
		std::vector<size_type> found_indices;
		for (size_t i = 0; i < m_data_model.size(); ++i) {
			if (predicate(m_data_model[i])) {
				found_indices.push_back(i);
			}
		}
		return found_indices;
	}

	/**
	 * @brief 条件に一致する全ての行データを検索します。
	 * @param predicate 検索条件となる述語関数。
	 * @return 条件に一致した行データのリスト。
	 */
	std::vector<RowStruct> find_rows(std::function<bool(const RowStruct&)> predicate) const {
		std::vector<RowStruct> found_rows;
		for (const auto& row : m_data_model) {
			if (predicate(row)) {
				found_rows.push_back(row);
			}
		}
		return found_rows;
	}

	/**
	 * @brief 条件に一致する全ての行のインデックスと行データのペアを検索します。
	 * @param predicate 検索条件となる述語関数。
	 * @return 条件に一致した行のインデックスと行データのペアのリスト。
	 */
	std::vector<std::pair<size_type, RowStruct>> find_indices_and_rows(std::function<bool(const RowStruct&)> predicate) const {
		std::vector<std::pair<size_type, RowStruct>> found_pairs;
		for (size_t i = 0; i < m_data_model.size(); ++i) {
			if (predicate(m_data_model[i])) {
				found_pairs.emplace_back(i, m_data_model[i]);
			}
		}
		return found_pairs;
	}

	/**
	 * @brief 条件に一致する最初の行のインデックスを検索します。
	 * @param predicate 検索条件となる述語関数。
	 * @return 条件に一致した最初の行のインデックス。
	 */
	std::optional<size_type> find_first_index(std::function<bool(const RowStruct&)> predicate) const {
		for (size_t i = 0; i < m_data_model.size(); ++i) {
			if (predicate(m_data_model[i])) {
				return i;
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief 条件に一致する最初の行データを検索します。
	 * @param predicate 検索条件となる述語関数。
	 * @return 条件に一致した最初の行データ。
	 */
	std::optional<RowStruct> find_first_row(std::function<bool(const RowStruct&)> predicate) const {
		for (const auto& row : m_data_model) {
			if (predicate(row)) {
				return row;
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief 条件に一致する最初の行のインデックスと行データのペアを検索します。
	 * @param predicate 検索条件となる述語関数。
	 * @return 条件に一致した最初の行のインデックスと行データのペア。
	 */
	std::optional<std::pair<size_type, RowStruct>> find_first_index_and_row(std::function<bool(const RowStruct&)> predicate) const {
		for (size_t i = 0; i < m_data_model.size(); ++i) {
			if (predicate(m_data_model[i])) {
				return std::make_pair(i, m_data_model[i]);
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
public:
	using size_type = typename ListStoreModelBase<RowStruct>::size_type;
private:
	/// @brief このモデルが関連付けられているTreeViewウィジェット。
	sgex::TreeView& m_tree_view;

public:
	/**
	 * @brief TreeViewListStoreModelのコンストラクタ。
	 * @param tree_view 管理対象のTreeViewウィジェット。
	 * @param column_types GtkListStoreが持つ列のGTypeのリスト。
	 * @param row_setter RowStructのデータをGtkListStoreの行に変換・設定する関数。
	 */
	TreeViewListStoreModel(
		sgex::TreeView& tree_view,
		const std::vector<GType>& column_types,
		std::function<void(sgex::ListStore*, GtkTreeIter*, const RowStruct&)> row_setter
	) : ListStoreModelBase<RowStruct>(column_types, row_setter), // 基底クラスのコンストラクタを呼び出し
		m_tree_view(tree_view)
	{
		// TreeViewに、基底クラスで生成された新しいListStoreモデルをセットします。
		m_tree_view.set_model(this->get_gtk_model());
	}

	// --- TreeView特化の機能 ---

	/**
	 * @brief 現在TreeViewで選択されている行のインデックスを取得します。
	 * @return 選択されている行のインデックス。なければ空のoptional。
	 */
	std::optional<size_type> get_selected_index() const {
		sgex::TreeSelection selection(m_tree_view.get_selection());
		GtkTreeIter iter;
		if (selection.get_selected(nullptr, &iter)) {
			sgex::TreePath path(this->get_sg_store()->get_path(&iter));
			if (!path) {
				return std::nullopt;
			}
			int index = path.get_indices()[0];
			return index;
		}
		return std::nullopt;
	}

	/**
	 * @brief 現在選択されている行のデータをC++モデルから取得します。
	 * @return 選択されていれば行データ、なければ空のoptional。
	 */
	std::optional<RowStruct> get_selected_row() const {
		auto index_opt = get_selected_index();
		if (index_opt.has_value()) {
			return this->get_row(index_opt.value());
		}
		return std::nullopt;
	}

	/**
	 * @brief 現在選択されている行のインデックスと行データのペアを取得します。
	 * @return 選択されていればインデックスと行データのペア、なければ空のoptional。
	 */
	std::optional<std::pair<size_type, RowStruct>> get_selected_index_and_row() const {
		auto index_opt = get_selected_index();
		if (index_opt.has_value()) {
			return std::make_pair(index_opt.value(), this->get_row(index_opt.value()));
		}
		return std::nullopt;
	}
};

/**
 * @class PaginatedListStoreModel
 * @brief 複数のTreeViewListStoreModelを1ページとして扱い、ページネーション機能を提供するクラス。
 * @details 全データ(マスターデータ)を内部に保持し、データ操作とページ表示を完全にカプセル化します。
 * @tparam RowStruct 1行分のデータを表現する構造体の型。
 */
template<typename RowStruct>
class PaginatedListStoreModel {
public:
	using size_type = typename std::vector<RowStruct>::size_type;

private:
	/// @brief (Model) 全てのページのマスターデータ
	std::vector<RowStruct> m_master_data;
	/// @brief 1つのTreeViewが表示できる最大行数
	size_t m_rows_per_view;
	/// @brief 1ページに含まれるTreeViewListStoreModelのリスト
	std::vector<TreeViewListStoreModel<RowStruct>*> m_page_models;
	/// @brief 現在表示中のページ番号 (0から始まるインデックス)
	int m_current_page_index;

public:
	/**
	 * @brief 指定されたページのデータを各TreeViewに表示する内部関数。
	 * @param page_index 表示するページのインデックス (0から始まる)。
	 */
	void display_page(int page_index) {
		// 全ての管理下モデルをクリア
		for (auto* model : m_page_models) {
			model->clear();
		}

		int total_pages = get_total_pages(); // 現在の総ページ数を取得

		// ページインデックスが範囲外かチェックし、範囲内に丸める (Clamp)
		if (page_index < 0) {
			page_index = 0; // 0未満なら0ページ目に
		} else if (page_index >= total_pages) {
			// 総ページ数以上なら最終ページに (ページがない場合は0)
			page_index = std::max(0, total_pages - 1);
		}

		// 1ページあたりが表示できる合計行数
		size_t total_rows_per_page = m_rows_per_view * m_page_models.size();

		if (total_rows_per_page == 0) { // モデルが登録されていない場合は何もしない
			m_current_page_index = 0; // ページ番号をリセット
			return;
		}

		// 表示を開始するマスターデータのインデックス
		size_t start_index = page_index * total_rows_per_page;

		// マスターデータから現在のページに表示すべき部分を切り出して表示
		for (size_t i = 0; i < total_rows_per_page; i++) {
			size_t master_index = start_index + i;
			if (master_index >= m_master_data.size()) { break; }	// マスターデータの終端を超えたらループ終了

			// どのTreeViewに表示するかを計算
			int view_index = i / m_rows_per_view;
			if (view_index < m_page_models.size()) {	// 念のため範囲チェック
				m_page_models[view_index]->append(m_master_data[master_index]);
			}
		}
		m_current_page_index = page_index;
	}

	/**
	 * @brief コンストラクタ
	 * @param rows_per_view 1つのTreeViewが保持できる最大行数。
	 */
	PaginatedListStoreModel(size_t rows_per_view)
		: m_rows_per_view(rows_per_view), m_current_page_index(0) {}

	/**
	 * @brief このページネーション管理に追加するTreeViewListStoreModelを登録します。
	 * @param model 管理対象のモデルへのポインタ。
	 */
	void add_model(TreeViewListStoreModel<RowStruct>* model) {
		m_page_models.push_back(model);
	}

	/**
	 * @brief 複数のTreeViewListStoreModelを一括で登録します。
	 * @param models 管理対象のモデルへのポインタのvector。
	 */
	void add_model(const std::vector<TreeViewListStoreModel<RowStruct>*>& models) {
		for (auto* model : models) {
			add_model(model);
		}
	}

	/**
	 * @brief 登録されている全てのTreeViewListStoreModelをクリアします。
	 * @details マスターデータが空のときのみクリアを許可します。
	 * @return クリアに成功した場合はtrue、マスターデータが空でない場合はfalse。
	 */
	bool clear_models() {
		// マスターデータが空のときのみクリアを許可
		if (!m_master_data.empty()) return false;
		m_page_models.clear();
		return true;
	}

	/**
	 * @brief 全てのページの元となるマスターデータをコピーしてセットします。
	 * @details この関数を呼び出すと、データが内部にコピーされ、最初のページが表示されます。
	 * @param all_data 全てのデータを含むvector (コピー元)。
	 * @param refresh_current_page (オプション) trueの場合、編集後に現在のページを再表示しようと試みます。
	 */
	void set_master_data(const std::vector<RowStruct>& all_data, bool refresh_current_page = false) {
		m_master_data = all_data; // コピーが発生
		display_page(((refresh_current_page) ? (m_current_page_index) : (0)));
	}

	/**
	 * @brief 全てのページの元となるマスターデータをムーブしてセットします。
	 * @details この関数を呼び出すと、データの所有権が効率的に移動され、最初のページが表示されます。
	 * 呼び出し元のvectorは空になります。
	 * @param all_data 全てのデータを含むvector (ムーブ元)。
	 * @param refresh_current_page (オプション) trueの場合、編集後に現在のページを再表示しようと試みます。
	 */
	void set_master_data(std::vector<RowStruct>&& all_data, bool refresh_current_page = false) {
		m_master_data = std::move(all_data); // ムーブ (効率的)
		display_page(((refresh_current_page) ? (m_current_page_index) : (0)));
	}

	/**
	 * @brief マスターデータに新しいデータセットをコピーし、古いマスターデータを効率的に返します。
	 * @details
	 * 内部のマスターデータに `new_data` をコピーします。
	 * 置き換え前の古いマスターデータはムーブされて返されます。
	 * 操作後、UIは自動的に最初のページ（0ページ目）を表示するように更新されます。
	 * @param new_data 新しく設定するマスターデータ。関数内でマスターデータにコピーされます。
	 * @param refresh_current_page (オプション) trueの場合、編集後に現在のページを再表示しようと試みます。
	 * @return 置き換えられる前の古いマスターデータ (ムーブされて返される)。
	 */
	std::vector<RowStruct> replace_master_data(const std::vector<RowStruct>& new_data, bool refresh_current_page = false) {
		// 1. 現在のマスターデータを一時変数にムーブして退避
		std::vector<RowStruct> old_data = std::move(m_master_data);

		// 2. 引数で受け取った新しいデータをマスターデータにコピー
		m_master_data = new_data;

		// 3. UIを更新
		display_page(((refresh_current_page) ? (m_current_page_index) : (0)));

		// 4. 退避しておいた古いデータを返す (これもムーブされる)
		return old_data;
	}

	/**
	 * @brief マスターデータを新しいデータセットに置き換え、古いマスターデータを効率的に返します。
	 * @details
	 * 内部のマスターデータを `new_data` に置き換えます。この操作はムーブセマンティクスを利用するため、
	 * データのコピーは最小限に抑えられます。置き換え前の古いマスターデータはムーブされて返されます。
	 * 操作後、UIは自動的に最初のページ（0ページ目）を表示するように更新されます。
	 * @param new_data 新しく設定するマスターデータ (ムーブ元)。この関数の呼び出し後、`new_data` は空（または有効だが未定義の状態）になります。
	 * @param refresh_current_page (オプション) trueの場合、編集後に現在のページを再表示しようと試みます。
	 * @return 置き換えられる前の古いマスターデータ (ムーブされて返される)。
	 */
	std::vector<RowStruct> replace_master_data(std::vector<RowStruct>&& new_data, bool refresh_current_page = false) {
		// 1. 現在のマスターデータを一時変数にムーブして退避
		std::vector<RowStruct> old_data = std::move(m_master_data);

		// 2. 引数で受け取った新しいデータをマスターデータにムーブ
		m_master_data = std::move(new_data);

		// 3. UIを更新
		display_page(((refresh_current_page) ? (m_current_page_index) : (0)));

		// 4. 退避しておいた古いデータを返す (これもムーブされる)
		return old_data;
	}

	/**
	 * @brief 次のページへ切り替えます。
	 * @return 次のページが存在し、切り替えに成功した場合はtrue。
	 */
	bool next_page() {
		if (has_next_page()) {
			display_page(m_current_page_index + 1);
			return true;
		}
		return false;
	}

	/**
	 * @brief 前のページへ切り替えます。
	 * @return 前のページが存在し、切り替えに成功した場合はtrue。
	 */
	bool prev_page() {
		if (has_prev_page()) {
			display_page(m_current_page_index - 1);
			return true;
		}
		return false;
	}

	// --- 状態取得関数 ---

	/**
	 * @brief 現在の総ページ数を取得します。
	 * @return 総ページ数 (1から始まる)。
	 */
	int get_total_pages() const {
		if (m_master_data.empty()) return 1;
		size_t total_rows_per_page = m_rows_per_view * m_page_models.size();
		if (total_rows_per_page == 0) return 1;
		return (m_master_data.size() + total_rows_per_page - 1) / total_rows_per_page;
	}

	/**
	 * @brief 現在表示中のページ番号を取得します (1から始まる)。
	 * @return 現在のページ番号。
	 */
	int get_current_page() const {
		return m_current_page_index + 1;
	}

	/**
	 * @brief 次のページが存在するかを返します。
	 * @return 次のページが存在する場合はtrue。
	 */
	bool has_next_page() const {
		return m_current_page_index < (get_total_pages() - 1);
	}

	/**
	 * @brief 前のページが存在するかを返します。
	 * @return 前のページが存在する場合はtrue。
	 */
	bool has_prev_page() const {
		return m_current_page_index > 0;
	}

	// --- データ操作・検索メソッド ---

	/**
	 * @brief マスターデータと表示中の全データをクリアします。
	 * @param update_display 画面を更新するかどうか
	 */
	void clear(bool update_display = true) {
		m_master_data.clear();
		if (update_display) {
			display_page(0); // 0ページ目を表示（空になる）
		}
	}

	/**
	 * @brief マスターデータの末尾に単一の行を追加し、UIを更新します。
	 * @param row_data 追加する行のデータ。
	 * @param update_display 画面を更新するかどうか
	 * @return 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
	 */
	bool append(const RowStruct& row_data, bool update_display = true) {
		if (size() >= (size_type)(std::numeric_limits<int>::max())) return false;
		m_master_data.push_back(row_data);
		if (update_display) {
			display_page(m_current_page_index);
		}
		return true;
	}

	/**
	 * @brief マスターデータの末尾に複数の行をまとめて追加し、UIを更新します。
	 * @param rows 追加する行データのvector。
	 * @param update_display 画面を更新するかどうか
	 * @return 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
	 */
	bool append(const std::vector<RowStruct>& rows, bool update_display = true) {
		if (size() + rows.size() > (size_type)(std::numeric_limits<int>::max())) return false;
		m_master_data.insert(m_master_data.end(), rows.begin(), rows.end());
		if (update_display) {
			display_page(m_current_page_index);
		}
		return true;
	}

	/**
	 * @brief マスターデータの指定したグローバルインデックスに行を挿入し、UIを更新します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * size()以上の値を指定した場合は、末尾に追加(append)されます。
	 * @param global_position 挿入位置のグローバルインデックス。
	 * @param row_data 挿入する行のデータ。
	 * @param update_display 画面を更新するかどうか
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool insert(size_type global_position, const RowStruct& row_data, bool update_display = true) {
		size_type original_position = global_position;
		if (size() >= (size_type)(std::numeric_limits<int>::max())) return false;

		if (global_position > m_master_data.size()) global_position = m_master_data.size();

		m_master_data.insert(m_master_data.begin() + global_position, row_data);
		if (update_display) {
			display_page(m_current_page_index);
		}
		return global_position == original_position;
	}

	/**
	 * @brief マスターデータの指定したグローバルインデックスに複数の行をまとめて挿入し、UIを更新します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * size()以上の値を指定した場合は、末尾に追加(append)されます。
	 * @param global_position 挿入位置のグローバルインデックス。
	 * @param rows 挿入する行データのvector。
	 * @param update_display 画面を更新するかどうか
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool insert(size_type global_position, const std::vector<RowStruct>& rows, bool update_display = true) {
		if (size() + rows.size() > (size_type)(std::numeric_limits<int>::max())) return false;
		bool position_was_valid = (global_position <= size());

		if (global_position > size()) global_position = size();

		m_master_data.insert(m_master_data.begin() + global_position, rows.begin(), rows.end());
		if (update_display) {
			display_page(m_current_page_index);
		}
		return position_was_valid;
	}

	/**
	 * @brief マスターデータの指定したグローバルインデックスの行を削除し、UIを更新します。
	 * @param global_position 削除する行のグローバルインデックス。
	 * @param update_display 画面を更新するかどうか
	 * @return 削除に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool erase(size_type global_position, bool update_display = true) {
		if (global_position >= size()) return false;
		m_master_data.erase(m_master_data.begin() + global_position);

		if (m_current_page_index >= get_total_pages() && m_current_page_index > 0) {
			m_current_page_index--;
		}
		if (update_display) {
			display_page(m_current_page_index);
		}
		return true;
	}

	/**
	 * @brief マスターデータの指定した複数のグローバルインデックスの行をまとめて削除します。
	 * @param global_positions 削除する行のグローバルインデックスのリスト。
	 * @param update_display 画面を更新するかどうか
	 * @return 指定されたインデックスが全て有効な場合のみtrue。一つでも無効なら処理せずfalse。
	 */
	bool erase(std::vector<size_type> global_positions, bool update_display = true) {
		for (size_type pos : global_positions) {
			if (pos >= size()) return false;
		}
		std::sort(global_positions.rbegin(), global_positions.rend());
		for (size_type pos : global_positions) {
			m_master_data.erase(m_master_data.begin() + pos);
		}
		if (m_current_page_index >= get_total_pages() && m_current_page_index > 0) {
			m_current_page_index = get_total_pages() - 1;
		}
		if (update_display) {
			display_page(m_current_page_index);
		}
		return true;
	}

	/**
	 * @brief マスターデータの指定したグローバルインデックスの行を上書きし、UIを更新します。
	 * @details 変更対象の行が現在表示中のページにあれば、その行だけを再描画します。
	 * @param global_position 上書きする行のグローバルインデックス。
	 * @param row_data 新しい行データ。
	 * @return 更新に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool update(size_type global_position, const RowStruct& row_data) {
		if (global_position >= size()) return false;

		// 1. マスターデータを更新
		m_master_data[global_position] = row_data;

		// 2. 更新した行が現在表示中のページにあるか判定
		size_t total_rows_per_page = m_rows_per_view * m_page_models.size();
		size_t page_start_index = m_current_page_index * total_rows_per_page;
		size_t page_end_index = page_start_index + total_rows_per_page;

		if (global_position >= page_start_index && global_position < page_end_index) {
			// 3. 表示中の場合、ローカルインデックスを計算してピンポイントで更新
			size_t local_position_on_page = global_position - page_start_index;
			int view_index = local_position_on_page / m_rows_per_view;
			int local_row_index = local_position_on_page % m_rows_per_view;

			// 対象のTreeViewListStoreModelのupdateを呼び出す
			m_page_models[view_index]->update(local_row_index, row_data);
		}

		return true;
	}

	/**
	 * @brief マスターデータの指定した複数のグローバルインデックスの行をまとめて上書きします。
	 * @param updates 上書きする行のグローバルインデックスと新しい行データのmap。
	 * @param update_display 画面を更新するかどうか
	 * @return 指定されたインデックスが全て有効な場合のみtrue。一つでも無効なら処理せずfalse。
	 */
	bool update(const std::map<size_type, RowStruct>& updates, bool update_display = true) {
		for (const auto& [pos, row_data] : updates) {
			if (pos >= size()) return false;
		}
		for (const auto& [pos, row_data] : updates) {
			m_master_data[pos] = row_data;
		}
		if (update_display) {
			display_page(m_current_page_index);
		}
		return true;
	}


	/**
	 * @brief マスターデータを直接編集するための関数オブジェクトを受け付けます。
	 * @details
	 * この関数に渡されたラムダ式などの関数オブジェクト内で、マスターデータ(vector)を
	 * 直接変更できます (ソート、フィルタリング、要素の変更など)。
	 * 関数オブジェクトの実行が完了した後、自動的にUIが更新されます。
	 * @param modifier マスターデータへの非const参照を受け取り、編集を行う関数オブジェクト。
	 * @param refresh_current_page (オプション) trueの場合、編集後に現在のページを再表示しようと試みます。
	 * ページが存在しなくなっていれば最終ページが表示されます。falseの場合は0ページ目を表示します。デフォルトはfalse。
	 */
	void modify_master_data(std::function<void(std::vector<RowStruct>&)> modifier, bool refresh_current_page = false) {
		// 1. 渡された関数を実行し、m_master_data を直接変更してもらう
		modifier(m_master_data);

		// 2. 変更が完了したら、表示を更新する
		// display_page が範囲チェックと丸めを行うため、ここでは単純にページ番号を渡すだけでOK
		display_page(((refresh_current_page) ? (m_current_page_index) : (0)));
	}

	/**
	 * @brief 指定したグローバルインデックスの行データをマスターデータから取得します。
	 * @param global_position 取得する行のグローバルインデックス。
	 * @return 行データへのconst参照。
	 */
	const RowStruct& get_row(size_type global_position) const {
		return m_master_data.at(global_position);
	}

	/**
	 * @brief マスターデータ内の全ての行データを取得します。
	 * @return 全ての行データを含むvectorのコピー。
	 */
	const std::vector<RowStruct>& get_master_data() const {
		return m_master_data;
	}

	/// @brief マスターデータ内の現在の行数を返します。
	size_type size() const {
		return m_master_data.size();
	}

	/**
	 * @brief 条件に一致する全ての行のグローバルインデックスを検索します。
	 *
	 * @code
	 * // 例: ageフィールドが30以上の行のインデックスを取得
	 * auto indices = find_indices([](const RowStruct& row) {
	 *     return row.age >= 30;
	 * });
	 * @endcode
	 *
	 * @param predicate 検索条件となる述語関数。
	 * @return 条件に一致した行のグローバルインデックスのリスト。
	 */
	std::vector<size_type> find_indices(std::function<bool(const RowStruct&)> predicate) const {
		std::vector<size_type> found_indices;
		for (size_t i = 0; i < m_master_data.size(); ++i) {
			if (predicate(m_master_data[i])) {
				found_indices.push_back(i);
			}
		}
		return found_indices;
	}

	/**
	 * @brief 条件に一致する全ての行データを検索します。
	 * @param predicate 検索条件となる述語関数。
	 * @return 条件に一致した行データのリスト。
	 */
	std::vector<RowStruct> find_rows(std::function<bool(const RowStruct&)> predicate) const {
		std::vector<RowStruct> found_rows;
		for (const auto& row : m_master_data) {
			if (predicate(row)) {
				found_rows.push_back(row);
			}
		}
		return found_rows;
	}

	/**
	 * @brief 条件に一致する全ての行のグローバルインデックスと行データのペアを検索します。
	 * @param predicate 検索条件となる述語関数。
	 * @return 条件に一致した行のグローバルインデックスと行データのペアのリスト。
	 */
	std::vector<std::pair<size_type, RowStruct>> find_indices_and_rows(std::function<bool(const RowStruct&)> predicate) const {
		std::vector<std::pair<size_type, RowStruct>> found_pairs;
		for (size_type i = 0; i < m_master_data.size(); ++i) {
			if (predicate(m_master_data[i])) {
				found_pairs.emplace_back(i, m_master_data[i]);
			}
		}
		return found_pairs;
	}

	/**
	 * @brief 条件に一致する最初の行のグローバルインデックスを検索します。
	 * @param predicate 検索条件となる述語関数。
	 * @return 条件に一致した最初の行のグローバルインデックス。
	 */
	std::optional<size_type> find_first_index(std::function<bool(const RowStruct&)> predicate) const {
		for (size_type i = 0; i < m_master_data.size(); ++i) {
			if (predicate(m_master_data[i])) {
				return i;
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief 条件に一致する最初の行データを検索します。
	 * @param predicate 検索条件となる述語関数。
	 * @return 条件に一致した最初の行データ。
	 */
	std::optional<RowStruct> find_first_row(std::function<bool(const RowStruct&)> predicate) const {
		for (const auto& row : m_master_data) {
			if (predicate(row)) {
				return row;
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief 条件に一致する最初の行のグローバルインデックスと行データのペアを検索します。
	 * @param predicate 検索条件となる述語関数。
	 * @return 条件に一致した最初のグローバルインデックスと行データのペア。
	 */
	std::optional<std::pair<size_type, RowStruct>> find_first_index_and_row(std::function<bool(const RowStruct&)> predicate) const {
		for (size_type i = 0; i < m_master_data.size(); ++i) {
			if (predicate(m_master_data[i])) {
				return std::make_pair(i, m_master_data[i]);
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief 現在表示されているページで選択されている行のグローバルインデックスを取得します。
	 * @return 選択されていればグローバルインデックス、なければ空のoptional。
	 */
	std::optional<size_type> get_selected_index() const {
		size_t total_rows_per_page = m_rows_per_view * m_page_models.size();
		size_t page_start_index = m_current_page_index * total_rows_per_page;

		// 管理している全てのTreeViewをチェック
		for (size_t i = 0; i < m_page_models.size(); ++i) {
			auto local_index_opt = m_page_models[i]->get_selected_index();
			if (local_index_opt.has_value()) {
				int local_index = local_index_opt.value();
				// TreeView内での開始インデックスを計算
				size_t view_start_index = i * m_rows_per_view;
				// グローバルインデックスを計算して返す
				return page_start_index + view_start_index + local_index;
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief 現在選択されている行のデータをマスターデータから取得します。
	 * @return 選択されていれば行データ、なければ空のoptional。
	 */
	std::optional<RowStruct> get_selected_row() const {
		auto index_opt = get_selected_index();
		if (index_opt.has_value()) {
			return get_row(index_opt.value());
		}
		return std::nullopt;
	}

	/**
	 * @brief 現在選択されている行のグローバルインデックスと行データのペアを取得します。
	 * @return 選択されていればグローバルインデックスと行データのペア、なければ空のoptional。
	 */
	std::optional<std::pair<size_type, RowStruct>> get_selected_index_and_row() const {
		auto index_opt = get_selected_index();
		if (index_opt.has_value()) {
			return std::make_pair(index_opt.value(), get_row(index_opt.value()));
		}
		return std::nullopt;
	}
};
} // namespace sgex
