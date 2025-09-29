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
 * @class Dialog
 * @brief sg::Dialogを拡張し、"show"シグナルハンドラを追加したクラス。
 * @details "show"シグナルは、ダイアログが表示された直後に発生します。
 */
class Dialog : public sg::Dialog {
public:
	// 基底クラスのコンストラクタを継承
	using sg::Dialog::Dialog;

	// "show"シグナルハンドラを定義するマクロ
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

	// "visible"プロパティのゲッターとセッター(set_visible/get_visible)を定義する
	DEFINE_PROPERTY_ACCESSORS(Widget, visible, bool)
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
};

/**
 * @class Button
 * @brief sg::Buttonを拡張し、表示/非表示を切り替える機能を追加したクラス。
 */
class Button : public sg::Button {
public:
	using sg::Button::Button; // 基底クラスのコンストラクタを継承

	// "visible"プロパティのゲッターとセッター(set_visible/get_visible)を定義する
	DEFINE_PROPERTY_ACCESSORS(Widget, visible, bool)
};

/**
 * @class Entry
 * @brief sg::Entryを拡張し、"activate"シグナルハンドラを追加したクラス。
 * @details "activate"シグナルは、Entry内でEnterキーが押された時に発生します。
 */
class Entry : public sg::Entry {
public:
	// 基底クラスのコンストラクタを継承
	using sg::Entry::Entry;

	// "activate"シグナルハンドラを定義するマクロ
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

	// シグナル (状態が変更されたときに発生)
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

	// シグナル
	DEFINE_SIGNAL_HANDLER("changed", changed, ComboBox, GtkComboBox* self)
};

/**
 * @class Label
 * @brief sg::Labelを拡張し、表示/非表示を切り替える機能を追加したクラス。
 */
class Label : public sg::Label {
public:
	using sg::Label::Label; // 基底クラスのコンストラクタを継承

	// "visible"プロパティのゲッターとセッター(set_visible/get_visible)を定義する
	DEFINE_PROPERTY_ACCESSORS(Widget, visible, bool)
};

/**
 * @class TreeView
 * @brief sg::TreeViewを拡張し、有効/無効と表示/非表示を切り替える機能を追加したクラス。
 */
class TreeView : public sg::TreeView {
public:
	using sg::TreeView::TreeView; // 基底クラスのコンストラクタを継承

	// "visible"プロパティのゲッターとセッター(set_visible/get_visible)を定義する
	DEFINE_PROPERTY_ACCESSORS(Widget, sensitive, bool)
	DEFINE_PROPERTY_ACCESSORS(Widget, visible, bool)
};

} // namespace tool_io