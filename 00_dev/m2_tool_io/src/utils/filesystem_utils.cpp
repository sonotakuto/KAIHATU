/**
 * @file filesystem_utils.cpp
 * @brief ファイルシステムに関する拡張機能の実装ファイル。
 */
#include "filesystem_utils.hpp"
#include <limits.h>

/**
 * @namespace utils
 * @brief ユーティリティ関数やクラスをまとめた名前空間
 */
namespace utils {

/**
 * @namespace utils::detail
 * @brief utils名前空間内の詳細実装を格納する名前空間
 */
namespace filesystem {

namespace fs = std::filesystem;

const std::pair<fs::path, bool>& get_executable_dir() {
	// static変数により、初期化は「最初の呼び出し時」に一度だけ行われます。
	// C++11以降、この初期化はスレッドセーフです。
	static const auto cache = []() -> std::pair<fs::path, bool> {
		std::error_code ec;
		// Linuxの場合、/proc/self/exe が実行ファイルへのシンボリックリンク
		fs::path p = fs::read_symlink("/proc/self/exe", ec);

		if (!ec) {
			return {p.parent_path(), true};
		}

		// 失敗時のフォールバック
		// 注意: 初回呼び出し時のカレントディレクトリがキャッシュされます
		return {fs::current_path(), false};
	}();

	return cache;
}

} // namespace filesystem
} // namespace utils
