/**
 * @file keyboard_utils.cpp
 * @brief onboardキーボードの起動・表示・非表示を制御するユーティリティ関数群の実装。
 */
#include "gutils/gio_utils.hpp"
#include "keyboard_utils.hpp"

namespace onboard {

// Onboard固有の定数
static const char* BUS_NAME = "org.onboard.Onboard";
static const char* OBJECT_PATH = "/org/onboard/Onboard/Keyboard";
static const char* INTERFACE_NAME = "org.onboard.Onboard.Keyboard";

void ensure_running() {
	// サービスが既にいれば何もしない
	if (gutils::gio::dbus_service_exists(BUS_NAME)) {
		return;
	}
	// なければ起動 (コマンド名のみ指定)
	gutils::gio::spawn_async({"onboard"});
}

void show() {
	ensure_running();
	gutils::gio::dbus_call_async(BUS_NAME, OBJECT_PATH, INTERFACE_NAME, "Show");
}

void hide() {
	// 隠すだけなら起動確認不要
	gutils::gio::dbus_call_async(BUS_NAME, OBJECT_PATH, INTERFACE_NAME, "Hide");
}

} // namespace onboard
