/**
 * @file filesystem_utils.hpp
 * @brief ファイルシステムに関する拡張機能を提供するヘッダーファイル。
 */
#pragma once
#include <filesystem>
#include <utility>

/**
 * @namespace utils
 * @brief ユーティリティ関数やクラスをまとめた名前空間
 */
namespace utils {

/**
 * @namespace utils::filesystem
 * @brief ファイルシステム関連のユーティリティ関数をまとめた名前空間
 */
namespace filesystem {

/**
 * @brief 実行ファイルのディレクトリパスを取得します。
 * @return 実行ファイルのディレクトリパスと成功フラグのペアの内部キャッシュへの参照。
 * 成功フラグが true の場合、パスの取得に成功しています。
 * 失敗した場合、パスには現在のカレントディレクトリが設定され、成功フラグは false になります。
 *
 * @note 初回呼び出し時にのみパスの解決が行われ、その結果がキャッシュされます。
 * 以降の呼び出しではキャッシュされた結果が返されます。
 */
const std::pair<std::filesystem::path, bool>& get_executable_dir();

/**
 * @brief 複数の引数をパスとして結合し、新しい std::filesystem::path を作成します。
 *
 * operator/ を連鎖させる (a / b / c) 代わりに、初期化したオブジェクトに対して
 * 順次 operator/= を適用することで、中間の一時オブジェクト生成を防ぎます。
 *
 * @tparam Args std::filesystem::path に変換可能な型
 * @param[in] head 最初のパス要素
 * @param[in] tail 後続のパス要素（可変長引数）
 * @return std::filesystem::path 結合された新しいパス
 *
 * @note C++17 以降が必要です（Fold Expressions 使用）。
 */
template <typename Head, typename... Tail>
std::filesystem::path path_join(Head const& head, Tail const&... tail) {
	std::filesystem::path result = head;

	// Fold Expression (Comma operator)
	// 順次 /= を適用して追記します
	(result /= ... /= tail); // C++17 binary fold (または (result /= tail, ...) も可)

	return result;
}

/**
 * @brief 既存の std::filesystem::path に複数の引数をディレクトリ階層として追記します。
 *
 * operator/= を使用して、既存のオブジェクト（dest）に対して直接操作を行います。
 *
 * @tparam Args std::filesystem::path に変換可能な型
 * @param[in,out] dest 追記先のパスオブジェクト
 * @param[in] args 追記したいパス要素（可変長引数）
 * @return 追記後の dest への参照
 *
 * @warning **自己参照（Aliasing）の禁止**
 * 出力先の `dest` の内部文字列表現（`dest.native()` や `dest.c_str()` 等）を
 * 引数 `args` に渡さないでください。
 * \n
 * **理由:** `std::filesystem::path` は内部の実装によって（特に追記時に）
 * バッファの再配置を行う可能性があります。その際、引数が `dest` 内部を指していると
 * 無効なメモリ領域へのアクセス（ダングリングポインタ）となる危険性があります。
 */
template <typename... Args>
std::filesystem::path& append_path_join(std::filesystem::path& dest, Args const&... args) {
	// 順次 /= を適用
	(dest /= ... /= args);
	return dest;
}

/**
 * @brief 既存の std::filesystem::path の内容を破棄し、新しい要素でパスを再構築します。
 *
 * 内部で `clear()` を呼んだ後、順次要素を結合します。
 * std::string と異なり capacity の制御はできませんが、インスタンスを再利用します。
 *
 * @tparam Args std::filesystem::path に変換可能な型
 * @param[in,out] dest 上書き先のパスオブジェクト
 * @param[in] args 新しいパス要素（可変長引数）
 * @return  上書き後の dest への参照
 *
 * @warning **自己参照（Aliasing）の厳禁**
 * 出力先の `dest` 自身、あるいは `dest` の内部バッファを参照するオブジェクトを
 * 引数 `args` に渡さないでください。
 * \n
 * **理由:** 最初の `dest.clear()` によって `dest` の保持する内容は即座に破棄されるため、
 * 引数として渡した参照元データも消失し、未定義動作を引き起こします。
 */
template <typename... Args>
std::filesystem::path& assign_path_join(std::filesystem::path& dest, Args const&... args) {
	// 1. 中身をクリア
	dest.clear();

	// 2. 順次追記 (Fold Expression)
	// 引数が空の場合でも clear() だけで正しく動作するようにしています
	if constexpr (sizeof...(args) > 0) {
		(dest /= ... /= args);
	}
	return dest;
}

} // namespace filesystem
} // namespace utils
