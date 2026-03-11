/**
 * @file gio_utils.cpp
 * @brief GIO（D-Busやプロセス起動など）に関連するユーティリティ関数の実装
 */
#include "gio_utils.hpp"
#include "utils/string_utils.hpp"
#include <gio/gio.h>
#include <vector>

/**
 * @namespace gutils
 * @brief GLib関連のユーティリティをまとめた名前空間。
 */
namespace gutils {

// 内部で使用するヘルパー
namespace {
// 【共通実装】
// 既に char** (argv) が用意されている状態で g_spawn_async を呼ぶヘルパー関数。
// ※ argv_ptr は nullptr で終わる配列であること。
bool spawn_async_impl(char** argv_ptr, std::string* error_message) {
	GError* error = nullptr;

	// argv_ptr[0] があるかチェック（ログ用）
	const char* cmd_name = (argv_ptr && argv_ptr[0]) ? argv_ptr[0] : "unknown";

	if (!g_spawn_async(
		nullptr,
		argv_ptr, // 既に char** なのでそのまま渡す
		nullptr,
		static_cast<GSpawnFlags>(G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL),
		nullptr, nullptr, nullptr, &error
	)) {
		if (error_message) {
			utils::assign_concat(*error_message, "[GIO] Failed to spawn ", cmd_name, ": ", ((error && error->message) ? (error->message) : ("unknown")));
		}
		if (error) g_error_free(error);
		return false;
	}
	return true;
}

void dbus_call_ready_callback(GObject* source_object, GAsyncResult* res, gpointer /* user_data */) {
	GError* error = nullptr;
	// 呼び出し結果を取得
	GVariant* result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source_object), res, &error);

	if (result) {
		// 成功時は結果を破棄
		g_variant_unref(result);
	} else {
		// 失敗時は警告ログを出力
		if (error) {
			LogManager::logError(utils::concat("[GIO] dbus_call_async delayed execution failed: ", error->message));
			g_error_free(error);
		} else {
			LogManager::logError("[GIO] dbus_call_async delayed execution failed with an unknown error.");
		}
	}
}
} // namespace

/**
 * @namespace gutils::gio
 * @brief GIO関連のユーティリティをまとめた名前空間。
 */
namespace gio {

bool dbus_call_async(
	const char* bus_name,
	const char* object_path,
	const char* interface_name,
	const char* method_name,
	std::string* error_message
) {
	GError* error = nullptr;

	if (bus_name == nullptr || object_path == nullptr || interface_name == nullptr || method_name == nullptr) {
		if (error_message) {
			utils::assign_concat(*error_message, "[GIO] Invalid argument: bus_name, object_path, interface_name, and method_name must not be null.");
		}
		return false;
	}

	GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);

	if (!connection) {
		if (error_message) {
			utils::assign_concat(*error_message, "[GIO] Failed to connect to dbus: ", ((error && error->message) ? (error->message) : ("unknown")));
		}
		if (error) {
			g_error_free(error);
		}
		return false;
	}

	g_dbus_connection_call(
		connection, bus_name, object_path, interface_name, method_name,
		nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
		dbus_call_ready_callback, nullptr
	);

	g_object_unref(connection);

	return true;
}

bool dbus_service_exists(const char* bus_name, std::string* error_message) {
	GError* error = nullptr;

	if (bus_name == nullptr) {
		if (error_message) {
			utils::assign_concat(*error_message, "[GIO] Invalid argument: bus_name must not be null.");
		}
		return false;
	}

	GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);

	if (!connection) {
		if (error_message) {
			utils::assign_concat(*error_message, "[GIO] Failed to connect to dbus: ", ((error && error->message) ? (error->message) : ("unknown")));
		}
		if (error) {
			g_error_free(error);
		}
		return false;
	}

	GVariant* result = g_dbus_connection_call_sync(
		connection,
		"org.freedesktop.DBus",
		"/org/freedesktop/DBus",
		"org.freedesktop.DBus",
		"NameHasOwner",
		g_variant_new("(s)", bus_name),
		G_VARIANT_TYPE("(b)"),
		G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error
	);

	bool exists = false;
	if (result) {
		g_variant_get(result, "(b)", &exists);
		g_variant_unref(result);
	} else {
		if (error_message) {
			utils::assign_concat(*error_message, "[GIO] Failed to check service existence: ", ((error && error->message) ? (error->message) : ("unknown")));
		}
		if (error) {
			g_error_free(error);
		}
	}

	g_object_unref(connection);
	return exists;
}

bool spawn_async(const std::vector<std::string>& argv, std::string* error_message) {
	if (argv.empty()) {
		if (error_message) {
			utils::assign_concat(*error_message, "[GIO] Invalid argument: argv must not be empty.");
		}
		return false;
	}

	// Constな文字列リストなので、安全のために変更可能なコピー(storage)を作成
	std::vector<std::string> storage = argv;

	// ポインタ配列を作成
	std::vector<char*> argv_exec;
	argv_exec.reserve(storage.size() + 1);

	for (auto& s : storage) {
		argv_exec.push_back(s.data());	// C++17: 非const string::data() は char*
	}
	argv_exec.push_back(nullptr);	// 番兵

	return spawn_async_impl(argv_exec.data(), error_message);
}

bool spawn_async(std::initializer_list<const char*> argv, std::string* error_message) {
	if (argv.size() == 0) {
		if (error_message) {
			utils::assign_concat(*error_message, "[GIO] Invalid argument: argv must not be empty.");
		}
		return false;
	}

	// 文字列の実体をコピー作成
	std::vector<std::string> storage;
	storage.reserve(argv.size());
	for (const char* arg : argv) {
		if (arg != nullptr) { // nullptr を安全に除外
			storage.emplace_back(arg);
		}
	}

	// ポインタ配列を作成
	std::vector<char*> argv_exec;
	argv_exec.reserve(storage.size() + 1);

	// 構築とポインタ取得
	for (auto& s : storage) {
		argv_exec.push_back(s.data());	// C++17: 非const string::data() は char*
	}
	argv_exec.push_back(nullptr);	// 番兵

	return spawn_async_impl(argv_exec.data(), error_message);}

} // namespace gio
} // namespace gutils
