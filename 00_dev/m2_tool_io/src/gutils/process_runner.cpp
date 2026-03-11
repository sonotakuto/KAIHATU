/**
 * @file process_runner.cpp
 * @brief ProcessRunnerクラスの実装。
 */
#include "process_runner.hpp"
#include "utils/string_utils.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/**
 * @namespace gutils
 * @brief GLib関連のユーティリティをまとめた名前空間。
 */
namespace gutils {

/**
 * @brief コンストラクタ。
 * 初期化はメンバ初期化子で行われるため、特に追加の処理はありません。
 */
ProcessRunner::ProcessRunner() = default;

/**
 * @brief デストラクタ。
 * オブジェクト破棄時にプロセスが残っていた場合、強制的に停止処理を行います。
 */
ProcessRunner::~ProcessRunner() {
	kill();
}

bool ProcessRunner::start(const std::string& command, std::function<void(int)> on_exit) {
	// 二重起動防止
	if (pid_ != 0) {
		error_message_ = "[ProcessRunner] Process is already running.";
		return false;
	}

	on_exit_ = on_exit;

	gchar **argv = nullptr;
	GError *error = nullptr;

	// コマンド文字列を引数リスト(argv)に解析します。
	// 例: "ls -l" -> {"ls", "-l", NULL}
	// 注意: シェルのパイプやリダイレクトはここでは解釈されません。
	if (!g_shell_parse_argv(command.c_str(), nullptr, &argv, &error)) {
		utils::assign_concat(error_message_, "[ProcessRunner] Parse error: ", ((error && error->message) ? (error->message) : ("Unknown parse error")));
		g_error_free(error);
		return false;
	}

	// 非同期でプロセスを生成します。
	// G_SPAWN_DO_NOT_REAP_CHILD:
	//   重要フラグ。GLibに自動的にwait(回収)させないようにします。
	//   これがないと、後述の g_child_watch_add が正しく動作しません。
	// G_SPAWN_SEARCH_PATH:
	//   PATH環境変数から実行ファイルを探します。
	gboolean success = g_spawn_async(
		nullptr,	// 作業ディレクトリ (nullptr = 現在のディレクトリを継承)
		argv,		// 引数リスト
		nullptr,	// 環境変数 (nullptr = 現在の環境を継承)
		static_cast<GSpawnFlags>(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH),
		nullptr,	// 子プロセスセットアップ関数
		nullptr,	// ユーザーデータ
		&pid_,		// 生成されたPIDを受け取る
		&error
	);

	// argvはもう不要なので解放します
	g_strfreev(argv);

	if (!success) {
		utils::assign_concat(error_message_, "[ProcessRunner] Spawn error: ", ((error && error->message) ? (error->message) : ("Unknown spawn error")));
		g_error_free(error);
		pid_ = 0;	// 状態をリセット
		return false;
	}

	// プロセスの終了を監視します。
	// プロセス終了時、メインループのコンテキストで on_child_exited_static が呼ばれます。
	watch_id_ = g_child_watch_add(pid_, on_child_exited_static, this);

	return true;
}

void ProcessRunner::stop() noexcept {
	if (pid_ > 0) {
		// 終了要請のみを行う
		::kill(pid_, SIGTERM);

		// 【注意点】
		// プロセスは非同期に終了するため、ここでは g_source_remove や g_spawn_close_pid を行いません。
		// プロセスがSIGTERMを受け取って終了したタイミングで、GLibのメインループが
		// 自動的に on_child_exited_static を呼び出し、そこで適切にリソースが回収されます。
	}
}

void ProcessRunner::kill() noexcept {
	// 1. GLibによるプロセスの終了監視をキャンセル（コールバックの発火を防ぐ）
	if (watch_id_ > 0) {
		g_source_remove(watch_id_);
		watch_id_ = 0;
	}

	// 2. プロセスを強制終了させ、即座に回収する
	if (pid_ > 0) {
		::kill(pid_, SIGKILL);

		// ゾンビ化を防ぐため、安全なダミーコールバックを再登録してGLibに回収させる
		// キャプチャが空（[]）なので、C言語の関数ポインタに自動変換されて渡される
		waitpid(pid_, nullptr, 0);
		g_child_watch_add(pid_, [](GPid pid, gint status, gpointer data) {
			g_spawn_close_pid(pid);
		}, nullptr);

		pid_ = 0;
	}
}

bool ProcessRunner::is_running() const noexcept {
	return pid_ != 0;
}

std::string ProcessRunner::get_error_message() const {
	return error_message_;
}

void ProcessRunner::on_child_exited_static(GPid pid, gint status, gpointer user_data) {
	// user_data をクラスのインスタンスに戻してメンバ関数を呼びます
	auto* self = static_cast<ProcessRunner*>(user_data);
	if (self) {
		self->handle_child_exited(status);
	}
}

void ProcessRunner::handle_child_exited(gint status) {
	// プロセスは既に終了しているので kill は不要ですが、
	// PIDハンドルのクローズは必要です。
	g_spawn_close_pid(pid_);

	// 状態をリセット
	pid_ = 0;
	watch_id_ = 0; // watchイベントは発火時に自動削除されるため、g_source_removeは不要

	// ユーザー定義のコールバックがあれば実行
	if (on_exit_) {
		on_exit_(status);
	}
}

} // namespace gutils
