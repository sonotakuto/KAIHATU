#include "./process_runner.hpp"

// 実装に必要なヘッダはここに書く
// これにより、利用者は <sys/types.h> などを意識しなくて済む
#include <sys/types.h>
#include <signal.h>
#include <iostream>

ProcessRunner::ProcessRunner() = default;

ProcessRunner::~ProcessRunner() {
	stop();
}

bool ProcessRunner::start(const std::string& command, ExitCallback on_exit) {
	if (pid_ != 0) {
		std::cerr << "Process is already running." << std::endl;
		return false;
	}

	on_exit_ = on_exit;

	gchar **argv = nullptr;
	GError *error = nullptr;

	if (!g_shell_parse_argv(command.c_str(), nullptr, &argv, &error)) {
		std::cerr << "Parse error: " << error->message << std::endl;
		g_error_free(error);
		return false;
	}

	gboolean success = g_spawn_async(
		nullptr, argv, nullptr,
		static_cast<GSpawnFlags>(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH),
		nullptr, nullptr, &pid_, &error
	);

	g_strfreev(argv);

	if (!success) {
		std::cerr << "Spawn error: " << error->message << std::endl;
		g_error_free(error);
		return false;
	}

	watch_id_ = g_child_watch_add(pid_, on_child_exited_static, this);
	return true;
}

void ProcessRunner::stop() {
	if (watch_id_ > 0) {
		g_source_remove(watch_id_);
		watch_id_ = 0;
	}

	if (pid_ > 0) {
		kill(pid_, SIGTERM);
		g_spawn_close_pid(pid_);
		pid_ = 0;
	}
}

bool ProcessRunner::is_running() const {
	return pid_ != 0;
}

void ProcessRunner::on_child_exited_static(GPid pid, gint status, gpointer user_data) {
	auto* self = static_cast<ProcessRunner*>(user_data);
	if (self) {
		self->handle_child_exited(status);
	}
}

void ProcessRunner::handle_child_exited(gint status) {
	g_spawn_close_pid(pid_);
	pid_ = 0;
	watch_id_ = 0;

	if (on_exit_) {
		on_exit_(status);
	}
}
