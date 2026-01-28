/**
 * @file gmain_utils.cpp
 * @brief GLibのメインループ関連のユーティリティ関数の実装
 */
#include "gmain_utils.hpp"

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

	gboolean dispatch_to_gmain(gpointer func) {
		// 渡されたポインタを元の std::function オブジェクトにキャスト
		auto* casted_func = static_cast<std::function<bool()>*>(func);

		// 関数を実行
		bool result = (*casted_func)();

		// イベントソースのリストから削除される場合は、関数オブジェクトを解放
		if (!result) {
			delete casted_func;
		}

		// 関数オブジェクトの戻り値を、G_SOURCE_** に変換
		return ((result) ? (G_SOURCE_CONTINUE) : (G_SOURCE_REMOVE));
	}

#if GLIB_CHECK_VERSION(2, 74, 0)

	void dispatch_to_gmain_once(gpointer func) {
		// void型としてキャスト
		auto* casted_func = static_cast<std::function<void()>*>(func);

		// 関数を実行 (戻り値チェックは不要)
		(*casted_func)();

		// Once版は必ず1回で終わるため、結果に関わらず無条件で解放する
		delete casted_func;
	}

#endif

} // namespace detail

} // namespace gutils
