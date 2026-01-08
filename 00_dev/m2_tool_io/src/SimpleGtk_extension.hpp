/**
 * @file SimpleGtk_extension.hpp
 * @brief SimpleGtk を拡張する、gtkのラッパークラスをまとめたヘッダファイル。
 * @details ToggleButton, Stack, Boxの3クラスを提供します。
 */
#pragma once
#include <optional>
#include <SimpleGtk>
#include "./utility.hpp"

/**
 * @brief 戻り値を持つシグナルのハンドラを定義するマクロ。
 * @param[in] signal_name GTKシグナル名 (文字列)。
 * @param[in] c_signal_name C++の関数名として使用する名前。
 * @param[in] return_type ハンドラの戻り値の型 (例: gboolean)。
 * @param[in] para_static 静的コールバック関数の引数リスト。
 * @param[in] function std::function の型。
 * @param[in] ... std::function に渡す引数。
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
 * @param[in] gtk_property_name GTKオブジェクトのプロパティ名 (例: "can-focus")。文字列リテラルで指定します。
 * @param[in] cpp_suffix 生成されるC++関数名の一部 (例: can_focus)。識別子として有効な名前を指定します。
 * ゲッターは `get_<cpp_suffix>()`、セッターは `set_<cpp_suffix>()` という名前になります。
 * @param[in] PropertyType プロパティのC++におけるデータ型 (例: bool, int, const char*)。
 *
 * ```cpp
 * // 使用例
 * // クラス定義内で使用
 * DEFINE_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus, bool)
 * // これにより、以下のメンバ関数が自動的に定義されます:
 * // bool get_can_focus() const;
 * // void set_can_focus(bool value);
 * ```
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
 * @brief bool型プロパティのゲッターとセッターを定義するマクロ。
 * @details
 * g_object_get/set関数は gboolean 型 (4バイト整数) を使用しますが、
 * C++コードでは bool 型 (1バイト) を使用することが一般的です。
 * このマクロは、gboolean と bool の間で自動的に変換を行うゲッターとセッターを定義します。
 *
 * @param[in] property_name GTKオブジェクトのプロパティ名 (例: visible)。
 *
 * ```cpp
 * // 使用例
 * // クラス定義内で使用
 * DEFINE_BOOL_PROPERTY_ACCESSORS(visible)
 * // これにより、以下のメンバ関数が自動的に定義されます:
 * // bool get_visible() const;
 * // void set_visible(bool value);
 * ```
 */
#define DEFINE_BOOL_PROPERTY_ACCESSORS(property_name) \
	bool get_##property_name() const { \
		gboolean value; \
		g_object_get(G_OBJECT(this->object), #property_name, &value, NULL); \
		return (bool)value; \
	} \
	void set_##property_name(bool value) { \
		g_object_set(G_OBJECT(this->object), #property_name, (gboolean)value, NULL); \
	}

// 名前指定版（can-focusなどハイフン付き用）
/**
 * @brief GTKプロパティ名にハイフンが含まれる場合に対応した、bool型プロパティのゲッターとセッターを定義するマクロ。
 * @details
 * g_object_get/set関数は gboolean 型 (4バイト整数) を使用しますが、
 * C++コードでは bool 型 (1バイト) を使用することが一般的です。
 * このマクロは、gboolean と bool の間で自動的に変換を行うゲッターとセッターを定義します。
 * また、GTKプロパティ名とC++関数名の一部を別々に指定できるようにすることで、
 * ハイフンを含むプロパティ名にも対応します。
 *
 * @param[in] gtk_property_name GTKオブジェクトのプロパティ名 (例: "can-focus")。文字列リテラルで指定します。
 * @param[in] cpp_suffix 生成されるC++関数名の一部 (例: can_focus)。識別子として有効な名前を指定します。
 *
 * ```cpp
 * // 使用例
 * // クラス定義内で使用
 * DEFINE_BOOL_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus)
 * // これにより、以下のメンバ関数が自動的に定義されます:
 * // bool get_can_focus() const;
 * // void set_can_focus(bool value);
 * ```
 */
#define DEFINE_BOOL_PROPERTY_ACCESSORS_WITH_NAME(gtk_property_name, cpp_suffix) \
	bool get_##cpp_suffix() const { \
		gboolean value; \
		g_object_get(G_OBJECT(this->object), gtk_property_name, &value, NULL); \
		return (bool)value; \
	} \
	void set_##cpp_suffix(bool value) { \
		g_object_set(G_OBJECT(this->object), gtk_property_name, (gboolean)value, NULL); \
	}

#define DEFINE_CSS_CLASS_ACCESSORS() \
	void add_css_class(const gchar* class_name) { \
		gtk_style_context_add_class(*this, class_name); \
	} \
	void add_css_class(const std::string& class_name) { \
		add_css_class(class_name.c_str()); \
	} \
	void remove_css_class(const gchar* class_name) { \
		gtk_style_context_remove_class(*this, class_name); \
	} \
	void remove_css_class(const std::string& class_name) { \
		remove_css_class(class_name.c_str()); \
	} \
	gboolean has_css_class(const gchar* class_name) { \
		return gtk_style_context_has_class(*this, class_name); \
	} \
	gboolean has_css_class(const std::string& class_name) { \
		return has_css_class(class_name.c_str()); \
	}

/**
 * @namespace sgex
 * @brief SimpleGtk を拡張する、gtkのラッパークラスを格納する名前空間。
 */
namespace sgex {

/**
 * @struct Position
 * @brief 2次元座標を表す構造体。
 */
struct Position {
	gint x;
	gint y;
};

/**
 * @class Window
 * @brief sg::Windowを拡張し、機能を追加したクラス。
 */
class Window : public sg::Window {
public:
	using sg::Window::Window; // 基底クラスのコンストラクタを継承

	// メソッド
	DEFINE_METHOD(widget, get_mapped, gboolean)
	DEFINE_METHOD(window, fullscreen, void)
	DEFINE_METHOD_WITH_ARGS(window, set_keep_above, void, (gboolean setting), setting)
	DEFINE_METHOD_WITH_ARGS(window, move, void, (gint x, gint y), x, y)
	DEFINE_METHOD_WITH_ARGS(window, get_position, void, (gint* root_x, gint* root_y), root_x, root_y)
	DEFINE_CSS_CLASS_ACCESSORS()

	/**
	 * @brief ウィンドウを指定した位置に移動します。
	 * @param[in] pos 移動先の位置を表す Position 構造体。
	 */
	void move(const Position& pos) {
		this->move(pos.x, pos.y);
	}

	/**
	 * @brief ウィンドウを現在の位置から指定したオフセット分だけ移動します。
	 * @param[in] pos 移動量を表す Position 構造体。
	 */
	void move_by(const Position& pos) {
		Position current_pos = this->get_position();
		this->move(current_pos.x + pos.x, current_pos.y + pos.y);
	}

	/**
	 * @brief ウィンドウを現在の位置から指定したオフセット分だけ移動します。
	 * @param[in] x 移動量のX成分。
	 * @param[in] y 移動量のY成分。
	 */
	void move_by(gint x, gint y) {
		Position current_pos = this->get_position();
		this->move(current_pos.x + x, current_pos.y + y);
	}

	/**
	 * @brief ウィンドウの現在の位置を取得します。
	 * @return ウィンドウの位置を表す Position 構造体。
	 */
	Position get_position() {
		Position pos;
		this->get_position(&pos.x, &pos.y);
		return pos;
	}

	// プロパティ
	DEFINE_BOOL_PROPERTY_ACCESSORS(visible)
	DEFINE_PROPERTY_ACCESSORS_WITH_NAME("transient-for", transient_for, GtkWindow*)
	DEFINE_BOOL_PROPERTY_ACCESSORS(modal)
	DEFINE_PROPERTY_ACCESSORS_WITH_NAME("type-hint", type_hint, GdkWindowTypeHint)
	DEFINE_BOOL_PROPERTY_ACCESSORS(deletable)

	// シグナル
	DEFINE_SIGNAL_HANDLER("map", map, Window, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER("show", show, Window, GtkWidget* self)
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
	DEFINE_METHOD(widget, get_mapped, gboolean)
	DEFINE_METHOD(window, fullscreen, void)
	DEFINE_METHOD_WITH_ARGS(window, set_keep_above, void, (gboolean setting), setting)
	DEFINE_METHOD_WITH_ARGS(window, move, void, (gint x, gint y), x, y)
	DEFINE_METHOD_WITH_ARGS(window, get_position, void, (gint* root_x, gint* root_y), root_x, root_y)
	DEFINE_METHOD_WITH_ARGS(dialog, response, void, (gint response_id), response_id)
	DEFINE_CSS_CLASS_ACCESSORS()

	/**
	 * @brief ウィンドウを指定した位置に移動します。
	 * @param[in] pos 移動先の位置を表す Position 構造体。
	 */
	void move(const Position& pos) {
		this->move(pos.x, pos.y);
	}

	/**
	 * @brief ウィンドウを現在の位置から指定したオフセット分だけ移動します。
	 * @param[in] pos 移動量を表す Position 構造体。
	 */
	void move_by(const Position& pos) {
		Position current_pos = this->get_position();
		this->move(current_pos.x + pos.x, current_pos.y + pos.y);
	}

	/**
	 * @brief ウィンドウを現在の位置から指定したオフセット分だけ移動します。
	 * @param[in] x 移動量のX成分。
	 * @param[in] y 移動量のY成分。
	 */
	void move_by(gint x, gint y) {
		Position current_pos = this->get_position();
		this->move(current_pos.x + x, current_pos.y + y);
	}

	/**
	 * @brief ウィンドウの現在の位置を取得します。
	 * @return ウィンドウの位置を表す Position 構造体。
	 */
	Position get_position() {
		Position pos;
		this->get_position(&pos.x, &pos.y);
		return pos;
	}

	// プロパティ
	DEFINE_BOOL_PROPERTY_ACCESSORS(visible)
	DEFINE_PROPERTY_ACCESSORS_WITH_NAME("transient-for", transient_for, GtkWindow*)
	DEFINE_BOOL_PROPERTY_ACCESSORS(modal)
	DEFINE_PROPERTY_ACCESSORS_WITH_NAME("type-hint", type_hint, GdkWindowTypeHint)
	DEFINE_BOOL_PROPERTY_ACCESSORS(deletable)

	// シグナル
	DEFINE_SIGNAL_HANDLER("map", map, Dialog, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER("show", show, Dialog, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER_WIDTH_ARGS("response", response, (GtkDialog* self, gint response_id, Dialog* thiz), std::function<void(gint)>, response_id)
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
	DEFINE_CSS_CLASS_ACCESSORS()

	// プロパティ
	DEFINE_BOOL_PROPERTY_ACCESSORS(visible)

	// シグナル
	DEFINE_SIGNAL_HANDLER("map", map, Box, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER("show", show, Box, GtkWidget* self)
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
	DEFINE_CSS_CLASS_ACCESSORS()

	// プロパティ
	DEFINE_BOOL_PROPERTY_ACCESSORS(visible)

	// シグナル
	DEFINE_SIGNAL_HANDLER("map", map, Stack, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER("show", show, Stack, GtkWidget* self)
};

/**
 * @class Button
 * @brief sg::Buttonを拡張し、機能を追加したクラス。
 */
class Button : public sg::Button {
public:
	using sg::Button::Button; // 基底クラスのコンストラクタを継承

	// メソッド
	DEFINE_CSS_CLASS_ACCESSORS()

	// プロパティ
	DEFINE_BOOL_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus)
	DEFINE_BOOL_PROPERTY_ACCESSORS(sensitive)
	DEFINE_BOOL_PROPERTY_ACCESSORS(visible)

	// シグナル
	DEFINE_SIGNAL_HANDLER("map", map, Button, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER("show", show, Button, GtkWidget* self)
};

/**
 * @class CheckButton
 * @brief sg::CheckButtonを拡張し、機能を追加したクラス。
 */
class CheckButton : public sg::CheckButton {
public:
	using sg::CheckButton::CheckButton; // 基底クラスのコンストラクタを継承

	// メソッド
	DEFINE_CSS_CLASS_ACCESSORS()

	// プロパティ
	DEFINE_BOOL_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus)
	DEFINE_BOOL_PROPERTY_ACCESSORS(sensitive)
	DEFINE_BOOL_PROPERTY_ACCESSORS(visible)

	// シグナル
	DEFINE_SIGNAL_HANDLER("map", map, CheckButton, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER("show", show, CheckButton, GtkWidget* self)
};

/**
 * @class RadioButton
 * @brief sg::RadioButtonを拡張し、機能を追加したクラス。
 */
class RadioButton : public sg::RadioButton {
public:
	using sg::RadioButton::RadioButton; // 基底クラスのコンストラクタを継承

	// メソッド
	DEFINE_CSS_CLASS_ACCESSORS()

	// プロパティ
	DEFINE_BOOL_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus)
	DEFINE_BOOL_PROPERTY_ACCESSORS(sensitive)
	DEFINE_BOOL_PROPERTY_ACCESSORS(visible)
	DEFINE_BOOL_PROPERTY_ACCESSORS(active)

	// シグナル
	DEFINE_SIGNAL_HANDLER("map", map, RadioButton, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER("show", show, RadioButton, GtkWidget* self)
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
	DEFINE_CSS_CLASS_ACCESSORS()

	// プロパティ
	DEFINE_BOOL_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus)
	DEFINE_BOOL_PROPERTY_ACCESSORS(sensitive)
	DEFINE_BOOL_PROPERTY_ACCESSORS(visible)
	DEFINE_BOOL_PROPERTY_ACCESSORS(editable)

	// シグナル
	DEFINE_SIGNAL_HANDLER_WITH_RETURN("button-press-event", button_press_event, gboolean, (GtkWidget* self, GdkEventButton* event, sgex::Entry* thiz), std::function<bool(GdkEventButton* event)>, event)
	DEFINE_SIGNAL_HANDLER_WITH_RETURN("button-release-event", button_release_event, gboolean, (GtkWidget* self, GdkEventButton* event, sgex::Entry* thiz), std::function<bool(GdkEventButton* event)>, event)
	DEFINE_SIGNAL_HANDLER_WITH_RETURN("focus-in-event", focus_in_event, gboolean, (GtkWidget* self, GdkEventFocus* event, sgex::Entry* thiz), std::function<bool(GdkEventFocus* event)>, event)
	DEFINE_SIGNAL_HANDLER_WITH_RETURN("focus-out-event", focus_out_event, gboolean, (GtkWidget* self, GdkEventFocus* event, sgex::Entry* thiz), std::function<bool(GdkEventFocus* event)>, event)
	DEFINE_SIGNAL_HANDLER("map", map, Entry, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER("show", show, Entry, GtkWidget* self)
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
	DEFINE_BOOL_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus)

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
	DEFINE_CSS_CLASS_ACCESSORS()

	// プロパティ
	DEFINE_BOOL_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus)

	// シグナル
	DEFINE_SIGNAL_HANDLER("map", map, Switch, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER("show", show, Switch, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER_WITH_RETURN("state-set", state_set, gboolean, (GtkSwitch* self, gboolean state, sgex::Switch* thiz), std::function<bool(bool)>, state)
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
	DEFINE_CSS_CLASS_ACCESSORS()

	// プロパティ
	DEFINE_BOOL_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus)
	DEFINE_BOOL_PROPERTY_ACCESSORS(sensitive)
	DEFINE_BOOL_PROPERTY_ACCESSORS(visible)
	DEFINE_PROPERTY_ACCESSORS(ComboBox, active, gint)
	DEFINE_PROPERTY_ACCESSORS(ComboBox, model, GtkTreeModel*)

	// シグナル
	DEFINE_SIGNAL_HANDLER("map", map, ComboBox, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER("show", show, ComboBox, GtkWidget* self)
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
	DEFINE_CSS_CLASS_ACCESSORS()

	// プロパティ
	DEFINE_BOOL_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus)
	DEFINE_BOOL_PROPERTY_ACCESSORS(visible)

	// シグナル
	DEFINE_SIGNAL_HANDLER("map", map, Label, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER("show", show, Label, GtkWidget* self)
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
	DEFINE_METHOD_WITH_ARGS(tree_model, get_iter_first, gboolean, (GtkTreeIter  *iter), iter)
	DEFINE_METHOD_WITH_ARGS(tree_model, get_path, GtkTreePath*, (GtkTreeIter  *iter), iter)
	DEFINE_METHOD_WITH_ARGS(tree_model, iter_previous, gboolean, (GtkTreeIter  *iter), iter)
	DEFINE_METHOD_WITH_ARGS(tree_model, iter_next, gboolean, (GtkTreeIter  *iter), iter)
	DEFINE_METHOD_WITH_ARGS(tree_model, iter_nth_child, gboolean, (GtkTreeIter  *iter, GtkTreeIter *parent, gint n), iter, parent, n)
	DEFINE_METHOD_WITH_ARGS(list_store, remove, gboolean, (GtkTreeIter *iter), iter)
	DEFINE_METHOD_WITH_ARGS(list_store, insert, void, (GtkTreeIter *iter, gint position), iter, position)
	DEFINE_METHOD_WITH_ARGS(list_store, append, void, (GtkTreeIter *iter), iter)

	/**
	 * @brief gtk_list_store_set の可変長テンプレートラッパー
	 * @details ラッパー内で args... の末尾に -1 を自動で付与するため、呼び出し側で末尾の -1 は不要
	 * @param[out] iter 更新する行のイテレータ
	 * @param[in] args GtkListStore に設定する列インデックスと値のペアの可変長引数
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
	explicit TreePath(GtkTreePath* path) : m_path(path) {}

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
	DEFINE_METHOD(tree_selection, unselect_all, void)

	// プロパティ
	DEFINE_PROPERTY_ACCESSORS(Widget, mode, GtkSelectionMode)

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
//	DEFINE_METHOD_WITH_ARGS(tree_view, set_model, void, (GtkTreeModel* model), model)
	DEFINE_METHOD_WITH_ARGS(tree_view, append_column, gint, (GtkTreeViewColumn* column), column)
	DEFINE_METHOD(tree_view, get_n_columns, guint)
	DEFINE_METHOD_WITH_ARGS(tree_view, get_column, GtkTreeViewColumn*, (gint n), n)
	DEFINE_METHOD(tree_view, get_grid_lines, GtkTreeViewGridLines)
	DEFINE_METHOD_WITH_ARGS(tree_view, set_grid_lines, void, (GtkTreeViewGridLines grid_lines), grid_lines)
	DEFINE_CSS_CLASS_ACCESSORS()

	// プロパティ
	DEFINE_BOOL_PROPERTY_ACCESSORS_WITH_NAME("can-focus", can_focus)
	DEFINE_BOOL_PROPERTY_ACCESSORS(sensitive)
	DEFINE_BOOL_PROPERTY_ACCESSORS(visible)
	DEFINE_PROPERTY_ACCESSORS(TreeView, model, GtkTreeModel*)

	// シグナル
	DEFINE_SIGNAL_HANDLER_WITH_RETURN("button-press-event", button_press_event, gboolean, (GtkWidget* self, GdkEventButton* event, sgex::TreeView* thiz), std::function<bool(GdkEventButton* event)>, event)
	DEFINE_SIGNAL_HANDLER_WITH_RETURN("button-release-event", button_release_event, gboolean, (GtkWidget* self, GdkEventButton* event, sgex::TreeView* thiz), std::function<bool(GdkEventButton* event)>, event)
	DEFINE_SIGNAL_HANDLER("map", map, TreeView, GtkWidget* self)
	DEFINE_SIGNAL_HANDLER("show", show, TreeView, GtkWidget* self)
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
	TreeViewColumn(GtkTreeViewColumn* column) = delete;
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

	// メソッド
	DEFINE_METHOD_WITH_ARGS(tree_view_column, pack_start, void, (GtkCellRenderer* cell, bool expand), cell, expand)
	DEFINE_METHOD_WITH_ARGS(tree_view_column, pack_end, void, (GtkCellRenderer* cell, bool expand), cell, expand)
	DEFINE_METHOD_WITH_ARGS(tree_view_column, add_attribute, void, (GtkCellRenderer* cell, const char* attribute, int column), cell, attribute, column)

	// プロパティ
	DEFINE_PROPERTY_ACCESSORS_WITH_NAME("fixed-width", fixed_width, gint)
	DEFINE_PROPERTY_ACCESSORS(Widget, title, const gchar*)
	DEFINE_PROPERTY_ACCESSORS(Widget, alignment, gfloat)

	// シグナル
};

/**
 * @class ListStoreModelBase
 * @brief GtkListStoreとstd::vector<_Tp>を同期させる、汎用的なデータモデルの基底クラス。
 * @details このクラスはTreeViewやComboBoxなどの特定のウィジェットには依存せず、
 * GtkListStoreそのものの生成と、C++側のデータモデル(std::vector)の基本的な
 * CRUD(Create, Read, Update, Delete)操作のみを提供します。
 * @tparam _Tp テーブルの1行分のデータを表現する構造体の型。
 */
template<typename _Tp>
class ListStoreModelBase {
public:
	// --- 型定義 ---
	using vector_type				= std::vector<_Tp>;	///< 内部データモデルとして使用する std::vector の型エイリアス
	using value_type				= typename vector_type::value_type;
	using size_type					= typename vector_type::size_type;
	using const_iterator			= typename vector_type::const_iterator;
	using const_reverse_iterator	= typename vector_type::const_reverse_iterator;

	using indexed_value_pair		= std::pair<size_type, value_type>;	///< インデックスと value_type のペア型エイリアス
	using setter_function_type		= std::function<void(sgex::ListStore*, GtkTreeIter*, const value_type&)>;	///< セッター関数の型エイリアス

protected:
	/// @brief (View) SimpleGtk でラップされた GtkListStore の所有権を持つポインタ。
	std::unique_ptr<sgex::ListStore> m_list_store_ptr;
	/// @brief (Model) C++側のデータ本体。アプリケーションロジックの「信頼できる唯一の情報源」。
	vector_type m_data_model;
	/// @brief value_type から GtkListStore の行データに変換するためのセッター関数。
	setter_function_type m_row_setter;
	/// @brief モデルがプログラムによって更新されている最中かを示すフラグ
	bool m_is_programmatically_changing;

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
	 * @param[out] iter 値を設定する行を指すGtkTreeIter。
	 * @param[in] value セットするデータを持つ _Tp。
	 */
	void set_row_to_store(GtkTreeIter* iter, const value_type& value) {
		if (m_row_setter) {
			m_row_setter(get_sg_store(), iter, value);
		}
	}

	/**
	 * @brief (内部ヘルパー) 現在の m_data_model の内容で GTK側のストアを完全に作り直します。
	 * @details clear() して append() するのと同等ですが、一括操作用です。
	 */
	void repopulate_store() {
		// GTK側のストアをクリア
		m_list_store_ptr->clear();

		GtkTreeIter iter;
		// データモデルの全要素をGTK側に追加
		for (const auto& value : m_data_model) {
			get_sg_store()->append(&iter);
			set_row_to_store(&iter, value);
		}
	}

private:
	// ---  内部ヘルパー ---
	/**
	 * @brief 末尾に単一の行をコピー追加します。
	 * @param[in] value 追加する行のデータ。
	 * @return 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
	 */
	void _append(const value_type& value) {
		m_data_model.push_back(value);
		GtkTreeIter iter;
		get_sg_store()->append(&iter);
		set_row_to_store(&iter, value);
	}

	/**
	 * @brief 末尾に単一の行をムーブ追加します。
	 * @param[in] value 追加する行のデータ（右辺値）。
	 * @return 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
	 */
	void _append(value_type&& value) {
		// vector にムーブ
		m_data_model.push_back(std::move(value));

		GtkTreeIter iter;
		get_sg_store()->append(&iter);

		// GTKへの設定には、vector内の（ムーブ済みの）実体を使用する
		set_row_to_store(&iter, m_data_model.back());
	}

	/**
	 * @brief 指定した位置に単一の行をコピー挿入します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * size()以上の値を指定した場合は、末尾に追加(append)されます。
	 * @param[in] position 0から始まる挿入位置のインデックス。
	 * @param[in] value 挿入する行のデータ。
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool _insert(size_type position, const value_type& value) {
		size_type original_position = position;
		bool position_was_clamped = false;
		if (position > size()) {
			position = size();
		}

		// 要求された位置と、実際の挿入位置が違うか判定
		if (position != original_position) {
			position_was_clamped = true;
		}

		m_data_model.insert(m_data_model.begin() + position, value);
		GtkTreeIter iter;
		get_sg_store()->insert(&iter, position);
		set_row_to_store(&iter, value);
		return !position_was_clamped;
	}

	/**
	 * @brief 指定した位置に単一の行をムーブ挿入します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * @param[in] position 0から始まる挿入位置のインデックス。
	 * @param[in] value 挿入する行のデータ（右辺値）。
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool _insert(size_type position, value_type&& value) {
		size_type original_position = position;
		bool position_was_clamped = false;
		if (position > size()) {
			position = size();
		}

		// 要求された位置と、実際の挿入位置が違うか判定
		if (position != original_position) {
			position_was_clamped = true;
		}

		// vector にムーブ挿入
		auto it = m_data_model.insert(m_data_model.begin() + position, std::move(value));

		GtkTreeIter iter;
		get_sg_store()->insert(&iter, position);

		// GTKへの設定には、vector内の（ムーブ済みの）実体を使用する
		set_row_to_store(&iter, *it);

		return !position_was_clamped;
	}

	/**
	 * @brief 指定した位置の行を削除します。
	 * @param[in] position 削除位置のインデックス。
	 */
	void _erase(size_type position) {
		m_data_model.erase(m_data_model.begin() + position);
		GtkTreeIter iter;
		if (get_sg_store()->iter_nth_child(&iter, nullptr, position)) {
			get_sg_store()->remove(&iter);
		}
	}

	/**
	 * @brief 指定した位置の行をコピー上書きします。
	 * @param[in] position 上書き位置のインデックス。
	 * @param[in] value 新しい行データ。
	 */
	void _update(size_type position, const value_type& value) {
		m_data_model[position] = value;
		GtkTreeIter iter;
		if (get_sg_store()->iter_nth_child(&iter, nullptr, position)) {
			set_row_to_store(&iter, value);
		}
	}

	/**
	 * @brief 指定した位置の行をコピー上書きします。
	 * @param[in] indexed_value 上書き位置のインデックスと新しい行データのペア。
	 */
	void _update(const indexed_value_pair& indexed_value) {
		_update(indexed_value.first, indexed_value.second);	// 別のオーバーロードへ処理を委譲
	}

	/**
	 * @brief 指定した位置の行をムーブ上書きします。
	 * @param[in] position 上書き位置のインデックス。
	 * @param[in] value 新しい行データ（右辺値）。
	 */
	void _update(size_type position, value_type&& value) {
		// vector にムーブ代入
		m_data_model[position] = std::move(value);

		GtkTreeIter iter;
		if (get_sg_store()->iter_nth_child(&iter, nullptr, position)) {
			// GTKへの設定には、vector内の（ムーブ済みの）実体を使用する
			set_row_to_store(&iter, m_data_model[position]);
		}
	}

	/**
	 * @brief 指定した位置の行をムーブ上書きします。
	 * @param[in] indexed_value 上書き位置のインデックスと新しい行データのペア（右辺値）。
	 */
	void _update(indexed_value_pair&& indexed_value) {
		_update(indexed_value.first, std::move(indexed_value.second));	// 別のオーバーロードへ処理を委譲
	}

public:
	/**
	 * @brief ListStoreModelBaseのコンストラクタ。
	 * @param[in] column_types GtkListStoreが持つ列のGTypeのリスト。
	 * @param[in] row_setter _Tp のデータをGtkListStoreの行に変換・設定する関数。
	 */
	ListStoreModelBase(
		const std::vector<GType>& column_types,
		setter_function_type row_setter
	) :
		m_row_setter(row_setter),
		m_is_programmatically_changing(false)
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

	// コピー・ムーブを完全に禁止（アドレスを固定するため）
	ListStoreModelBase(const ListStoreModelBase&) = delete;
	ListStoreModelBase& operator=(const ListStoreModelBase&) = delete;
	ListStoreModelBase(ListStoreModelBase&&) = delete;
	ListStoreModelBase& operator=(ListStoreModelBase&&) = delete;

	// --- イテレータ対応 (読み取り専用) ---

	const_iterator begin() const { return m_data_model.begin(); }
	const_iterator end() const { return m_data_model.end(); }
	const_reverse_iterator rbegin() const noexcept { return m_data_model.rbegin(); }
	const_reverse_iterator rend() const noexcept { return m_data_model.rend(); }
	const_iterator cbegin() const { return m_data_model.cbegin(); }
	const_iterator cend() const { return m_data_model.cend(); }
	const_reverse_iterator crbegin() const noexcept { return m_data_model.crbegin(); }
	const_reverse_iterator crend() const noexcept { return m_data_model.crend(); }

	// --- 基本的なデータ操作（単一行） ---

	/**
	 * @brief Gtk側（GtkListStore）の内容を強制的に更新します。
	 * @details 保持しているデータモデル(m_data_model)の値を使って、ビューを同期させます。
	 * @param[in] force_repopulate (オプション・デフォルト false)
	 * trueの場合、GTKストアをクリアして全行を作り直します（行の増減や順序変更も反映されます）。
	 * falseの場合、既存の行に対して値を再設定するだけです（高速ですが、行数の不整合は解消されません）。
	 */
	void refresh(bool force_repopulate = false) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		// trueなら全作り直し
		if (force_repopulate) {
			repopulate_store();
			return;
		}

		// falseなら既存の行の値更新のみ
		GtkTreeIter iter;

		// 最初の行を取得
		if (!get_sg_store()->get_iter_first(&iter)) {
			return; // データがない場合は何もしない
		}

		size_type index = 0;
		do {
			if (index >= m_data_model.size()) { break; }	// 安全策（念のため）

			// 現在の行のデータをセッターに渡して更新
			set_row_to_store(&iter, m_data_model[index]);

			index++;
		} while (get_sg_store()->iter_next(&iter));	// 次の行へ移動。次がなければループ終了
	}

	/// @brief C++モデルとGtkListStoreの両方から全ての行を削除します。
	void clear() {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_data_model.clear();
		m_list_store_ptr->clear();
	}

	/**
	 * @brief 内部データモデルの容量を事前に確保します。
	 * @param[in] new_cap 確保する新しい容量。
	 */
	void reserve(size_type new_cap) {
		m_data_model.reserve(new_cap);
	}

	/**
	 * @brief 末尾に単一の行をコピー追加します。
	 * @param[in] value 追加する行のデータ。
	 * @return 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
	 */
	bool append(const value_type& value) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		if (size() >= (size_type)((size_type)(std::numeric_limits<int>::max()))) {
			return false; // 失敗: 行数がintの最大値を超える
		}
		_append(value);
		return true; // 成功
	}

	/**
	 * @brief 末尾に単一の行をムーブ追加します。
	 * @param[in] value 追加する行のデータ（右辺値）。
	 * @return 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
	 */
	bool append(value_type&& value) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		if (size() >= (size_type)((size_type)(std::numeric_limits<int>::max()))) {
			return false; // 失敗: 行数がintの最大値を超える
		}
		_append(std::move(value));
		return true; // 成功
	}

	/**
	 * @brief 末尾に複数の行をまとめてコピー追加します。
	 * @param[in] values 追加する行データのvector。
	 * @return 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
	 */
	bool append(const vector_type& values) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		size_type appended_size = size() + values.size();
		if (appended_size > (size_type)(std::numeric_limits<int>::max())) {
			return false;	// 失敗: 行数がintの最大値を超える
		}

		// パフォーマンス向上のため、あらかじめメモリを確保
		reserve(appended_size);

		for (const auto& value : values) {
			_append(value);	// 内部ヘルパーをループで呼び出す
		}
		return true;	// 成功
	}

	/**
	 * @brief 末尾に複数の行をまとめて追加します（ムーブ版）。
	 * @details 引数のvector内の各要素をムーブして追加します。処理後、引数のvectorの中身は不定（空など）になります。
	 * @param[in] values 追加する行データのvector（右辺値）。
	 * @return 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
	 */
	bool append(vector_type&& values) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		size_type appended_size = size() + values.size();
		if (appended_size > (size_type)(std::numeric_limits<int>::max())) {
			return false;	// 失敗: 行数がintの最大値を超える
		}

		// パフォーマンス向上のため、あらかじめメモリを確保
		reserve(appended_size);

		for (auto& value : values) {
			// 単一行のムーブ版 append を呼び出す
			// (value自体は左辺値なので、std::moveで右辺値キャストして渡す)
			_append(std::move(value));
		}
		return true;	// 成功
	}

	/**
	 * @brief 指定した位置に単一の行をコピー挿入します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * size()以上の値を指定した場合は、末尾に追加(append)されます。
	 * @param[in] position 0から始まる挿入位置のインデックス。
	 * @param[in] value 挿入する行のデータ。
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool insert(size_type position, const value_type& value) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		if (size() >= (size_type)(std::numeric_limits<int>::max())) {
			return false; // 失敗: 行数がintの最大値を超える
		}
		return _insert(position, value);
	}

	/**
	 * @brief 指定した位置に単一の行をムーブ挿入します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * @param[in] position 0から始まる挿入位置のインデックス。
	 * @param[in] value 挿入する行のデータ（右辺値）。
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool insert(size_type position, value_type&& value) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		if (size() >= (size_type)(std::numeric_limits<int>::max())) {
			return false; // 失敗: 行数がintの最大値を超える
		}
		return _insert(position, std::move(value));
	}

	/**
	 * @brief 指定した位置に複数の行をまとめてコピー挿入します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * size()以上の値を指定した場合は、末尾に追加(append)されます。
	 * @param[in] position 0から始まる挿入位置のインデックス。
	 * @param[in] values 挿入する行データのvector。
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool insert(size_type position, const vector_type& values) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		size_type inserted_size = size() + values.size();
		if (inserted_size > (size_type)(std::numeric_limits<int>::max())) {
			return false; // 失敗: 行数がintの最大値を超える
		}
		// 開始位置が有効範囲内([0, size()])にあるかチェック
		bool position_was_valid = (position <= size());
		size_type current_pos = position;

		// パフォーマンス向上のため、あらかじめメモリを確保
		reserve(inserted_size);

		for (const auto& value : values) {
			// 位置補正(Clamp)は単一行版がやってくれるので、ここではインクリメントするだけで良い
			_insert(current_pos++, value);
		}
		return position_was_valid;
	}

	/**
	 * @brief 指定した位置に複数の行をまとめてムーブ挿入します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * size()以上の値を指定した場合は、末尾に追加(append)されます。
	 * @param[in] position 0から始まる挿入位置のインデックス。
	 * @param[in] values 挿入する行データのvector（右辺値）。
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool insert(size_type position, vector_type&& values) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		size_type inserted_size = size() + values.size();
		if (inserted_size > (size_type)(std::numeric_limits<int>::max())) {
			return false; // 失敗
		}

		// 開始位置が有効範囲内かチェック
		bool position_was_valid = (position <= size());
		size_type current_pos = position;

		// パフォーマンス向上のためメモリ確保
		reserve(inserted_size);

		for (auto& value : values) {
			// 単一行のムーブ版 insert を呼び出す
			// 位置補正(Clamp)は単一行版がやってくれるので、ここではインクリメントするだけで良い
			_insert(current_pos++, std::move(value));
		}
		return position_was_valid;
	}

	/**
	 * @brief 指定した位置の行を削除します。
	 * @param[in] position 削除位置のインデックス。
	 * @return 削除に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool erase(size_type position) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		if (position >= m_data_model.size()) {
			return false; // 失敗: インデックスが範囲外
		}
		_erase(position);
		return true; // 成功
	}

	/**
	 * @brief 指定した複数の位置の行をまとめて削除します。
	 * @param[in] positions 削除する行のインデックスのvector。
	 * @return 指定されたインデックスが全て有効な場合のみtrue。一つでも無効なら処理せずfalse。
	 */
	bool erase(std::vector<size_type> positions) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		// ★ 先に全てのインデックスが有効かチェック
		for (size_type pos : positions) {
			if (pos >= size()) {
				return false; // 一つでも無効なインデックスがあれば失敗
			}
		}

		std::sort(positions.rbegin(), positions.rend());
		for (size_type pos : positions) {
			_erase(pos);
		}
		return true; // 全て成功
	}

	/**
	 * @brief 指定した位置の行をコピー上書きします。
	 * @param[in] position 上書き位置のインデックス。
	 * @param[in] value 新しい行データ。
	 * @return 更新に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool update(size_type position, const value_type& value) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		if (position >= size()) {
			return false; // 失敗: インデックスが範囲外
		}
		_update(position, value);
		return true; // 成功
	}

	/**
	 * @brief 指定した位置の行をコピー上書きします。
	 * @param[in] indexed_value 上書き位置のインデックスと新しい行データのペア。
	 * @return 更新に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool update(const indexed_value_pair& indexed_value) {
		return update(indexed_value.first, indexed_value.second);	// 別のオーバーロードへ処理を委譲
	}

	/**
	 * @brief 指定した位置の行をムーブ上書きします。
	 * @param[in] position 上書き位置のインデックス。
	 * @param[in] value 新しい行データ（右辺値）。
	 * @return 更新に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool update(size_type position, value_type&& value) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		if (position >= size()) {
			return false; // 失敗: インデックスが範囲外
		}
		_update(position, std::move(value));
		return true; // 成功
	}

	/**
	 * @brief 指定した位置の行をムーブ上書きします。
	 * @param[in] indexed_value 上書き位置のインデックスと新しい行データのペア（右辺値）。
	 * @return 更新に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool update(indexed_value_pair&& indexed_value) {
		return update(indexed_value.first, std::move(indexed_value.second));	// 別のオーバーロードへ処理を委譲
	}

	/**
	 * @brief 指定した複数の位置の行をまとめてコピー上書きします。
	 * @param[in] updates 上書きする位置のインデックスと、新しい行データのmap。
	 * @return 指定されたインデックスが全て有効な場合のみtrue。一つでも無効なら処理せずfalse。
	 */
	bool update(const std::map<size_type, value_type>& updates) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		// ★ 先に全てのインデックスが有効かチェック
		for (const auto& [pos, value] : updates) {
			if (pos >= size()) {
				return false; // 一つでも無効なインデックスがあれば失敗
			}
		}

		for (const auto& pair : updates) {
			_update(pair);
		}
		return true; // 全て成功
	}

	/**
	 * @brief 指定した複数の位置の行をまとめてムーブ上書きします。
	 * @param[in] updates 上書きする位置のインデックスと、新しい行データのmap（右辺値）。
	 * @return 全て成功ならtrue。
	 */
	bool update(std::map<size_type, value_type>&& updates) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		// インデックスチェック
		for (const auto& [pos, value] : updates) {
			if (pos >= size()) { return false; }
		}

		for (auto& pair : updates) {
			// 単一行のムーブ版 update を呼び出す
			_update(std::move(pair));
		}
		return true;
	}

	/**
	 * @brief データモデル全体を新しいデータセットで置き換えます（コピー版）。
	 * @details 既存のデータは破棄され、新しいデータでビューが再構築されます。
	 * @param[in] new_data 新しいデータセット。
	 */
	void set_model_data(const vector_type& new_data) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_data_model = new_data; // コピー
		repopulate_store();
	}

	/**
	 * @brief データモデル全体を新しいデータセットで置き換えます（ムーブ版）。
	 * @details 既存のデータは破棄され、新しいデータでビューが再構築されます。
	 * @param[in] new_data 新しいデータセット（ムーブ元）。
	 */
	void set_model_data(vector_type&& new_data) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_data_model = std::move(new_data); // ムーブ
		repopulate_store();
	}

	/**
	 * @brief データモデルに新しいデータセットをコピーし、古いデータを返します。
	 * @details Undo機能の実装や、一時的なデータ切り替えに便利です。
	 * @param[in] new_data 新しいデータセット。
	 * @return 置き換えられる前の古いデータセット。
	 */
	vector_type replace_model_data(const vector_type& new_data) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		vector_type old_data = std::move(m_data_model); // 現在のデータを退避
		m_data_model = new_data; // 新しいデータをコピー

		repopulate_store();
		return old_data;
	}

	/**
	 * @brief データモデルを新しいデータセットに置き換え、古いデータを返します（ムーブ版）。
	 * @param[in] new_data 新しいデータセット（ムーブ元）。
	 * @return 置き換えられる前の古いデータセット。
	 */
	vector_type replace_model_data(vector_type&& new_data) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		vector_type old_data = std::move(m_data_model); // 現在のデータを退避
		m_data_model = std::move(new_data); // 新しいデータをムーブ

		repopulate_store();
		return old_data;
	}

	/**
	 * @brief データモデルを直接編集するための関数オブジェクトを受け付けます。
	 * @details
	 * ソートや一括フィルタリングなど、vector全体に対する操作を行う場合に使用します。
	 * 操作完了後、ビューは自動的に再構築されます。
	 * @param[in] modifier データモデルへの参照を受け取り、編集を行う関数オブジェクト。
	 *
	 * ```cpp
	 * // 使用例: IDでソートする
	 * model.modify_model_data([](std::vector<_Tp>& values) {
	 * 	std::sort(values.begin(), values.end(), [](const auto& a, const auto& b) {
	 * 		return a.id < b.id;
	 * 	});
	 * });
	 * ```
	 */
	void modify_model_data(std::function<void(vector_type&)> modifier) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		// ユーザー関数にデータを渡して直接編集させる
		modifier(m_data_model);

		// 編集結果に合わせてビューを再構築
		repopulate_store();
	}

	/**
	 * @brief 指定した位置の行データを取得します。
	 * @details std::vector::at と同様、範囲外アクセスの場合は例外が投げられます。
	 * @param[in] position 0から始まる取得位置のインデックス。
	 * @return 行データへのconst参照。
	 */
	const value_type& at(size_type position) const {
		return m_data_model.at(position);
	}

	/**
	 * @brief 指定した位置の行データを取得します。（旧・互換版）
	 * @deprecated Use 'at' instead.
	 */
	[[deprecated("Use 'at' instead.")]]
	const value_type& get_item(size_type position) const {
		return at(position);
	}

	/**
	 * @brief C++モデル内の全ての行データを取得します。
	 * @return 全ての行データを含むvectorへのconst参照。
	 */
	const vector_type& get_data_model() const {
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
	 * @brief C++モデルが空かどうかを返します。
	 * @return 空であればtrue、そうでなければfalse。
	 */
	bool empty() const {
		return m_data_model.empty();
	}

	/**
	 * @brief 条件に一致する全ての行のインデックスを検索します。
	 * @param[in] predicate 検索条件となる述語関数。
	 * @return 条件に一致した行のインデックスのリスト。
	 *
	 * ```cpp
	 * // 例: ageフィールドが30以上の行のインデックスを取得
	 * auto indices = find_indices([](const _Tp& value) {
	 * 	return value.age >= 30;
	 * });
	 * ```
	 */
	std::vector<size_type> find_indices(std::function<bool(const value_type&)> predicate) const {
		std::vector<size_type> found_indices;
		for (size_type i = 0; i < m_data_model.size(); i++) {
			if (predicate(m_data_model[i])) {
				found_indices.push_back(i);
			}
		}
		return found_indices;
	}

	/**
	 * @brief 条件に一致する全ての行データを検索します。
	 * @param[in] predicate 検索条件となる述語関数。
	 * @return 条件に一致した行データのリスト。
	 *
	 * ```cpp
	 * // 例: ageフィールドが30以上の行のインデックスを取得
	 * auto values = find_values([](const _Tp& value) {
	 * 	return value.age >= 30;
	 * });
	 * ```
	 */
	vector_type find_values(std::function<bool(const value_type&)> predicate) const {
		vector_type found_values;
		for (const auto& value : m_data_model) {
			if (predicate(value)) {
				found_values.push_back(value);
			}
		}
		return found_values;
	}

	/**
	 * @brief 条件に一致する全ての行データを検索します。（旧・互換版）
	 * @deprecated Use 'find_values' instead.
	 */
	[[deprecated("Use 'find_values' instead.")]]
	vector_type find_items(std::function<bool(const value_type&)> predicate) const {
		return find_values(predicate);
	}

	/**
	 * @brief 条件に一致する全ての行のインデックスと行データのペアを検索します。
	 * @param[in] predicate 検索条件となる述語関数。
	 * @return 条件に一致した行のインデックスと行データのペアのリスト。
	 *
	 * ```cpp
	 * // 例: ageフィールドが30以上の行のインデックスを取得
	 * std::vector<std::pair<size_t, _Tp>> pairs = find_indexed_value_pairs([](const _Tp& value) {
	 * 	return value.age >= 30;
	 * });
	 * ```
	 */
	std::vector<indexed_value_pair> find_indexed_value_pairs(std::function<bool(const value_type&)> predicate) const {
		std::vector<indexed_value_pair> found_pairs;
		for (size_type i = 0; i < m_data_model.size(); i++) {
			if (predicate(m_data_model[i])) {
				found_pairs.emplace_back(i, m_data_model[i]);
			}
		}
		return found_pairs;
	}

	/**
	 * @brief 条件に一致する全ての行のインデックスと行データのペアを検索します。（旧・互換版）
	 * @deprecated Use 'find_indexed_value_pairs' instead.
	 */
	[[deprecated("Use 'find_indexed_value_pairs' instead.")]]
	std::vector<indexed_value_pair> find_indices_and_items(std::function<bool(const value_type&)> predicate) const {
		return find_indexed_value_pairs(predicate);
	}

	/**
	 * @brief 条件に一致する最初の行のインデックスを検索します。
	 * @param[in] predicate 検索条件となる述語関数。
	 * @return 条件に一致した最初の行のインデックス。
	 *
	 * ```cpp
	 * // 例: ageフィールドが30以上の行のインデックスを取得
	 * auto index_opt = find_first_index([](const _Tp& value) {
	 * 	return value.age >= 30;
	 * });
	 * ```
	 */
	std::optional<size_type> find_first_index(std::function<bool(const value_type&)> predicate) const {
		for (size_type i = 0; i < m_data_model.size(); i++) {
			if (predicate(m_data_model[i])) {
				return i;
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief 条件に一致する最初の行データを検索します。
	 * @param[in] predicate 検索条件となる述語関数。
	 * @return 条件に一致した最初の行データ。
	 *
	 * ```cpp
	 * // 例: ageフィールドが30以上の行のインデックスを取得
	 * auto value_opt = find_first_value([](const _Tp& value) {
	 * 	return value.age >= 30;
	 * });
	 * ```
	 */
	std::optional<value_type> find_first_value(std::function<bool(const value_type&)> predicate) const {
		for (const auto& value : m_data_model) {
			if (predicate(value)) {
				return value;
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief 条件に一致する最初の行データを検索します。（旧・互換版）
	 * @deprecated Use 'find_first_value' instead.
	 */
	[[deprecated("Use 'find_first_value' instead.")]]
	std::optional<value_type> find_first_item(std::function<bool(const value_type&)> predicate) const {
		return find_first_value(predicate);
	}

	/**
	 * @brief 条件に一致する最初の行のインデックスと行データのペアを検索します。
	 * @param[in] predicate 検索条件となる述語関数。
	 * @return 条件に一致した最初の行のインデックスと行データのペア。
	 *
	 * ```cpp
	 * // 例: ageフィールドが30以上の行のインデックスを取得
	 * auto indexed_value_opt = find_first_indexed_value([](const _Tp& value) {
	 * 	return value.age >= 30;
	 * });
	 * ```
	 */
	std::optional<indexed_value_pair> find_first_indexed_value(std::function<bool(const value_type&)> predicate) const {
		for (size_type i = 0; i < m_data_model.size(); i++) {
			if (predicate(m_data_model[i])) {
				return std::make_pair(i, m_data_model[i]);
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief 条件に一致する最初の行のインデックスと行データのペアを検索します。（旧・互換版）
	 * @deprecated Use 'find_first_indexed_value' instead.
	 */
	[[deprecated("Use 'find_first_indexed_value' instead.")]]
	std::optional<indexed_value_pair> find_first_index_and_item(std::function<bool(const value_type&)> predicate) const {
		return find_first_indexed_value(predicate);
	}
};

/**
 * @class ComboBoxListStoreModel
 * @brief ListStoreModelBaseを継承し、GtkComboBoxに特化した機能を追加した派生クラス。
 * @details セルレンダラーの自動設定や、選択状態の管理機能を提供します。
 * @tparam _Tp テーブルの1行分のデータを表現する構造体の型。
 */
template<typename _Tp>
class ComboBoxListStoreModel : public ListStoreModelBase<_Tp> {
public:
	// --- 型定義 ---
	using base_type					= ListStoreModelBase<_Tp>;	///< 基底クラスの型エイリアス
	using vector_type				= typename base_type::vector_type;
	using value_type				= typename base_type::value_type;
	using size_type					= typename base_type::size_type;
	using const_iterator			= typename base_type::const_iterator;
	using const_reverse_iterator	= typename base_type::const_reverse_iterator;
	using indexed_value_pair		= typename base_type::indexed_value_pair;
	using setter_function_type		= typename base_type::setter_function_type;

private:
	/// @brief このモデルが関連付けられている ComboBox ウィジェット。
	sgex::ComboBox& m_combo_box;
	/// @brief 外部に通知するコールバック (選択されたインデックスを渡す)
	std::function<void(std::optional<size_type>)> m_user_activation_callback = nullptr;

	/**
	 * @brief (内部ハンドラ) ComboBox の changed シグナルに接続
	 */
	void internal_on_changed() {
		// プログラムによる変更中は無視
		if (this->m_is_programmatically_changing) {
			return;
		}

		// コールバックが登録されていれば呼び出す
		if (m_user_activation_callback) {
			m_user_activation_callback(get_active_index());
		}
	}

public:
	/**
	 * @brief ComboBoxListStoreModelのコンストラクタ。
	 * @param[in, out] combo_box 管理対象のComboBoxウィジェット。
	 * @param[in] column_types GtkListStoreが持つ列のGTypeのリスト。
	 * @param[in] row_setter _Tp のデータをGtkListStoreの行に変換・設定する関数。
	 */
	ComboBoxListStoreModel(
		sgex::ComboBox& combo_box,
		const std::vector<GType>& column_types,
		setter_function_type row_setter
	) :
		base_type(column_types, row_setter), // 基底クラス初期化
		m_combo_box(combo_box)
	{
		// ComboBoxにモデルをセット
		m_combo_box.set_model(this->get_gtk_model());

		// 内部シグナルハンドラを接続
		m_combo_box.on_changed([this]() { this->internal_on_changed(); });
	}

	/**
	 * @brief ComboBoxのフォーカス可否を設定します。
	 * @param[in] can_focus フォーカス可能にする場合はtrue、不可能にする場合はfalse。
	 */
	void set_can_focus(bool can_focus) {
		m_combo_box.set_can_focus(can_focus);
	}

	/**
	 * @brief ComboBoxのフォーカス可否を取得します。
	 * @return フォーカス可能であればtrue、不可能であればfalse。
	 */
	bool get_can_focus() {
		return m_combo_box.get_can_focus();
	}

	/**
	 * @brief ComboBoxの感応可否を設定します。
	 * @param[in] sensitive 感応可能にする場合はtrue、無効にする場合はfalse。
	 */
	void set_sensitive(bool sensitive) {
		m_combo_box.set_sensitive(sensitive);
	}

	/**
	 * @brief ComboBoxの感応可否を取得します。
	 * @return 感応可能であればtrue、無効であればfalse。
	 */
	bool get_sensitive() {
		return m_combo_box.get_sensitive();
	}

	/**
	 * @brief ComboBoxの表示・非表示を設定します。
	 * @param[in] visible 表示する場合はtrue、非表示にする場合はfalse。
	 */
	void set_visible(bool visible) {
		m_combo_box.set_visible(visible);
	}

	/**
	 * @brief ComboBoxの表示・非表示を取得します。
	 * @return 表示されていればtrue、非表示であればfalse。
	 */
	bool get_visible() {
		return m_combo_box.get_visible();
	}

	/**
	 * @brief ユーザーが選択を変更したときに呼び出されるコールバックを登録します。
	 * @param[in] callback 選択された行のインデックスを受け取る関数オブジェクト。
	 * 未選択の場合は std::nullopt が渡されます。
	 */
	void on_changed_by_user(std::function<void(std::optional<size_type>)> callback) {
		m_user_activation_callback = callback;
	}

	/**
	 * @brief 現在選択されている行のインデックスを取得します。
	 * @return 選択されている行のインデックス。未選択なら std::nullopt。
	 */
	std::optional<size_type> get_active_index() {
		int active_index = m_combo_box.get_active();
		if (active_index < 0) {
			return std::nullopt;
		}
		return static_cast<size_type>(active_index);
	}

	/**
	 * @brief 現在選択されている行のデータを取得します。
	 * @return 選択されている行のデータ。未選択なら std::nullopt。
	 */
	std::optional<value_type> get_active_value() {
		auto index_opt = get_active_index();
		if (index_opt.has_value()) {
			return this->at(index_opt.value());
		}
		return std::nullopt;
	}

	/**
	 * @brief 現在選択されている行のデータを取得します。（旧・互換版）
	 * @deprecated Use 'get_active_value' instead.
	 */
	[[deprecated("Use 'get_active_value' instead.")]]
	std::optional<value_type> get_active_item() {
		return get_active_value();
	}

	/**
	 * @brief 現在選択されている行のインデックスと行データのペアを取得します。
	 * @return 選択されている行のインデックスとデータのペア。未選択なら std::nullopt。
	 */
	std::optional<indexed_value_pair> get_active_indexed_value() {
		auto index_opt = get_active_index();
		if (index_opt.has_value()) {
			return std::make_pair(index_opt.value(), this->at(index_opt.value()));
		}
		return std::nullopt;
	}

	/**
	 * @brief 現在選択されている行のインデックスと行データのペアを取得します。（旧・互換版）
	 * @deprecated Use 'get_active_indexed_value' instead.
	 */
	[[deprecated("Use 'get_active_indexed_value' instead.")]]
	std::optional<indexed_value_pair> get_active_index_and_item() {
		return get_active_indexed_value();
	}

	/**
	 * @brief 指定したインデックスを選択状態にします。
	 * @details プログラムによる変更フラグを立てるため、on_changed_by_user は発火しません。
	 * @param[in] index 選択するインデックス。
	 */
	void set_active_index(size_type index) {
		if (index >= this->size()) return;

		utils::scoped_value_guard guard(this->m_is_programmatically_changing, true);
		m_combo_box.set_active(static_cast<int>(index));
	}

	/**
	 * @brief 選択を解除します（未選択状態にする）。
	 */
	void clear_active() {
		utils::scoped_value_guard guard(this->m_is_programmatically_changing, true);
		m_combo_box.set_active(-1);
	}
};

/**
 * @class ListStoreViewInterface
 * @brief テンプレートに依存しない、ListStore系モデルの共通ビュー操作インターフェース。
 * @details テンプレート引数なしでポインタを保持し、表示設定やタイトル変更などを統一的に行うために使用します。
 */
class ListStoreViewInterface {
public:
	virtual ~ListStoreViewInterface() = default;

	// --- カラム・タイトル操作 ---

	/**
	 * @brief 指定したカラムのタイトルを設定します。
	 * @param[in] n カラムのインデックス。
	 * @param[in] title 新しいタイトル文字列。
	 * @return 設定に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	virtual bool set_title(gint n, const gchar* title) = 0;

	/**
	 * @brief 全カラムのタイトルを一括でコピー設定します。
	 * @param[in] titles 各カラムの新しいタイトル文字列のリスト。
	 * @return 設定に成功すればtrue、タイトル数がカラム数と一致しない場合はfalse。
	 */
	virtual bool set_titles(const std::vector<std::string>& titles) = 0;

	/**
	 * @brief 全カラムのタイトルを一括でムーブ設定します。
	 * @param[in] titles 各カラムの新しいタイトル文字列のリスト。
	 * @return 設定に成功すればtrue、タイトル数がカラム数と一致しない場合はfalse。
	 */
	virtual bool set_titles(std::vector<std::string>&& titles) = 0;

	/**
	 * @brief 指定したカラムのタイトルを取得します。
	 * @param[in] n カラムのインデックス。
	 * @return タイトル文字列。インデックスが無効な場合はstd::nullopt。
	 */
	virtual std::optional<std::string> get_title(gint n) = 0;	// 実装の制約により非const

	/**
	 * @brief 全カラムのタイトルを取得します。
	 * @return 各カラムのタイトル文字列のリスト。
	 */
	virtual std::vector<std::string> get_titles() = 0;	// 実装の制約により非const

	/**
	 * @brief カラム数を取得します。
	 * @return カラム数。
	 */
	virtual guint get_n_columns() = 0;	// 実装の制約により非const

	// --- グリッド線 ---

	/**
	 * @brief グリッド線の表示設定を取得します。
	 * @return 現在のグリッド線の表示設定。
	 */
	virtual GtkTreeViewGridLines get_grid_lines() = 0;	// 実装の制約により非const

	/**
	 * @brief グリッド線の表示設定を変更します。
	 * @param[in] grid_lines 新しいグリッド線の表示設定。
	 */
	virtual void set_grid_lines(GtkTreeViewGridLines grid_lines) = 0;

	// --- ウィジェット状態 ---

	/**
	 * @brief ウィジェットがフォーカス可能かどうかを設定します。
	 * @param[in] can_focus フォーカス可能にする場合はtrue、不可能にする場合はfalse。
	 */
	virtual void set_can_focus(bool can_focus) = 0;

	/**
	 * @brief ウィジェットがフォーカス可能かどうかを取得します。
	 * @return フォーカス可能であればtrue、不可能であればfalse。
	 */
	virtual bool get_can_focus() = 0;

	/**
	 * @brief ウィジェットの感度(有効/無効)を設定します。
	 * @param[in] sensitive 有効にする場合はtrue、無効にする場合はfalse。
	 */
	virtual void set_sensitive(bool sensitive) = 0;

	/**
	 * @brief ウィジェットの感度(有効/無効)を取得します。
	 * @return 有効であればtrue、無効であればfalse。
	 */
	virtual bool get_sensitive() = 0;

	/**
	 * @brief ウィジェットの表示/非表示を設定します。
	 * @param[in] visible 表示する場合はtrue、非表示にする場合はfalse。
	 */
	virtual void set_visible(bool visible) = 0;

	/**
	 * @brief ウィジェットの表示/非表示を取得します。
	 * @return 表示されていればtrue、非表示であればfalse。
	 */
	virtual bool get_visible() = 0;

	// --- 選択操作 ---

	/**
	 * @brief 選択モードを設定します。
	 * @param[in] type 新しい選択モード。
	 */
	virtual void set_selection_mode(GtkSelectionMode type) = 0;

	/**
	 * @brief 現在の選択モードを取得します。
	 * @return 現在の選択モード。
	 */
	virtual GtkSelectionMode get_selection_mode() = 0;

	/**
	 * @brief 全ての選択を解除します。
	 */
	virtual void unselect_all() = 0;
};

/**
 * @class TreeViewListStoreModel
 * @brief ListStoreModelBaseを継承し、GtkTreeViewに特化した機能を追加した派生クラス。
 * @details 選択行の取得や、パフォーマンスを考慮した複数行の一括更新機能などを提供します。
 * @tparam _Tp テーブルの1行分のデータを表現する構造体の型。
 */
template<typename _Tp>
class TreeViewListStoreModel : public ListStoreModelBase<_Tp>, public ListStoreViewInterface {
public:
	// --- 型定義 ---
	using base_type					= ListStoreModelBase<_Tp>;	///< 基底クラスの型エイリアス
	using vector_type				= typename base_type::vector_type;
	using value_type				= typename base_type::value_type;
	using size_type					= typename base_type::size_type;
	using const_iterator			= typename base_type::const_iterator;
	using const_reverse_iterator	= typename base_type::const_reverse_iterator;
	using indexed_value_pair		= typename base_type::indexed_value_pair;
	using setter_function_type		= typename base_type::setter_function_type;

private:
	/// @brief このモデルが関連付けられている TreeView ウィジェット。
	sgex::TreeView& m_tree_view;
	/// @brief このモデルが関連付けられている TreeView の Selection。
	sgex::TreeSelection m_tree_selection;
	/// @brief 連動させる外側の Widget へのポインタ
	sg::Widget* m_enclosing_widget = nullptr;
	/// @brief ユーザーがウィジェットを操作したことを示すフラグ
	bool m_user_initiated_selection = false;
	/// @brief 外部に通知するコールバック (ローカルインデックスを渡す)
	std::function<void(std::optional<size_type>)> m_user_selection_callback = nullptr;

	/**
	 * @brief (内部ハンドラ) TreeView の button-press-event に接続
	 * @param[in] event GdkEventButton イベントデータ
	 * @return イベントが処理された場合は true、伝播させる場合は false
	 */
	bool internal_on_button_press(const GdkEventButton* event) {
		if (event->button == GDK_BUTTON_PRIMARY) {
			m_user_initiated_selection = true;
		}
		return false; // イベントを伝播させ、GTKの選択処理を続行させる
	}

	/**
	 * @brief (内部ハンドラ) TreeSelection の changed に接続
	 */
	void internal_on_selection_changed() {
		// モデル更新中、またはユーザー起因でない場合は無視
		if (this->m_is_programmatically_changing || !m_user_initiated_selection) {
			return;
		}

		// 処理が開始したのでフラグをリセット
		m_user_initiated_selection = false;

		// コールバックが登録されていれば呼び出す
		if (m_user_selection_callback) {
			// 現在選択されている行のインデックスを渡す
			m_user_selection_callback(get_selected_index());
		}
	}

public:
	/**
	 * @brief TreeViewListStoreModelのコンストラクタ。
	 * @param[in, out] tree_view 管理対象のTreeViewウィジェット。
	 * @param[in] column_types GtkListStoreが持つ列のGTypeのリスト。
	 * @param[in] row_setter _Tp のデータをGtkListStoreの行に変換・設定する関数。
	 */
	TreeViewListStoreModel(
		sgex::TreeView& tree_view,
		const std::vector<GType>& column_types,
		setter_function_type row_setter
	) :
		base_type(column_types, row_setter), // 基底クラスのコンストラクタを呼び出し
		m_tree_view(tree_view),
		m_tree_selection(tree_view.get_selection()),
		m_user_initiated_selection(false)
	{
		// TreeViewにモデルをセット
		m_tree_view.set_model(this->get_gtk_model());

		// 内部シグナルハンドラを接続
		m_tree_view.on_button_press_event([this](GdkEventButton* event){ return this->internal_on_button_press(event); });
		m_tree_selection.on_changed([this](){ this->internal_on_selection_changed(); });
	}

	/**
	 * @brief TreeViewを囲む外枠のBoxなどを登録し、表示状態を連動させます。
	 * @param[in] widget 連動させるウィジェット（Box, Frame, Windowなど何でも可）。
	 */
	void set_enclosing_widget(sg::Widget* widget) {
		m_enclosing_widget = widget;

		// 登録時に現在のTreeViewの状態と同期させる
		if (m_enclosing_widget) {
			// sg::Widget* から GtkWidget* への変換は SimpleGtk が自動で行ってくれる
			bool visible = m_tree_view.get_visible();
			gtk_widget_set_visible(*m_enclosing_widget, visible);
		}
	}

	// --- ListStoreViewInterface の実装 (override) ---

	/**
	 * @brief 指定したインデックスのカラムのタイトルを設定します。
	 * @param[in] n カラムのインデックス (0から始まる)。
	 * @param[in] title 設定するタイトル文字列。
	 * @return 成功すればtrue、指定されたインデックスのカラムが存在しない場合はfalse。
	 */
	bool set_title(gint n, const gchar* title) override {
		GtkTreeViewColumn* column = m_tree_view.get_column(n);
		if (!column) {
			return false; // 指定されたインデックスのカラムが存在しない
		}
		gtk_tree_view_column_set_title(column, title);
		return true;
	}

	/**
	 * @brief 複数のカラムのタイトルを一括で設定します。
	 * @param[in] titles 各カラムのタイトル文字列を格納したvector。
	 * インデックス0が最初のカラム、1が2番目のカラム、という順序になります。
	 * @return 成功すればtrue、指定されたインデックスのカラムが存在しない場合はfalse。
	 */
	bool set_titles(const std::vector<std::string>& titles) override {
		if (titles.size() != m_tree_view.get_n_columns()) {
			return false;	// カラム数とタイトル数が一致しない
		}
		for (size_t i = 0; i < titles.size(); i++) {
			set_title(static_cast<gint>(i), titles[i].c_str());
		}
		return true;
	}

	/**
	 * @brief 複数のカラムのタイトルを一括で設定します。
	 * @details ムーブする必要がないため、内部でコピー版のset_titlesを呼び出します。
	 * @param[in] titles 各カラムのタイトル文字列を格納したvector。
	 * インデックス0が最初のカラム、1が2番目のカラム、という順序になります。
	 * @return 成功すればtrue、指定されたインデックスのカラムが存在しない場合はfalse。
	 */
	bool set_titles(std::vector<std::string>&& titles) override {
		return set_titles(titles);
	}

	/**
	 * @brief 指定したインデックスのカラムのタイトルを取得します。
	 * @param[in] n カラムのインデックス (0から始まる)。
	 * @return カラムのタイトル文字列。指定されたインデックスのカラムが存在しない場合は空のoptional。
	 */
	std::optional<std::string> get_title(gint n) override {
		GtkTreeViewColumn* column = m_tree_view.get_column(n);
		if (!column) {
			return std::nullopt; // 指定されたインデックスのカラムが存在しない
		}
		return std::string(gtk_tree_view_column_get_title(column));
	}

	std::vector<std::string> get_titles() override {
		std::vector<std::string> titles;
		gint n_columns = 0;
		while (true) {
			GtkTreeViewColumn* column = m_tree_view.get_column(n_columns);
			if (!column) {
				break; // これ以上カラムが存在しない
			}
			titles.push_back(std::string(gtk_tree_view_column_get_title(column)));
			n_columns++;
		}
		return titles;
	}

	/**
	 * @brief 表の列数を取得します。
	 * @return 列数。
	 */
	guint get_n_columns() override {
		return m_tree_view.get_n_columns();
	}

	/**
	 * @brief TreeViewのグリッド線の表示状態を取得します。
	 * @return グリッド線の表示状態。
	 */
	GtkTreeViewGridLines get_grid_lines() override {
		return m_tree_view.get_grid_lines();
	}

	/**
	 * @brief TreeViewのグリッド線の表示状態を設定します。
	 * @param[in] grid_lines グリッド線の表示状態。
	 */
	void set_grid_lines(GtkTreeViewGridLines grid_lines) override {
		m_tree_view.set_grid_lines(grid_lines);
	}

	/**
	 * @brief TreeViewのフォーカス可否を設定します。
	 * @param[in] can_focus フォーカス可能にする場合はtrue、不可能にする場合はfalse。
	 */
	void set_can_focus(bool can_focus) override {
		m_tree_view.set_can_focus(can_focus);
	}

	/**
	 * @brief TreeViewのフォーカス可否を取得します。
	 * @return フォーカス可能であればtrue、不可能であればfalse。
	 */
	bool get_can_focus() override {
		return m_tree_view.get_can_focus();
	}

	/**
	 * @brief TreeViewの感応可否を設定します。
	 * @param[in] sensitive 感応可能にする場合はtrue、無効にする場合はfalse。
	 */
	void set_sensitive(bool sensitive) override {
		m_tree_view.set_sensitive(sensitive);
	}

	/**
	 * @brief TreeViewの感応可否を取得します。
	 * @return 感応可能であればtrue、無効であればfalse。
	 */
	bool get_sensitive() override {
		return m_tree_view.get_sensitive();
	}

	/**
	 * @brief TreeViewの表示・非表示を設定します。
	 * @param[in] visible 表示する場合はtrue、非表示にする場合はfalse。
	 */
	void set_visible(bool visible) override {
		m_tree_view.set_visible(visible);
		if (m_enclosing_widget) {
			// sg::Widget に set_visible が無くても、これが万能に動作します
			gtk_widget_set_visible(*m_enclosing_widget, visible);
		}
	}

	/**
	 * @brief TreeViewの表示・非表示を取得します。
	 * @return 表示されていればtrue、非表示であればfalse。
	 */
	bool get_visible() override {
		return m_tree_view.get_visible();
	}

	/**
	 * @brief 選択モードを設定します。
	 * @param[in] type GtkSelectionMode 列挙型のいずれかの値。
	 */
	void set_selection_mode(GtkSelectionMode type) override {
		utils::scoped_value_guard guard(this->m_is_programmatically_changing, true);
		m_tree_selection.set_mode(type);	// GtkTreeSelection の set_mode を呼ぶ
	}

	/**
	 * @brief 選択モードを取得します。
	 * @return 現在の選択モード。
	 */
	GtkSelectionMode get_selection_mode() override {
		return m_tree_selection.get_mode();
	}

	/**
	 * @brief 全ての選択を解除します。
	 * @details このメソッドは、他のテーブルの選択が変更されたときに
	 * 呼び出されることを想定しています。これにより、複数のテーブル間で
	 * 選択状態を同期させることができます。
	 */
	void unselect_all() override {
		utils::scoped_value_guard guard(this->m_is_programmatically_changing, true);
		m_tree_selection.unselect_all();	// GtkTreeSelection の unselect_all を呼ぶ
	}

	// --- TreeView特化の機能 ---

	/**
	 * @brief ユーザーが選択を変更したときに呼び出されるコールバックを登録します。
	 * @param[in] callback 選択された行のインデックスを受け取る関数オブジェクト。
	 * インデックスが無効な場合は std::nullopt が渡されます。
	 */
	void on_selection_changed_by_user(std::function<void(std::optional<size_type>)> callback) {
		m_user_selection_callback = callback;
	}

	/**
	 * @brief 現在TreeViewで選択されている行のインデックスを取得します。
	 * @warning [使用上の注意]
	 * この関数は、呼び出された瞬間の選択状態を問い合わせます。
	 * get_user_selection_callback() で登録したコールバック関数内で
	 * 「どの行が選択されたか」を知るためにこの関数を呼び出すのは
	 * 非推奨です (タイミング問題が発生する可能性があります)。
	 * コールバック内では、引数として渡されるインデックスを使用してください。
	 * @note
	 * 「確定」ボタンのクリック時など、選択変更イベント以外の場所から
	 * 現在の選択状態を問い合わせる目的で使うのは安全です。
	 * @return 選択されている行のインデックス。なければ空のoptional。
	 */
	std::optional<size_type> get_selected_index() {
		GtkTreeIter iter;
		if (m_tree_selection.get_selected(nullptr, &iter)) {
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
	 * @warning [使用上の注意]
	 * この関数は、呼び出された瞬間の選択状態を問い合わせます。
	 * get_user_selection_callback() で登録したコールバック関数内で
	 * 「どの行が選択されたか」を知るためにこの関数を呼び出すのは
	 * 非推奨です (タイミング問題が発生する可能性があります)。
	 * コールバック内では、引数として渡されるインデックスを使用してください。
	 * @note
	 * 「確定」ボタンのクリック時など、選択変更イベント以外の場所から
	 * 現在の選択状態を問い合わせる目的で使うのは安全です。
	 * @return 選択されていれば行データ、なければ空のoptional。
	 */
	std::optional<value_type> get_selected_value() {
		auto index_opt = get_selected_index();
		if (index_opt.has_value()) {
			return this->at(index_opt.value());
		}
		return std::nullopt;
	}

	/**
	 * @brief 現在選択されている行のデータをC++モデルから取得します。（旧・互換版）
	 * @deprecated Use 'get_selected_value' instead.
	 */
	[[deprecated("Use 'get_selected_value' instead.")]]
	std::optional<value_type> get_selected_item() {
		return get_selected_value();
	}

	/**
	 * @brief 現在選択されている行のインデックスと行データのペアを取得します。
	 * @warning [使用上の注意]
	 * この関数は、呼び出された瞬間の選択状態を問い合わせます。
	 * get_user_selection_callback() で登録したコールバック関数内で
	 * 「どの行が選択されたか」を知るためにこの関数を呼び出すのは
	 * 非推奨です (タイミング問題が発生する可能性があります)。
	 * コールバック内では、引数として渡されるインデックスを使用してください。
	 * @note
	 * 「確定」ボタンのクリック時など、選択変更イベント以外の場所から
	 * 現在の選択状態を問い合わせる目的で使うのは安全です。
	 * @return 選択されていればインデックスと行データのペア、なければ空のoptional。
	 */
	std::optional<indexed_value_pair> get_selected_indexed_value() {
		auto index_opt = get_selected_index();
		if (index_opt.has_value()) {
			return std::make_pair(index_opt.value(), this->at(index_opt.value()));
		}
		return std::nullopt;
	}

	/**
	 * @brief 現在選択されている行のインデックスと行データのペアを取得します。（旧・互換版）
	 * @deprecated Use 'get_selected_indexed_value' instead.
	 */
	[[deprecated("Use 'get_selected_indexed_value' instead.")]]
	std::optional<indexed_value_pair> get_selected_index_and_item() {
		return get_selected_indexed_value();
	}
};

/**
 * @class PaginatedListStoreModel
 * @brief 複数のTreeViewListStoreModelを1ページとして扱い、ページネーション機能を提供するクラス。
 * @details 全データ(マスターデータ)を内部に保持し、データ操作とページ表示を完全にカプセル化します。
 * @tparam _Tp 1行分のデータを表現する構造体の型。
 */
template<typename _Tp>
class PaginatedListStoreModel : public ListStoreViewInterface {
public:
	// --- 型定義 ---
	using vector_type				= std::vector<_Tp>;	///< 内部データモデルとして使用する std::vector の型エイリアス
	using value_type				= typename vector_type::value_type;
	using size_type					= typename vector_type::size_type;
	using const_iterator			= typename vector_type::const_iterator;
	using const_reverse_iterator	= typename vector_type::const_reverse_iterator;

	using indexed_value_pair		= std::pair<size_type, value_type>;		///< インデックスと value_type のペア型エイリアス
	using model_type				= TreeViewListStoreModel<value_type>;	///< 内部で使用する TreeViewListStoreModel の型エイリアス

private:
	/// @brief (Model) 全てのページのマスターデータ
	vector_type m_master_data;
	/// @brief 1つのTreeViewが表示できる最大行数
	size_type m_rows_per_view;
	/// @brief 1ページに含まれるTreeViewListStoreModelのリスト
	std::vector<model_type*> m_page_models;
	/// @brief 現在表示中のページ番号 (0から始まるインデックス)
	int m_current_page_index;
	/// @brief 外部に通知するコールバック (グローバルインデックスを渡す)
	std::function<void(std::optional<size_type>)> m_user_selection_callback;
	/// @brief モデルがプログラムによって更新されている最中かを示すフラグ
	bool m_is_programmatically_changing;
	/// @brief フォーカス可否
	bool m_can_focus;
	/// @brief 感応可否
	bool m_sensitive;
	/// @brief 表示・非表示
	bool m_visible;
	/// @brief TreeView の列数
	guint m_n_columns;
	/// @brief カラムタイトルのリスト
	std::vector<std::string> m_titles;
	/// @brief グリッド線の表示状態
	GtkTreeViewGridLines m_grid_lines;
	/// @brief 選択モード
	GtkSelectionMode m_selection_mode;

	/**
	 * @brief (内部ハンドラ) TreeViewListStoreModel の user_selection_changed に接続
	 */
	void internal_on_selection_changed(size_type view_index, std::optional<size_type> local_index_opt) {
		// この PaginatedListStoreModel 自身がモデル更新中なら無視
		if (m_is_programmatically_changing) {
			return;
		}

		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		// 他のすべての TreeView の選択を解除する
		for (size_type i = 0; i < m_page_models.size(); i++) {
			if (i != view_index) {
				m_page_models[i]->unselect_all();
			}
		}

		// 外部コールバックが登録されていれば、グローバルインデックスを計算して呼び出す
		if (m_user_selection_callback) {
			std::optional<size_type> global_index_opt = std::nullopt;
			if (local_index_opt.has_value()) {
				global_index_opt = (m_current_page_index * m_rows_per_view * m_page_models.size()) + (view_index * m_rows_per_view) + local_index_opt.value();
			}
			m_user_selection_callback(global_index_opt);
		}
	}

public:
	/// @brief ページネーションモデルの設定構造体
	struct Config {
		/// @brief フォーカス可否
		bool can_focus = true;
		/// @brief 感応可否
		bool sensitive = true;
		/// @brief 表示・非表示
		bool visible = true;
		/// @brief カラムタイトルのリスト
		std::vector<std::string> titles = {};
		/// @brief グリッド線の表示状態
		GtkTreeViewGridLines grid_lines = GtkTreeViewGridLines::GTK_TREE_VIEW_GRID_LINES_NONE;
		/// @brief 選択モード
		GtkSelectionMode selection_mode = GtkSelectionMode::GTK_SELECTION_SINGLE;
	};

	/**
	 * @brief コンストラクタ
	 * @param[in] rows_per_view 最大行数
	 * @param[in] n_columns 列数
	 * @param[in] config (オプション) その他の設定
	 */
	PaginatedListStoreModel(
		size_type rows_per_view,
		guint n_columns,
		Config config = {} // デフォルト値で初期化
	) :
		m_rows_per_view(rows_per_view),
		m_current_page_index(0),
		m_is_programmatically_changing(false),
		m_can_focus(config.can_focus),
		m_sensitive(config.sensitive),
		m_visible(config.visible),
		m_n_columns(n_columns),
		m_titles(std::move(config.titles)),	// ムーブ
		m_grid_lines(config.grid_lines),
		m_selection_mode(config.selection_mode)
	{
		m_titles.resize(n_columns);
	}

	// コピー・ムーブを完全に禁止（thisキャプチャがあるため必須）
	PaginatedListStoreModel(const PaginatedListStoreModel&) = delete;
	PaginatedListStoreModel& operator=(const PaginatedListStoreModel&) = delete;
	PaginatedListStoreModel(PaginatedListStoreModel&&) = delete;
	PaginatedListStoreModel& operator=(PaginatedListStoreModel&&) = delete;

	// --- イテレータ対応 (読み取り専用) ---

	const_iterator begin() const { return m_master_data.begin(); }
	const_iterator end() const { return m_master_data.end(); }
	const_reverse_iterator rbegin() const noexcept { return m_master_data.rbegin(); }
	const_reverse_iterator rend() const noexcept { return m_master_data.rend(); }
	const_iterator cbegin() const { return m_master_data.cbegin(); }
	const_iterator cend() const { return m_master_data.cend(); }
	const_reverse_iterator crbegin() const noexcept { return m_master_data.crbegin(); }
	const_reverse_iterator crend() const noexcept { return m_master_data.crend(); }

	/**
	 * @brief 指定されたページのデータを各TreeViewに表示する内部関数。
	 * @param[in] page_index 表示するページのインデックス (0から始まる)。
	 */
	void display_page(int page_index) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
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
		size_type total_rows_per_page = m_rows_per_view * m_page_models.size();

		if (total_rows_per_page == 0) { // モデルが登録されていない場合は何もしない
			m_current_page_index = 0; // ページ番号をリセット
			return;
		}

		// 表示を開始するマスターデータのインデックス
		size_type start_index = page_index * total_rows_per_page;

		// マスターデータから現在のページに表示すべき部分を切り出して表示
		for (size_type i = 0; i < total_rows_per_page; i++) {
			size_type master_index = start_index + i;
			if (master_index >= m_master_data.size()) { break; }	// マスターデータの終端を超えたらループ終了

			// どのTreeViewに表示するかを計算
			size_type view_index = i / m_rows_per_view;
			if (view_index < m_page_models.size()) {	// 念のため範囲チェック
				m_page_models[view_index]->append(m_master_data[master_index]);
			}
		}

		for (auto* model : m_page_models) {
			// 親(このクラス)が表示設定(m_visible)になっており、かつモデルに行データが存在する場合のみ表示する
			model->set_visible(m_visible && (model->size() > 0));
		}
		m_current_page_index = page_index;
	}

	/**
	 * @brief 現在表示中のページの表示を更新します。
	 * @param[in] force_repopulate (オプション・デフォルト false)
	 * trueの場合、ページを再構築します（マスターデータからデータを再取得し、リストを作り直します）。
	 * falseの場合、各子モデルのrefresh(false)を呼び出し、再描画のみ行います（外部変数による表示切り替えなどに使用）。
	 */
	void refresh(bool force_repopulate = false) {
		// trueなら構造再構築（Master Data -> Child Model -> View）
		if (force_repopulate) {
			display_page(m_current_page_index);
			return;
		}

		// falseなら各子モデルの再描画（Child Model -> View）
		// データそのものは変更せず、セッター関数を再実行して表示を更新します。
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		for (auto* model : m_page_models) {
			if (model) {
				model->refresh(false);
			}
		}
	}

	/**
	 * @brief ユーザーが選択を変更したときに呼び出されるコールバックを登録します。
	 * @param[in] callback 選択された行のグローバルインデックスを受け取る関数オブジェクト。
	 * インデックスが無効な場合は std::nullopt が渡されます。
	 */
	void on_selection_changed_by_user(std::function<void(std::optional<size_type>)> callback) {
		m_user_selection_callback = callback;
	}

	/**
	 * @brief このページネーション管理に追加する TreeViewListStoreModel を登録します。
	 * @param[in, out] model 管理対象のモデルへのポインタ。
	 * @param[in] update_ui (オプション・デフォルト true) trueの場合、登録後に画面を更新します。
	 * @return 登録に成功した場合はtrue、失敗した場合はfalse。
	 */
	bool add_model(model_type* model, bool update_ui = true) {
		// NULLチェック
		if (!model) {
			return false; // 無効なモデルポインタ
		}

		// 列数チェック
		if (model->get_n_columns() != m_n_columns) {
			return false; // 列数が一致しない
		}

		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		// view_index は、モデルが m_page_models に追加される前の .size()
		const size_type view_index = m_page_models.size();
		m_page_models.push_back(model);

		// 登録したモデルのプロパティを親に合わせる
		model->set_titles(m_titles);
		model->set_grid_lines(m_grid_lines);
		model->set_can_focus(m_can_focus);
		model->set_sensitive(m_sensitive);
		model->set_visible(m_visible);
		model->set_selection_mode(m_selection_mode);
		model->reserve(m_rows_per_view);
		model->unselect_all();

		// 子モデルのコールバックに、親のハンドラを登録
		model->on_selection_changed_by_user(
			[this, view_index](std::optional<size_type> local_index_opt) {
				this->internal_on_selection_changed(view_index, local_index_opt);
			}
		);

		if (update_ui) {
			display_page(m_current_page_index);
		}

		return true;
	}

	/**
	 * @brief 複数のTreeViewListStoreModelを一括で登録します。
	 * @param[in, out] models 管理対象のモデルへのポインタのvector。
	 * @param[in] update_ui (オプション・デフォルト true) trueの場合、登録後に画面を更新します。
	 * @return 登録に成功した場合はtrue、失敗した場合はfalse。
	 */
	bool add_model(const std::vector<model_type*>& models, bool update_ui = true) {
		// 登録前に全てのモデルをチェック
		for (auto* model : models) {
			// NULLチェック
			if (!model) {
				return false; // 無効なモデルポインタ
			}

			// 列数チェック
			if (model->get_n_columns() != m_n_columns) {
				return false; // 列数が一致しない
			}
		}

		// 全てのモデルが有効なら登録を実行
		for (auto* model : models) {
			add_model(model, false); // 個別のUI更新はしない
		}

		if (update_ui) {
			display_page(m_current_page_index);
		}

		return true;
	}

	/**
	 * @brief 登録されている全てのTreeViewListStoreModelをクリアします。
	 * @details マスターデータが空のときのみクリアを許可します。
	 * @return クリアに成功した場合はtrue、マスターデータが空でない場合はfalse。
	 */
	bool clear_models() {
		// マスターデータが空のときのみクリアを許可
		if (!m_master_data.empty()) { return false; }

		// 各モデルに登録したコールバックを解除する
		for (auto* model : m_page_models) {
			if (model) {
				model->on_selection_changed_by_user(nullptr);
			}
		}

		m_page_models.clear();
		return true;
	}

	/**
	 * @brief 1つのTreeViewが表示できる最大行数を設定します。
	 * @param[in] rows_per_view 1つのTreeViewが表示できる最大行数。
	 * @param[in] refresh_current_page (オプション・デフォルト false) 設定後に、trueの場合は現在のページを、falseの場合は最初のページを、再表示します。
	 */
	void set_rows_per_view(size_type rows_per_view, bool refresh_current_page = false) {
		if (rows_per_view == m_rows_per_view) {
			return; // 変更なし
		}
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_rows_per_view = rows_per_view;
		display_page(((refresh_current_page) ? (m_current_page_index) : (0)));
	}

	/**
	 * @brief 1つのTreeViewが表示できる最大行数を取得します。
	 * @return 1つのTreeViewが表示できる最大行数。
	 */
	size_type get_rows_per_view() const {
		return m_rows_per_view;
	}

	// --- ListStoreViewInterface の実装 (override) ---

	/**
	 * @brief 指定したインデックスのカラムのタイトルを設定します。
	 * @details 変更がない場合は更新しません。
	 * @param[in] n カラムのインデックス (0から始まる)。
	 * @param[in] title 設定するタイトル文字列。
	 * @return 成功すればtrue、指定されたインデックスのカラムが存在しない場合はfalse。
	 */
	bool set_title(gint n, const gchar* title) override {
		return set_title(n, title, false);
	}

		/**
	 * @brief 指定したインデックスのカラムのタイトルを設定します。
	 * @param[in] n カラムのインデックス (0から始まる)。
	 * @param[in] title 設定するタイトル文字列。
	 * @param[in] force_update 強制的に更新する場合はtrue、変更がない場合は更新しない場合はfalse。
	 * @return 成功すればtrue、指定されたインデックスのカラムが存在しない場合はfalse。
	 */
	bool set_title(gint n, const gchar* title, bool force_update) {
		// 列数チェック
		if (n < 0 || static_cast<guint>(n) >= m_n_columns) {
			return false; // 列数が一致しない
		}
		if (m_titles[n] == title && !force_update) {
			return true; // 変更なし
		}

		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_titles[n] = std::string(title);
		for (auto* model : m_page_models) {
			model->set_title(n, title);
		}
		return true;
	}

	/**
	 * @brief 複数のカラムのタイトルを一括でコピー設定します。
	 * @details 変更がない場合は更新しません。
	 * @param[in] titles 各カラムのタイトル文字列を格納したvector。
	 * インデックス0が最初のカラム、1が2番目のカラム、という順序になります。
	 * @return 成功すればtrue、指定されたインデックスのカラムが存在しない場合はfalse。
	 */
	bool set_titles(const std::vector<std::string>& titles) override {
		return set_titles(titles, false);
	}

	/**
	 * @brief 複数のカラムのタイトルを一括でコピー設定します。
	 * @param[in] titles 各カラムのタイトル文字列を格納したvector。
	 * @param[in] force_update 強制的に更新する場合はtrue、変更がない場合は更新しない場合はfalse。
	 * インデックス0が最初のカラム、1が2番目のカラム、という順序になります。
	 * @return 成功すればtrue、指定されたインデックスのカラムが存在しない場合はfalse。
	 */
	bool set_titles(const std::vector<std::string>& titles, bool force_update) {
		// 列数チェック
		if (titles.size() != m_n_columns) {
			return false; // 列数が一致しない
		}
		if (m_titles == titles && !force_update) {
			return true; // 変更なし
		}

		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_titles = titles;
		for (auto* model : m_page_models) {
			model->set_titles(m_titles);
		}
		return true;
	}

	/**
	 * @brief 複数のカラムのタイトルを一括でムーブ設定します。
	 * @details 変更がない場合は更新しません。
	 * @param[in] titles 各カラムのタイトル文字列を格納したvector。
	 * インデックス0が最初のカラム、1が2番目のカラム、という順序になります。
	 * @return 成功すればtrue、指定されたインデックスのカラムが存在しない場合はfalse。
	 */
	bool set_titles(std::vector<std::string>&& titles) override {
		return set_titles(std::move(titles), false);
	}

	/**
	 * @brief 複数のカラムのタイトルを一括でムーブ設定します。
	 * @param[in] titles 各カラムのタイトル文字列を格納したvector。
	 * @param[in] force_update 強制的に更新する場合はtrue、変更がない場合は更新しない場合はfalse。
	 * インデックス0が最初のカラム、1が2番目のカラム、という順序になります。
	 * @return 成功すればtrue、指定されたインデックスのカラムが存在しない場合はfalse。
	 */
	bool set_titles(std::vector<std::string>&& titles, bool force_update) {
		// 列数チェック
		if (titles.size() != m_n_columns) {
			return false; // 列数が一致しない
		}
		if (m_titles == titles && !force_update) {
			return true; // 変更なし
		}

		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_titles = std::move(titles);
		for (auto* model : m_page_models) {
			model->set_titles(m_titles);
		}
		return true;
	}

	/**
	 * @brief 指定したインデックスのカラムのタイトルを取得します。
	 * @param[in] n カラムのインデックス (0から始まる)。
	 * @return カラムのタイトル文字列。指定されたインデックスのカラムが存在しない場合は空のoptional。
	 */
	std::optional<std::string> get_title(gint n) override {
		// 列数チェック
		if (n < 0 || static_cast<guint>(n) >= m_n_columns) {
			return std::nullopt; // 列数が一致しない
		}
		return m_titles.at(n);
	}

	/**
	 * @brief カラムタイトルのリストを取得します。
	 * @return カラムタイトルのリスト。
	 */
	std::vector<std::string> get_titles() override {
		return m_titles;
	}

	/**
	 * @brief カラムタイトルのリストの参照を取得します。
	 * @return カラムタイトルのリストの const 参照。
	 */
	const std::vector<std::string>& get_titles_ref() {
		return m_titles;
	}

	/**
	 * @brief TreeViewの列数を取得します。
	 * @return 列数。
	 */
	guint get_n_columns() override {
		return m_n_columns;
	}

	/**
	 * @brief TreeViewのグリッド線の表示状態を取得します。
	 * @return グリッド線の表示状態。
	 */
	GtkTreeViewGridLines get_grid_lines() override {
		return m_grid_lines;
	}

	/**
	 * @brief TreeViewのグリッド線の表示状態を設定します。
	 * @details 変更がない場合は更新しません。
	 * @param[in] grid_lines グリッド線の表示状態。
	 */
	void set_grid_lines(GtkTreeViewGridLines grid_lines) override {
		set_grid_lines(grid_lines, false);
	}

	/**
	 * @brief TreeViewのグリッド線の表示状態を設定します。
	 * @param[in] grid_lines グリッド線の表示状態。
	 * @param[in] force_update 強制的に更新する場合はtrue、変更がない場合は更新しない場合はfalse。
	 */
	void set_grid_lines(GtkTreeViewGridLines grid_lines, bool force_update) {
		if (m_grid_lines == grid_lines && !force_update) {
			return; // 変更なし
		}
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_grid_lines = grid_lines;
		for (auto* model : m_page_models) {
			model->set_grid_lines(grid_lines);
		}
	}

	/**
	 * @brief TreeViewのフォーカス可否を設定します。
	 * @details 変更がない場合は更新しません。
	 * @param[in] can_focus フォーカス可能にする場合はtrue、不可能にする場合はfalse。
	 */
	void set_can_focus(bool can_focus) override {
		set_can_focus(can_focus, false);
	}

	/**
	 * @brief TreeViewのフォーカス可否を設定します。
	 * @param[in] can_focus フォーカス可能にする場合はtrue、不可能にする場合はfalse。
	 * @param[in] force_update 強制的に更新する場合はtrue、変更がない場合は更新しない場合はfalse。
	 */
	void set_can_focus(bool can_focus, bool force_update) {
		if (m_can_focus == can_focus && !force_update) {
			return; // 変更なし
		}
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_can_focus = can_focus;
		for (auto* model : m_page_models) {
			model->set_can_focus(can_focus);
		}
	}

	/**
	 * @brief TreeViewのフォーカス可否を取得します。
	 * @return フォーカス可能であればtrue、不可能であればfalse。
	 */
	bool get_can_focus() override {
		return m_can_focus;
	}

	/**
	 * @brief TreeViewの感応可否を設定します。
	 * @details 変更がない場合は更新しません。
	 * @param[in] sensitive 感応可能にする場合はtrue、無効にする場合はfalse。
	 */
	void set_sensitive(bool sensitive) override {
		set_sensitive(sensitive, false);
	}

	/**
	 * @brief TreeViewの感応可否を設定します。
	 * @param[in] sensitive 感応可能にする場合はtrue、無効にする場合はfalse。
	 * @param[in] force_update 強制的に更新する場合はtrue、変更がない場合は更新しない場合はfalse。
	 */
	void set_sensitive(bool sensitive, bool force_update) {
		if (m_sensitive == sensitive && !force_update) {
			return; // 変更なし
		}
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_sensitive = sensitive;
		for (auto* model : m_page_models) {
			model->set_sensitive(sensitive);
		}
	}

	/**
	 * @brief TreeViewの感応可否を取得します。
	 * @return 感応可能であればtrue、無効であればfalse。
	 */
	bool get_sensitive() override {
		return m_sensitive;
	}

	/**
	 * @brief TreeViewの表示・非表示を設定します。
	 * @details 変更がない場合は更新しません。
	 * @param[in] visible 表示する場合はtrue、非表示にする場合はfalse。
	 */
	void set_visible(bool visible) override {
		set_visible(visible, false);
	}

	/**
	 * @brief TreeViewの表示・非表示を設定します。
	 * @param[in] visible 表示する場合はtrue、非表示にする場合はfalse。
	 * @param[in] force_update 強制的に更新する場合はtrue、変更がない場合は更新しない場合はfalse。
	 */
	void set_visible(bool visible, bool force_update) {
		if (m_visible == visible && !force_update) {
			return; // 変更なし
		}
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_visible = visible;
		for (auto* model : m_page_models) {
			// display_page と同様のロジックで判定
			model->set_visible(m_visible && (model->size() > 0));
		}
	}

	/**
	 * @brief TreeViewの表示・非表示を取得します。
	 * @return 表示されていればtrue、非表示であればfalse。
	 */
	bool get_visible() override {
		return m_visible;
	}

	/**
	 * @brief 選択モードを設定します。
	 * @details 変更がない場合は更新しません。
	 * @param[in] type GtkSelectionMode 列挙型のいずれかの値。
	 */
	void set_selection_mode(GtkSelectionMode type) override {
		set_selection_mode(type, false);
	}

	/**
	 * @brief 選択モードを設定します。
	 * @param[in] type GtkSelectionMode 列挙型のいずれかの値。
	 * @param[in] force_update 強制的に更新する場合はtrue、変更がない場合は更新しない場合はfalse。
	 */
	void set_selection_mode(GtkSelectionMode type, bool force_update) {
		if (m_selection_mode == type && !force_update) {
			return; // 変更なし
		}
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_selection_mode = type;
		for (auto* model : m_page_models) {
			model->set_selection_mode(type);
		}
	}

	/**
	 * @brief 選択モードを取得します。
	 * @return 現在の選択モード。
	 */
	GtkSelectionMode get_selection_mode() override {
		return m_selection_mode;
	}

	/**
	 * @brief 現在表示されているページで選択されている行を全て解除します。
	 */
	void unselect_all() override {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		for (auto& model : m_page_models) {
			model->unselect_all();
		}
	}

	/**
	 * @brief 次のページへ切り替えます。
	 * @return 次のページが存在し、切り替えに成功した場合はtrue。
	 */
	bool next_page() {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
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
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
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
		if (m_master_data.empty()) { return 1; }
		size_type total_rows_per_page = m_rows_per_view * m_page_models.size();
		if (total_rows_per_page == 0) { return 1; }
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
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 */
	void clear(bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_master_data.clear();
		if (update_display) {
			display_page(0); // 0ページ目を表示（空になる）
		}
	}

	/**
	 * @brief 内部データモデルの容量を事前に確保します。
	 * @param[in] new_cap 確保する新しい容量。
	 */
	void reserve(size_type new_cap) {
		m_master_data.reserve(new_cap);
	}

	/**
	 * @brief マスターデータの末尾に単一の行をコピー追加し、UIを更新します。
	 * @param[in] value 追加する行のデータ。
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 * @return 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
	 */
	bool append(const value_type& value, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		if (size() >= (size_type)(std::numeric_limits<int>::max())) { return false; }
		m_master_data.push_back(value);
		if (update_display) {
			display_page(m_current_page_index);
		}
		return true;
	}

	/**
	 * @brief マスターデータの末尾に単一の行をムーブ追加し、UIを更新します。
	 * @param[in] value 追加する行のデータ（右辺値）。
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 * @return 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
	 */
	bool append(value_type&& value, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		if (size() >= (size_type)(std::numeric_limits<int>::max())) { return false; }

		// vector にムーブ
		m_master_data.push_back(std::move(value));

		if (update_display) {
			display_page(m_current_page_index);
		}
		return true;
	}

	/**
	 * @brief マスターデータの末尾に複数の行をまとめてコピー追加し、UIを更新します。
	 * @param[in] values 追加する行データのvector。
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 * @return 追加に成功すればtrue、行数がintの最大値を超える場合はfalse。
	 */
	bool append(const vector_type& values, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		if (size() + values.size() > (size_type)(std::numeric_limits<int>::max())) { return false; }
		m_master_data.insert(m_master_data.end(), values.begin(), values.end());
		if (update_display) {
			display_page(m_current_page_index);
		}
		return true;
	}

	/**
	 * @brief マスターデータの末尾に複数の行をまとめてムーブ追加し、UIを更新します。
	 * @param[in] values 追加する行データのvector（右辺値）。
	 * @param[in] update_display UI更新フラグ。
	 * @return 成功ならtrue。
	 */
	bool append(vector_type&& values, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		if (size() + values.size() > (size_type)(std::numeric_limits<int>::max())) { return false; }

		// std::make_move_iterator を使って、vectorの要素を一括で移動させる
		m_master_data.insert(
			m_master_data.end(),
			std::make_move_iterator(values.begin()),
			std::make_move_iterator(values.end())
		);

		if (update_display) {
			display_page(m_current_page_index);
		}
		return true;
	}

	/**
	 * @brief マスターデータの指定したグローバルインデックスに行をコピー挿入し、UIを更新します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * size()以上の値を指定した場合は、末尾に追加(append)されます。
	 * @param[in] global_position 挿入位置のグローバルインデックス。
	 * @param[in] value 挿入する行のデータ。
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool insert(size_type global_position, const value_type& value, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		size_type original_position = global_position;
		if (size() >= (size_type)(std::numeric_limits<int>::max())) { return false; }

		if (global_position > size()) { global_position = size(); }

		m_master_data.insert(m_master_data.begin() + global_position, value);
		if (update_display) {
			display_page(m_current_page_index);
		}
		return global_position == original_position;
	}

	/**
	 * @brief マスターデータの指定したグローバルインデックスに行をムーブ挿入し、UIを更新します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * size()以上の値を指定した場合は、末尾に追加(append)されます。
	 * @param[in] global_position 挿入位置のグローバルインデックス。
	 * @param[in] value 挿入する行のデータ（右辺値）。
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool insert(size_type global_position, value_type&& value, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		size_type original_position = global_position;
		if (size() >= (size_type)(std::numeric_limits<int>::max())) { return false; }

		if (global_position > size()) { global_position = size(); }

		m_master_data.insert(m_master_data.begin() + global_position, std::move(value));

		if (update_display) {
			display_page(m_current_page_index);
		}
		return global_position == original_position;
	}

	/**
	 * @brief マスターデータの指定したグローバルインデックスに複数の行をまとめてコピー挿入し、UIを更新します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * size()以上の値を指定した場合は、末尾に追加(append)されます。
	 * @param[in] global_position 挿入位置のグローバルインデックス。
	 * @param[in] values 挿入する行データのvector。
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool insert(size_type global_position, const vector_type& values, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		if (size() + values.size() > (size_type)(std::numeric_limits<int>::max())) { return false; }
		bool position_was_valid = (global_position <= size());

		if (global_position > size()) { global_position = size(); }

		m_master_data.insert(m_master_data.begin() + global_position, values.begin(), values.end());
		if (update_display) {
			display_page(m_current_page_index);
		}
		return position_was_valid;
	}

	/**
	 * @brief マスターデータの指定したグローバルインデックスに複数の行をまとめてムーブ挿入し、UIを更新します。
	 * @details positionが範囲外の場合、有効な範囲([0, size()])に丸められます。
	 * size()以上の値を指定した場合は、末尾に追加(append)されます。
	 * @param[in] global_position 挿入位置。
	 * @param[in] values 挿入する行データのvector（右辺値）。
	 * @param[in] update_display UI更新フラグ。
	 * @return 指定された位置通りに挿入できればtrue、位置が丸められたか、行数が最大値を超えた場合はfalse。
	 */
	bool insert(size_type global_position, vector_type&& values, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		if (size() + values.size() > (size_type)(std::numeric_limits<int>::max())) { return false; }
		bool position_was_valid = (global_position <= size());

		if (global_position > size()) { global_position = size(); }

		// std::make_move_iterator で一括移動挿入
		m_master_data.insert(
			m_master_data.begin() + global_position,
			std::make_move_iterator(values.begin()),
			std::make_move_iterator(values.end())
		);

		if (update_display) {
			display_page(m_current_page_index);
		}
		return position_was_valid;
	}

	/**
	 * @brief マスターデータの指定したグローバルインデックスの行を削除し、UIを更新します。
	 * @param[in] global_position 削除する行のグローバルインデックス。
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 * @return 削除に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool erase(size_type global_position, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		if (global_position >= size()) { return false; }
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
	 * @param[in] global_positions 削除する行のグローバルインデックスのリスト。
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 * @return 指定されたインデックスが全て有効な場合のみtrue。一つでも無効なら処理せずfalse。
	 */
	bool erase(std::vector<size_type> global_positions, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		for (size_type pos : global_positions) {
			if (pos >= size()) { return false; }
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
	 * @brief マスターデータの指定したグローバルインデックスの行をコピー上書きし、UIを更新します。
	 * @param[in] global_position 上書きする行のグローバルインデックス。
	 * @param[in] value 新しい行データ。
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 * @return 更新に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool update(size_type global_position, const value_type& value, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		if (global_position >= size()) { return false; }

		m_master_data[global_position] = value;
		if (update_display) {
			display_page(m_current_page_index);
		}

		return true;
	}

	/**
	 * @brief マスターデータの指定したグローバルインデックスの行をコピー上書きします。
	 * @param[in] indexed_value 上書き位置のインデックスと新しい行データのペア。
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 * @return 更新に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool update(const indexed_value_pair& indexed_value, bool update_display = true) {
		return update(indexed_value.first, indexed_value.second, update_display);	// 別のオーバーロードへ処理を委譲
	}

	/**
	 * @brief マスターデータの指定したグローバルインデックスの行をムーブ上書きし、UIを更新します。
	 * @param[in] global_position 上書きする行のグローバルインデックス。
	 * @param[in] value 新しい行データ（右辺値）。
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 * @return 更新に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool update(size_type global_position, value_type&& value, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		if (global_position >= size()) { return false; }

		m_master_data[global_position] = std::move(value);
		if (update_display) {
			display_page(m_current_page_index);
		}

		return true;
	}

	/**
	 * @brief マスターデータの指定したグローバルインデックスの行をムーブ上書きします。
	 * @param[in] indexed_value 上書き位置のインデックスと新しい行データのペア（右辺値）。
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 * @return 更新に成功すればtrue、インデックスが無効な場合はfalse。
	 */
	bool update(indexed_value_pair&& indexed_value, bool update_display = true) {
		return update(indexed_value.first, std::move(indexed_value.second), update_display);	// 別のオーバーロードへ処理を委譲
	}

	/**
	 * @brief マスターデータの指定した複数のグローバルインデックスの行をまとめてコピー上書きします。
	 * @param[in] updates 上書きする行のグローバルインデックスと新しい行データのmap。
	 * @param[in] update_display (オプション・デフォルト true) trueの場合に、画面を更新します。
	 * @return 指定されたインデックスが全て有効な場合のみtrue。一つでも無効なら処理せずfalse。
	 */
	bool update(const std::map<size_type, value_type>& updates, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		for (const auto& [pos, value] : updates) {
			if (pos >= size()) { return false; }
		}
		for (const auto& [pos, value] : updates) {
			m_master_data[pos] = value;
		}
		if (update_display) {
			display_page(m_current_page_index);
		}
		return true;
	}

	/**
	 * @brief マスターデータの指定した複数のグローバルインデックスの行をまとめてムーブ上書きします。
	 * @param[in] updates 更新マップ（右辺値）。
	 * @param[in] update_display UI更新フラグ。
	 * @return 成功ならtrue。
	 */
	bool update(std::map<size_type, value_type>&& updates, bool update_display = true) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		for (const auto& pair : updates) {
			if (pair.first >= size()) return false;
		}

		// mapからムーブして更新
		for (auto& [pos, value] : updates) {
			m_master_data[pos] = std::move(value);
		}

		if (update_display) {
			display_page(m_current_page_index);
		}
		return true;
	}

	/**
	 * @brief 全てのページの元となるマスターデータをコピーしてセットします。
	 * @details この関数を呼び出すと、データが内部にコピーされ、最初のページが表示されます。
	 * @param[in] all_data 全てのデータを含むvector (コピー元)。
	 * @param[in] refresh_current_page (オプション・デフォルト false) 編集後に、trueの場合は現在のページを、falseの場合は最初のページを、再表示します。
	 */
	void set_master_data(const vector_type& all_data, bool refresh_current_page = false) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_master_data = all_data; // コピーが発生
		display_page(((refresh_current_page) ? (m_current_page_index) : (0)));
	}

	/**
	 * @brief 全てのページの元となるマスターデータをムーブしてセットします。
	 * @details この関数を呼び出すと、データの所有権が効率的に移動され、最初のページが表示されます。
	 * 呼び出し元のvectorは空になります。
	 * @param[in] all_data 全てのデータを含むvector (ムーブ元)。
	 * @param[in] refresh_current_page (オプション・デフォルト false) 編集後に、trueの場合は現在のページを、falseの場合は最初のページを、再表示します。
	 */
	void set_master_data(vector_type&& all_data, bool refresh_current_page = false) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);
		m_master_data = std::move(all_data); // ムーブ (効率的)
		display_page(((refresh_current_page) ? (m_current_page_index) : (0)));
	}

	/**
	 * @brief マスターデータに新しいデータセットをコピーし、古いマスターデータを効率的に返します。
	 * @details
	 * 内部のマスターデータに `new_data` をコピーします。
	 * 置き換え前の古いマスターデータはムーブされて返されます。
	 * 操作後、UIは自動的に最初のページ（0ページ目）を表示するように更新されます。
	 * @param[in] new_data 新しく設定するマスターデータ。関数内でマスターデータにコピーされます。
	 * @param[in] refresh_current_page (オプション・デフォルト false) 編集後に、trueの場合は現在のページを、falseの場合は最初のページを、再表示します。
	 * @return 置き換えられる前の古いマスターデータ (ムーブされて返される)。
	 */
	vector_type replace_master_data(const vector_type& new_data, bool refresh_current_page = false) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		// 1. 現在のマスターデータを一時変数にムーブして退避
		vector_type old_data = std::move(m_master_data);

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
	 * @param[in] new_data 新しく設定するマスターデータ (ムーブ元)。この関数の呼び出し後、`new_data` は空（または有効だが未定義の状態）になります。
	 * @param[in] refresh_current_page (オプション・デフォルト false) 編集後に、trueの場合は現在のページを、falseの場合は最初のページを、再表示します。
	 * @return 置き換えられる前の古いマスターデータ (ムーブされて返される)。
	 */
	vector_type replace_master_data(vector_type&& new_data, bool refresh_current_page = false) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		// 1. 現在のマスターデータを一時変数にムーブして退避
		vector_type old_data = std::move(m_master_data);

		// 2. 引数で受け取った新しいデータをマスターデータにムーブ
		m_master_data = std::move(new_data);

		// 3. UIを更新
		display_page(((refresh_current_page) ? (m_current_page_index) : (0)));

		// 4. 退避しておいた古いデータを返す (これもムーブされる)
		return old_data;
	}

	/**
	 * @brief マスターデータを直接編集するための関数オブジェクトを受け付けます。
	 * @details
	 * この関数に渡されたラムダ式などの関数オブジェクト内で、マスターデータ(vector)を
	 * 直接変更できます (ソート、フィルタリング、要素の変更など)。
	 * 関数オブジェクトの実行が完了した後、自動的にUIが更新されます。
	 * @param[in] modifier マスターデータへの非const参照を受け取り、編集を行う関数オブジェクト。
	 * @param[in] refresh_current_page (オプション・デフォルト false) 編集後に、trueの場合は現在のページを、falseの場合は最初のページを、再表示します。
	 * trueの場合に現在のページが消滅していれば最終ページが表示されます。デフォルトはfalse。
	 */
	void modify_master_data(std::function<void(vector_type&)> modifier, bool refresh_current_page = false) {
		utils::scoped_value_guard guard(m_is_programmatically_changing, true);

		// 1. 渡された関数を実行し、m_master_data を直接変更してもらう
		modifier(m_master_data);

		// 2. 変更が完了したら、表示を更新する
		// display_page が範囲チェックと丸めを行うため、ここでは単純にページ番号を渡すだけでOK
		display_page(((refresh_current_page) ? (m_current_page_index) : (0)));
	}

	/**
	 * @brief 指定したグローバルインデックスの行データをマスターデータから取得します。
	 * @details std::vector::at と同様、範囲外アクセスの場合は例外が投げられます。
	 * @param[in] global_position 取得する行のグローバルインデックス。
	 * @return 行データへのconst参照。
	 */
	const value_type& at(size_type global_position) const {
		return m_master_data.at(global_position);
	}

	/**
	 * @brief 指定したグローバルインデックスの行データをマスターデータから取得します。（旧・互換版）
	 * @deprecated Use 'at' instead.
	 */
	[[deprecated("Use 'at' instead.")]]
	const value_type& get_item(size_type global_position) const {
		return at(global_position);
	}

	/**
	 * @brief マスターデータ内の全ての行データを取得します。
	 * @return 全ての行データを含むvectorへのconst参照。
	 */
	const vector_type& get_master_data() const {
		return m_master_data;
	}

	/**
	 * @brief マスターデータ内の現在の行数を返します。
	 * @return マスターデータ内の行数。
	 */
	size_type size() const {
		return m_master_data.size();
	}

	/**
	 * @brief マスターデータが空かどうかを返します。
	 * @return 空であればtrue、そうでなければfalse。
	 */
	bool empty() const {
		return m_master_data.empty();
	}

	/**
	 * @brief 条件に一致する全ての行のグローバルインデックスを検索します。
	 * @param[in] predicate 検索条件となる述語関数。
	 * @return 条件に一致した行のグローバルインデックスのリスト。
	 *
	 * ```cpp
	 * // 例: ageフィールドが30以上の行のインデックスを取得
	 * auto indices = find_indices([](const _Tp& value) {
	 * 	return value.age >= 30;
	 * });
	 * ```
	 */
	std::vector<size_type> find_indices(std::function<bool(const value_type&)> predicate) const {
		std::vector<size_type> found_indices;
		for (size_type i = 0; i < m_master_data.size(); i++) {
			if (predicate(m_master_data[i])) {
				found_indices.push_back(i);
			}
		}
		return found_indices;
	}

	/**
	 * @brief 条件に一致する全ての行データを検索します。
	 * @param[in] predicate 検索条件となる述語関数。
	 * @return 条件に一致した行データのリスト。
	 *
	 * ```cpp
	 * // 例: ageフィールドが30以上の行のインデックスを取得
	 * auto values = find_values([](const _Tp& value) {
	 * 	return value.age >= 30;
	 * });
	 * ```
	 */
	vector_type find_values(std::function<bool(const value_type&)> predicate) const {
		vector_type found_values;
		for (const auto& value : m_master_data) {
			if (predicate(value)) {
				found_values.push_back(value);
			}
		}
		return found_values;
	}

	/**
	 * @brief 条件に一致する全ての行データを検索します。（旧・互換版）
	 * @deprecated Use 'find_values' instead.
	 */
	[[deprecated("Use 'find_values' instead.")]]
	vector_type find_items(std::function<bool(const value_type&)> predicate) const {
		return find_values(predicate);
	}

	/**
	 * @brief 条件に一致する全ての行のグローバルインデックスと行データのペアを検索します。
	 * @param[in] predicate 検索条件となる述語関数。
	 * @return 条件に一致した行のグローバルインデックスと行データのペアのリスト。
	 *
	 * ```cpp
	 * // 例: ageフィールドが30以上の行のインデックスを取得
	 * std::vector<std::pair<size_t, _Tp>> pairs = find_indexed_value_pairs([](const _Tp& value) {
	 * 	return value.age >= 30;
	 * });
	 * ```
	 */
	std::vector<indexed_value_pair> find_indexed_value_pairs(std::function<bool(const value_type&)> predicate) const {
		std::vector<indexed_value_pair> found_pairs;
		for (size_type i = 0; i < m_master_data.size(); i++) {
			if (predicate(m_master_data[i])) {
				found_pairs.emplace_back(i, m_master_data[i]);
			}
		}
		return found_pairs;
	}

	/**
	 * @brief 条件に一致する全ての行のインデックスと行データのペアを検索します。（旧・互換版）
	 * @deprecated Use 'find_indexed_value_pairs' instead.
	 */
	[[deprecated("Use 'find_indexed_value_pairs' instead.")]]
	std::vector<indexed_value_pair> find_indices_and_items(std::function<bool(const value_type&)> predicate) const {
		return find_indexed_value_pairs(predicate);
	}

	/**
	 * @brief 条件に一致する最初の行のグローバルインデックスを検索します。
	 * @param[in] predicate 検索条件となる述語関数。
	 * @return 条件に一致した最初の行のグローバルインデックス。
	 *
	 * ```cpp
	 * // 例: ageフィールドが30以上の行のインデックスを取得
	 * auto index_opt = find_first_index([](const _Tp& value) {
	 * 	return value.age >= 30;
	 * });
	 * ```
	 */
	std::optional<size_type> find_first_index(std::function<bool(const value_type&)> predicate) const {
		for (size_type i = 0; i < m_master_data.size(); i++) {
			if (predicate(m_master_data[i])) {
				return i;
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief 条件に一致する最初の行データを検索します。
	 * @param[in] predicate 検索条件となる述語関数。
	 * @return 条件に一致した最初の行データ。
	 *
	 * ```cpp
	 * // 例: ageフィールドが30以上の行のインデックスを取得
	 * auto value_opt = find_first_value([](const _Tp& value) {
	 * 	return value.age >= 30;
	 * });
	 * ```
	 */
	std::optional<value_type> find_first_value(std::function<bool(const value_type&)> predicate) const {
		for (const auto& value : m_master_data) {
			if (predicate(value)) {
				return value;
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief 条件に一致する最初の行データを検索します。（旧・互換版）
	 * @deprecated Use 'find_first_value' instead.
	 */
	[[deprecated("Use 'find_first_value' instead.")]]
	std::optional<value_type> find_first_item(std::function<bool(const value_type&)> predicate) const {
		return find_first_value(predicate);
	}

	/**
	 * @brief 条件に一致する最初の行のグローバルインデックスと行データのペアを検索します。
	 * @param[in] predicate 検索条件となる述語関数。
	 * @return 条件に一致した最初のグローバルインデックスと行データのペア。
	 *
	 * ```cpp
	 * // 例: ageフィールドが30以上の行のインデックスを取得
	 * auto indexed_value_opt = find_first_indexed_value([](const _Tp& value) {
	 * 	return value.age >= 30;
	 * });
	 * ```
	 */
	std::optional<indexed_value_pair> find_first_indexed_value(std::function<bool(const value_type&)> predicate) const {
		for (size_type i = 0; i < m_master_data.size(); i++) {
			if (predicate(m_master_data[i])) {
				return std::make_pair(i, m_master_data[i]);
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief 条件に一致する最初の行のインデックスと行データのペアを検索します。（旧・互換版）
	 * @deprecated Use 'find_first_indexed_value' instead.
	 */
	[[deprecated("Use 'find_first_indexed_value' instead.")]]
	std::optional<indexed_value_pair> find_first_index_and_item(std::function<bool(const value_type&)> predicate) const {
		return find_first_indexed_value(predicate);
	}

	/**
	 * @brief 現在表示されているページで選択されている行のグローバルインデックスを取得します。
	 * @warning [使用上の注意]
	 * この関数は、呼び出された瞬間の選択状態を問い合わせます。
	 * get_user_selection_callback() で登録したコールバック関数内で
	 * 「どの行が選択されたか」を知るためにこの関数を呼び出すのは
	 * 非推奨です (タイミング問題が発生する可能性があります)。
	 * コールバック内では、引数として渡されるインデックスを使用してください。
	 * @note
	 * 「確定」ボタンのクリック時など、選択変更イベント以外の場所から
	 * 現在の選択状態を問い合わせる目的で使うのは安全です。
	 * @return 選択されていればグローバルインデックス、なければ空のoptional。
	 */
	std::optional<size_type> get_selected_index() {
		size_type total_rows_per_page = m_rows_per_view * m_page_models.size();
		size_type page_start_index = m_current_page_index * total_rows_per_page;

		// 管理している全てのTreeViewをチェック
		for (size_type i = 0; i < m_page_models.size(); i++) {
			auto local_index_opt = m_page_models[i]->get_selected_index();
			if (local_index_opt.has_value()) {
				int local_index = local_index_opt.value();
				// TreeView内での開始インデックスを計算
				size_type view_start_index = i * m_rows_per_view;
				// グローバルインデックスを計算して返す
				return page_start_index + view_start_index + local_index;
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief 現在選択されている行のデータをマスターデータから取得します。
	 * @warning [使用上の注意]
	 * この関数は、呼び出された瞬間の選択状態を問い合わせます。
	 * get_user_selection_callback() で登録したコールバック関数内で
	 * 「どの行が選択されたか」を知るためにこの関数を呼び出すのは
	 * 非推奨です (タイミング問題が発生する可能性があります)。
	 * コールバック内では、引数として渡されるインデックスを使用してください。
	 * @note
	 * 「確定」ボタンのクリック時など、選択変更イベント以外の場所から
	 * 現在の選択状態を問い合わせる目的で使うのは安全です。
	 * @return 選択されていれば行データ、なければ空のoptional。
	 */
	std::optional<value_type> get_selected_value() {
		auto index_opt = get_selected_index();
		if (index_opt.has_value()) {
			return this->at(index_opt.value());
		}
		return std::nullopt;
	}

	/**
	 * @brief 現在選択されている行のデータをマスターデータから取得します。（旧・互換版）
	 * @deprecated Use 'get_selected_value' instead.
	 */
	[[deprecated("Use 'get_selected_value' instead.")]]
	std::optional<value_type> get_selected_item() {
		return get_selected_value();
	}

	/**
	 * @brief 現在選択されている行のグローバルインデックスと行データのペアを取得します。
	 * @warning [使用上の注意]
	 * この関数は、呼び出された瞬間の選択状態を問い合わせます。
	 * get_user_selection_callback() で登録したコールバック関数内で
	 * 「どの行が選択されたか」を知るためにこの関数を呼び出すのは
	 * 非推奨です (タイミング問題が発生する可能性があります)。
	 * コールバック内では、引数として渡されるインデックスを使用してください。
	 * @note
	 * 「確定」ボタンのクリック時など、選択変更イベント以外の場所から
	 * 現在の選択状態を問い合わせる目的で使うのは安全です。
	 * @return 選択されていればグローバルインデックスと行データのペア、なければ空のoptional。
	 */
	std::optional<indexed_value_pair> get_selected_indexed_value() {
		auto index_opt = get_selected_index();
		if (index_opt.has_value()) {
			return std::make_pair(index_opt.value(), this->at(index_opt.value()));
		}
		return std::nullopt;
	}

	/**
	 * @brief 現在選択されている行のグローバルインデックスと行データのペアを取得します。（旧・互換版）
	 * @deprecated Use 'get_selected_indexed_value' instead.
	 */
	[[deprecated("Use 'get_selected_indexed_value' instead.")]]
	std::optional<indexed_value_pair> get_selected_index_and_item() {
		return get_selected_indexed_value();
	}
};
} // namespace sgex
