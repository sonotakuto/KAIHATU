/**
 * @file gmain_utils.hpp
 * @brief GLibのメインループ関連のユーティリティ関数の宣言
 */
#pragma once
#include <functional>
#include <glib.h>

/**
 * @namespace gutils
 * @brief GLib関連のユーティリティをまとめた名前空間。
 */
namespace gutils {

/**
 * @namespace detail
 * @brief gmain_utils.hpp の実装の詳細をまとめた名前空間。
 * この名前空間の関数は、直接使わないで下さい。
 */
namespace detail
{

	/**
	 * @brief GLibのメインループで関数オブジェクトを実行するためのディスパッチャー。
	 * @details
	 * C言語形式のコールバックである g_idle_add と C++ の関数オブジェクトを繋ぐための汎用的な糊（グルー）コード。
	 * 利用者は、実行したい処理を std::function オブジェクトとしてヒープに作成し、
	 * そのポインタをこの関数に渡すことで、GLibのメインループでの安全な実行を予約します。
	 * @param[in] func std::function<bool()>* にキャストされるポインタ。戻り値で false を返すと1回限りの実行、true を返すと継続的に実行されます。
	 * @return func が false を返すと G_SOURCE_REMOVE、true を返すと G_SOURCE_CONTINUE。
	 */
	gboolean dispatch_to_gmain(gpointer func);

#if GLIB_CHECK_VERSION(2, 74, 0)

	/**
	 * @brief GLibのメインループで関数オブジェクトを1回だけ実行するためのディスパッチャー。
	 * @details
	 * C言語形式のコールバックである g_idle_add_once と C++ の関数オブジェクトを繋ぐための汎用的な糊（グルー）コード。
	 * 利用者は、実行したい処理を std::function オブジェクトとしてヒープに作成し、
	 * そのポインタをこの関数に渡すことで、GLibのメインループでの安全な実行を予約します。
	 * @param[in] func std::function<void()>* にキャストされるポインタ。
	 */
	void dispatch_to_gmain_once(gpointer func);

#endif

} // namespace detail

/**
 * @brief GLibのメインループで関数オブジェクトを非同期実行する。
 * @tparam F 関数オブジェクトの型。bool() の呼び出し可能な型である必要があります。
 * @param[in] func 実行したい関数オブジェクト。戻り値で false を返すと1回限りの実行、true を返すと継続的に実行されます。
 * @return イベントソースのID。
 */
template <typename F>
guint idle_add(F&& func) {
	return g_idle_add(detail::dispatch_to_gmain, new std::function<bool()>(std::forward<F>(func)));
}

/**
 * @brief GLibのメインループで関数オブジェクトを1回だけ非同期実行する。
 * @tparam F 関数オブジェクトの型。
 * @param[in] func 実行したい関数オブジェクト。戻り値を返したとしても使用されません。
 * @return イベントソースのID。
 */
template <typename F>
guint idle_add_once(F&& func) {
#if GLIB_CHECK_VERSION(2, 74, 0)

	return g_idle_add_once(detail::dispatch_to_gmain_once, new std::function<void()>(std::forward<F>(func)));

#else

	// ユーザーの void() 関数をラップし、強制的に false (G_SOURCE_REMOVE) を返す
	// bool() 関数として idle_add に渡す。
	return idle_add([f = std::forward<F>(func)]() mutable -> bool {
		f();
		return false; // 1回で終了させる
	});

#endif
}

} // namespace gutils
