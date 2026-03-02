/**
 * @file gio_utils.hpp
 * @brief GIO（D-Busやプロセス起動など）に関連するユーティリティ関数の定義
 */
#pragma once
#include <initializer_list>
#include <string>
#include <vector>

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

/**
 * @brief D-Bus上の指定されたメソッドを非同期(Fire-and-forget)で呼び出します。
 * @param[in] bus_name バス名（サービス名）
 * @param[in] object_path オブジェクトパス
 * @param[in] interface_name インターフェース名
 * @param[in] method_name メソッド名
 * @param[out] error_message (オプション・デフォルト nullptr) エラーメッセージの格納先ポインタ
 * @return 呼び出しに成功した場合はtrue、失敗した場合はfalse
 */
bool dbus_call_async(
	const char* bus_name,
	const char* object_path,
	const char* interface_name,
	const char* method_name,
	std::string* error_message = nullptr
);

/**
 * @brief 指定されたBus Name(サービス名)がD-Bus上に存在するか確認します。
 * @param[in] bus_name バス名（サービス名）
 * @param[out] error_message (オプション・デフォルト nullptr) エラーメッセージの格納先ポインタ
 * @return 存在する場合はtrue、存在しない場合はfalse
 */
bool dbus_service_exists(const char* bus_name, std::string* error_message = nullptr);

/**
 * @brief 外部コマンドを非同期で起動します（シェルを経由しません）。
 * @param[in] argv コマンドと引数のリスト (例: {"ping", "google.com"} )
 * @param[out] error_message (オプション・デフォルト nullptr) エラーメッセージの格納先ポインタ
 * @return コマンドの起動に成功した場合はtrue、失敗した場合はfalse
 */
bool spawn_async(const std::vector<std::string>& argv, std::string* error_message = nullptr);

/**
 * @brief 外部コマンドを非同期で起動します（シェルを経由しません）。
 * @param[in] argv コマンドと引数のリスト (例: {"ping", "google.com"} )
 * @param[out] error_message (オプション・デフォルト nullptr) エラーメッセージの格納先ポインタ
 * @return コマンドの起動に成功した場合はtrue、失敗した場合はfalse
 */
bool spawn_async(std::initializer_list<const char*> argv, std::string* error_message = nullptr);

} // namespace gio
} // namespace gutils
