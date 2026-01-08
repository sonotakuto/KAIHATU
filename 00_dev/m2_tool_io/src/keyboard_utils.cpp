/**
 * @file keyboard_utils.cpp
 * @brief onboardキーボードの起動・表示・非表示を制御するユーティリティ関数群の実装。
 */
#include <cstdlib>
#include "./includes.hpp"
#include "./keyboard_utils.hpp"

namespace onboard {

/**
 * @brief コマンドを非同期で実行するヘルパー関数。
 * @param cmd 実行するコマンド文字列。
 */
static void run_cmd(const char* cmd) {
	GError* error = nullptr;
	// g_spawn_command_line_async はコマンドを実行してすぐに戻ってくるため、GUIがフリーズしません
	if (!g_spawn_command_line_async(fmt::format("sh -c \"{}\"", cmd).c_str(), &error)) {
		DEV_PRINT(stderr, "[KeyboardUtils] Error running command: {} -> {}\n", cmd, ((error) ? (error->message) : ("unknown")));
		if (error) { g_error_free(error); }
	}
}

void ensure_running() {
	// pgrep でプロセス存在確認を行い、なければ起動する
	// ※ system() はブロッキングですが、pgrep は一瞬で終わるので許容範囲です
	int status = std::system("pgrep -x onboard > /dev/null");

	// 戻り値 0 はプロセス発見、それ以外は未発見
	if (status != 0) {
		// 起動していなければ起動（nohup でバックグラウンド化）
		run_cmd("nohup onboard > /dev/null 2>&1 &");
	}
}

void show() {
	ensure_running();
	// D-Bus経由で「表示(Show)」シグナルを送る
	run_cmd("dbus-send --type=method_call --dest=org.onboard.Onboard "
			"/org/onboard/Onboard/Keyboard org.onboard.Onboard.Keyboard.Show");
}

void hide() {
	// D-Bus経由で「非表示(Hide)」シグナルを送る
	run_cmd("dbus-send --type=method_call --dest=org.onboard.Onboard "
			"/org/onboard/Onboard/Keyboard org.onboard.Onboard.Keyboard.Hide");
}

}
