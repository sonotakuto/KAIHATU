/**
 * @file gio_utils.cpp
 * @brief GIO（D-Busやプロセス起動など）に関連するユーティリティ関数の実装
 */
#include "gio_utils.hpp"
#include "utils/string_utils.hpp"
#include <gio/gio.h>
#include <vector>
#include <iostream> // またはプロジェクトのログ機能

/**
 * @namespace gutils
 * @brief GLib関連のユーティリティをまとめた名前空間。
 */
namespace gutils {

/**
 * @namespace gio
 * @brief GIO関連のユーティリティをまとめた名前空間。
 */
namespace gio {

void dbus_call_async(
	const char* bus_name,
	const char* object_path,
	const char* interface_name,
	const char* method_name,
	std::string* error_message
) {
	GError* error = nullptr;
	GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);

	if (!connection) {
		if (error_message) {
			*error_message = std::string("[GIO] Failed to connect to dbus: ") + std::string((error) ? (error->message) : ("unknown"));
		}
		if (error) {
			g_error_free(error);
		}
		return;
	}

	g_dbus_connection_call(
		connection, bus_name, object_path, interface_name, method_name,
		nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr
	);

	g_object_unref(connection);
}

bool dbus_service_exists(const char* bus_name) {
	GError* error = nullptr;
	GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);

	if (!connection) {
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
		if (error) g_error_free(error);
	}

	g_object_unref(connection);
	return exists;
}

void spawn_async(const std::vector<std::string>& args, std::string* error_message) {
	if (args.empty()) {
		return;
	}

	// vector<string> を char* 配列に変換
	std::vector<char*> argv;
	for (const auto& arg : args) {
		argv.push_back(const_cast<char*>(arg.c_str()));
	}
	argv.push_back(nullptr); // 番兵

	GError* error = nullptr;
	if (!g_spawn_async(nullptr, argv.data(), nullptr,
					   (GSpawnFlags)(G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL),
					   nullptr, nullptr, nullptr, &error)) {
		fprintf(stderr, "[GIO] Failed to spawn %s: %s\n", args[0].c_str(), error ? error->message : "unknown");
		g_error_free(error);
	}
}

void spawn_async(const std::vector<char*>& args, std::string* error_message) {
	if (args.empty()) {
		return;
	}

	args.push_back(nullptr); // 番兵

	GError* error = nullptr;
	if (!g_spawn_async(nullptr, argv.data(), nullptr,
					   (GSpawnFlags)(G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL),
					   nullptr, nullptr, nullptr, &error)) {
		if (error_message) {
			*error_message = std::string("[GIO] Failed to spawn ") + args[0] + std::string(": ") + std::string((error) ? (error->message) : ("unknown"));
		}
		fprintf(stderr, "[GIO] Failed to spawn %s: %s\n", args[0].c_str(), error ? error->message : "unknown");
		g_error_free(error);
	}
}

} // namespace gio
} // namespace gutils
