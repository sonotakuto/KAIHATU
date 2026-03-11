/**
 * @file gio_utils.hpp
 * @brief GIO（D-Busやプロセス起動など）に関連するユーティリティ関数の定義
 */
#pragma once
#include <functional>
#include <string>

/**
 * @namespace gutils
 * @brief GLib関連のユーティリティをまとめた名前空間。
 */
namespace gutils {

/**
 * @struct LogManager
 * @brief ライブラリ内のログ出力を一元管理する静的クラス
 */
struct LogManager {
	/**
	 * @brief ユーザーがログ出力の方法を指定できるようにするためのコールバック関数
	 * @warning スレッドセーフティではないため、マルチスレッドプログラムの場合は、起動直後以外での設定は避けることを推奨します。
	 */
	inline static std::function<void(const std::string&)> error_callback = nullptr;

	/**
	 * @brief エラーメッセージをログ出力するための静的関数。
	 * @param[in] msg 出力したいエラーメッセージ。
	 */
	static void logError(const std::string& msg) {
		if (error_callback) {
			error_callback(msg);
		}
		// 未設定時は何もしない、または std::cerr にフォールバックする等を選択
	}
};

} // namespace gutils
