/**
 * @file gmain_utils.cpp
 * @brief GLibのメインループ関連のユーティリティ関数の実装
 */
#include "gmain_utils.hpp"
#include "utils/string_utils.hpp"

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

	gboolean dispatch_to_gmain(gpointer func) {
		// 渡されたポインタを元の std::function オブジェクトにキャスト
		auto* casted_func = static_cast<std::function<bool()>*>(func);

		try {
			// 関数を実行
			bool result = (*casted_func)();
			return ((result) ? (G_SOURCE_CONTINUE) : (G_SOURCE_REMOVE));	// 関数オブジェクトの戻り値を、G_SOURCE_** に変換
		} catch (const std::exception& e) {
			LogManager::logError(utils::concat("[GMain] Exception caught in main loop:", e.what()));
			return G_SOURCE_REMOVE; // 例外発生時は安全のためにイベントソースの実行を終了させる
		} catch (...) {
			LogManager::logError("[GMain] Unknown exception caught in main loop.");
			return G_SOURCE_REMOVE;
		}
	}

	void destroy_gmain_data(gpointer data) {
		// g_source_remove等でイベントが破棄される際に必ず呼ばれるため、ここで安全にメモリ解放
		delete static_cast<std::function<bool()>*>(data);
	}

} // namespace detail

} // namespace gutils
