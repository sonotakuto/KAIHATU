/**
 * @file gmain_utils.hpp
 * @brief GLibのメインループ関連のユーティリティ関数の宣言
 */
#pragma once
#include <functional>
#include <utility>
#include <glib.h>
#include "common.hpp"

/**
 * @namespace gutils
 * @brief GLib関連のユーティリティをまとめた名前空間。
 */
namespace gutils {

/**
 * @namespace gutils::detail
 * @brief gmain_utils.hpp の実装の詳細をまとめた名前空間。
 * この名前空間の関数は、直接使わないで下さい。
 */
namespace detail
{

	/**
	 * @brief GLibのメインループで関数オブジェクトを実行するためのディスパッチャー。
	 * @details
	 * C言語形式のコールバックである g_idle_add 等と C++ の関数オブジェクトを繋ぐための汎用的な糊（グルー）コード。
	 * 利用者は、実行したい処理を std::function オブジェクトとしてヒープに作成し、
	 * そのポインタをこの関数に渡すことで、GLibのメインループでの安全な実行を予約します。
	 * @param[in] func std::function<bool()>* にキャストされるポインタ。戻り値で false を返すと1回限りの実行、true を返すと継続的に実行されます。
	 * @return func が false を返すと G_SOURCE_REMOVE、true を返すと G_SOURCE_CONTINUE。
	 */
	gboolean dispatch_to_gmain(gpointer func);

	/**
	 * @brief イベントソース破棄時に、確保した関数オブジェクトのメモリを安全に解放するコールバック。
	 * @param[in] data dispatch_to_gmain に渡された std::function オブジェクトのポインタ。
	 */
	void destroy_gmain_data(gpointer data);

} // namespace detail

/**
 * @brief GLibのメインループで関数オブジェクトをタイマー実行する。
 * @warning 【重要】非同期で遅延実行されるため、呼び出し元のローカル変数を参照キャプチャ（[&]）しないでください。
 * @warning 必ず値キャプチャ（[=]）を使用するか、寿命が保証されているポインタを使用してください。
 * @tparam F 関数オブジェクトの型。bool() の呼び出し可能な型である必要があります。
 * @param[in] interval_ms タイマーの間隔（ミリ秒）。
 * @param[in] func 実行したい関数オブジェクト。戻り値で false を返すと1回限りの実行、true を返すと継続的に実行されます。
 * @return イベントソースのID。
 */
template <typename F>
guint timeout_add(guint interval_ms, F&& func) {
	return g_timeout_add_full(G_PRIORITY_DEFAULT, interval_ms, detail::dispatch_to_gmain, new std::function<bool()>(std::forward<F>(func)), detail::destroy_gmain_data);
}

/**
 * @brief GLibのメインループで関数オブジェクトを1回だけタイマー実行する。
 * @warning 【重要】非同期で遅延実行されるため、呼び出し元のローカル変数を参照キャプチャ（[&]）しないでください。
 * @warning 必ず値キャプチャ（[=]）を使用するか、寿命が保証されているポインタを使用してください。
 * @tparam F 関数オブジェクトの型。
 * @param[in] interval_ms タイマーの間隔（ミリ秒）。
 * @param[in] func 実行したい関数オブジェクト。戻り値を返したとしても使用されません。
 * @return イベントソースのID。
 */
template <typename F>
guint timeout_add_once(guint interval_ms, F&& func) {
	// ユーザーの void() 関数をラップし、強制的に false (G_SOURCE_REMOVE) を返す
	// bool() 関数として timeout_add に渡す。
	return timeout_add(interval_ms, [f = std::forward<F>(func)]() mutable -> bool {
		f();
		return false; // 1回で終了させる
	});
}

/**
 * @brief GLibのメインループで関数オブジェクトを秒単位のタイマー実行する。
 * @warning 【重要】非同期で遅延実行されるため、呼び出し元のローカル変数を参照キャプチャ（[&]）しないでください。
 * @warning 必ず値キャプチャ（[=]）を使用するか、寿命が保証されているポインタを使用してください。
 * @tparam F 関数オブジェクトの型。bool() の呼び出し可能な型である必要があります。
 * @param[in] interval_sec タイマーの間隔（秒）。
 * @param[in] func 実行したい関数オブジェクト。戻り値で false を返すと1回限りの実行、true を返すと継続的に実行されます。
 * @return イベントソースのID。
 */
template <typename F>
guint timeout_add_seconds(guint interval_sec, F&& func) {
	return g_timeout_add_seconds_full(G_PRIORITY_DEFAULT, interval_sec, detail::dispatch_to_gmain, new std::function<bool()>(std::forward<F>(func)), detail::destroy_gmain_data);
}

/**
 * @brief GLibのメインループで関数オブジェクトを1回だけ秒単位のタイマー実行する。
 * @warning 【重要】非同期で遅延実行されるため、呼び出し元のローカル変数を参照キャプチャ（[&]）しないでください。
 * @warning 必ず値キャプチャ（[=]）を使用するか、寿命が保証されているポインタを使用してください。
 * @tparam F 関数オブジェクトの型。
 * @param[in] interval_sec タイマーの間隔（秒）。
 * @param[in] func 実行したい関数オブジェクト。戻り値を返したとしても使用されません。
 * @return イベントソースのID。
 */
template <typename F>
guint timeout_add_seconds_once(guint interval_sec, F&& func) {
	// ユーザーの void() 関数をラップし、強制的に false (G_SOURCE_REMOVE) を返す
	// bool() 関数として timeout_add に渡す。
	return timeout_add_seconds(interval_sec, [f = std::forward<F>(func)]() mutable -> bool {
		f();
		return false; // 1回で終了させる
	});
}

/**
 * @brief GLibのメインループで関数オブジェクトを非同期実行する。
 * @warning 【重要】非同期で遅延実行されるため、呼び出し元のローカル変数を参照キャプチャ（[&]）しないでください。
 * @warning 必ず値キャプチャ（[=]）を使用するか、寿命が保証されているポインタを使用してください。
 * @tparam F 関数オブジェクトの型。bool() の呼び出し可能な型である必要があります。
 * @param[in] func 実行したい関数オブジェクト。戻り値で false を返すと1回限りの実行、true を返すと継続的に実行されます。
 * @return イベントソースのID。
 */
template <typename F>
guint idle_add(F&& func) {
	return g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, detail::dispatch_to_gmain, new std::function<bool()>(std::forward<F>(func)), detail::destroy_gmain_data);
}

/**
 * @brief GLibのメインループで関数オブジェクトを1回だけ非同期実行する。
 * @warning 【重要】非同期で遅延実行されるため、呼び出し元のローカル変数を参照キャプチャ（[&]）しないでください。
 * @warning 必ず値キャプチャ（[=]）を使用するか、寿命が保証されているポインタを使用してください。
 * @tparam F 関数オブジェクトの型。
 * @param[in] func 実行したい関数オブジェクト。戻り値を返したとしても使用されません。
 * @return イベントソースのID。
 */
template <typename F>
guint idle_add_once(F&& func) {
	// ユーザーの void() 関数をラップし、強制的に false (G_SOURCE_REMOVE) を返す
	// bool() 関数として idle_add に渡す。
	return idle_add([f = std::forward<F>(func)]() mutable -> bool {
		f();
		return false; // 1回で終了させる
	});
}

} // namespace gutils
