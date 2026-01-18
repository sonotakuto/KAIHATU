/**
 * @file string_utils.hpp
 * @brief 文字列操作ユーティリティ関数群の定義。
 */
#pragma once
#include <string>
#include <string_view>
#include <algorithm>

/**
 * @namespace utils
 * @brief ユーティリティ関数やクラスをまとめた名前空間
 */
namespace utils {

// デフォルトの空白文字
inline constexpr std::string_view WHITESPACE(" \t\n\r\f\v");

/**
 * @brief 左側の文字を除去
 * @param[in] sv 対象の文字列ビュー。
 * @param[in] chars 除去する文字の集合 (デフォルトは空白文字)。
 * @return 除去後の文字列ビュー。
 */
constexpr std::string_view lstrip(std::string_view sv, std::string_view chars = WHITESPACE) {
	size_t start = sv.find_first_not_of(chars);
	return ((start == std::string_view::npos) ? (std::string_view{}) : (sv.substr(start)));
}

/**
 * @brief 右側の文字を除去
 * @param[in] sv 対象の文字列ビュー。
 * @param[in] chars 除去する文字の集合 (デフォルトは空白文字)。
 * @return 除去後の文字列ビュー。
 */
constexpr std::string_view rstrip(std::string_view sv, std::string_view chars = WHITESPACE) {
	size_t end = sv.find_last_not_of(chars);
	return ((end == std::string_view::npos) ? (std::string_view{}) : (sv.substr(0, end + 1)));
}

/**
 * @brief 両側の文字を除去
 * @param[in] sv 対象の文字列ビュー。
 * @param[in] chars 除去する文字の集合 (デフォルトは空白文字)。
 * @return 除去後の文字列ビュー。
 */
constexpr std::string_view strip(std::string_view sv, std::string_view chars = WHITESPACE) {
	return rstrip(lstrip(sv, chars), chars);
}

/**
 * @brief 左側の文字をその場で除去 (In-place)
 * @details 文字列の前方を詰めるため、データ移動(memmove)が発生しますが、
 * メモリの再割り当ては発生しません。
 */
inline std::string& lstrip_inplace(std::string& s, std::string_view chars = WHITESPACE) {
	size_t start = s.find_first_not_of(chars);
	if (start == std::string::npos) {
		s.clear(); // 全て除去対象なら空にする
	} else if (start > 0) {
		s.erase(0, start); // 前方start文字分を削除（左シフト）
	}
	return s;
}

/**
 * @brief 右側の文字をその場で除去 (In-place)
 * @details サイズを縮小するだけなので、データ移動もメモリ割り当ても発生せず非常に高速です。
 */
inline std::string& rstrip_inplace(std::string& s, std::string_view chars = WHITESPACE) {
	size_t end = s.find_last_not_of(chars);
	if (end == std::string::npos) {
		s.clear(); // 全て除去対象なら空にする
	} else if (end + 1 < s.size()) {
		s.erase(end + 1); // 末尾以降を切り捨て
	}
	return s;
}

/**
 * @brief 両側の文字をその場で除去 (In-place)
 * @return 変更された文字列への参照
 */
inline std::string& strip_inplace(std::string& s, std::string_view chars = WHITESPACE) {
	// rstripを先に行うことで、文字列が短くなった場合に
	// lstripでのデータ移動コストを僅かに減らせる可能性があります。
	return lstrip_inplace(rstrip_inplace(s, chars), chars);
}

/**
 * @brief 複数の引数を結合して新しい std::string を作成します。
 *
 * 引数の合計サイズを事前に計算し、メモリ確保（reserve）を1回のみ行うことで、
 * 無駄なアロケーションとコピーを最小限に抑えます。
 *
 * @tparam Args 文字列として扱える型（std::string, std::string_view, const char* 等）
 * @param[in] args 結合したい文字列要素（可変長引数）
 * @return std::string 結合された新しい文字列
 *
 * @note C++17 以降が必要です（Fold Expressions 使用）。
 * @note 入力の char 配列等はヌル終端されている必要があります。
 */
template <typename... Args>
std::string concat(Args const&... args) {
	// 1. 全ての引数の合計サイズを計算 (std::string_viewに変換してサイズ取得)
	//    これにより、終端文字走査のコストはあるものの、一時的なstd::string生成は防げます
	size_t len = (std::string_view(args).size() + ...);

	std::string result;
	// 2. メモリを一度だけ確保
	result.reserve(len);

	// 3. 順次追加 (Fold Expression)
	(result.append(args), ...);

	return result;
}

/**
 * @brief 既存の std::string バッファに複数の引数を効率的に追記します。
 *
 * 出力先となる文字列（dest）のキャパシティを確認・確保してから追記を行うため、
 * 頻繁な再割り当て（Reallocation）を防ぎます。
 * dest の capacity が十分であれば、メモリ割り当ては一切発生しません。
 *
 * @tparam Args 文字列として扱える型（std::string, std::string_view, const char* 等）
 * @param[in,out] dest 追記先の文字列バッファ
 * @param[in] args 追記したい文字列要素（可変長引数）
 *
 * @warning **自己参照（Aliasing）の禁止**
 * 出力先の `dest` 自身、あるいは `dest` の内部バッファを参照するオブジェクト
 * （`std::string_view` や `char*` ポインタなど）を引数 `args` に渡さないでください。
 * \n
 * **理由:** 内部で `dest.reserve()` を実行した際、メモリの再確保が発生すると
 * `dest` の古いバッファ領域が解放されます。このとき `args` がその解放された
 * 領域を指していると、ダングリングポインタへのアクセスとなり、
 * 未定義動作（クラッシュやデータ破損）を引き起こします。
 * \n
 * **対策:** `dest` の内容を含めて結合したい場合は、一度別の変数にコピーしてから渡してください。
 */
template <typename... Args>
void append_concat(std::string& dest, Args const&... args) {
	// 1. 追加する合計サイズを計算
	size_t added_len = (std::string_view(args).size() + ...);

	// 2. 現在のサイズ + 追加分で reserve
	//    (キャパシティが足りていれば、ここでメモリ確保は発生しません)
	dest.reserve(dest.size() + added_len);

	// 3. 追記
	(dest.append(args), ...);
}

/**
 * @brief 既存の std::string の内容を破棄し、新しい要素で上書きします。
 *
 * 出力先となる文字列（dest）のキャパシティを確認・確保してから追記を行うため、
 * 頻繁な再割り当て（Reallocation）を防ぎます。
 * dest の capacity が十分であれば、メモリ割り当ては一切発生しません。
 *
 * @tparam Args 文字列として扱える型（std::string, std::string_view, const char* 等）
 * @param[in,out] dest 追記先の文字列バッファ
 * @param[in] args 追記したい文字列要素（可変長引数）
 *
 * @warning **自己参照（Aliasing）の禁止**
 * 出力先の `dest` 自身、あるいは `dest` の内部バッファを参照するオブジェクト
 * （`std::string_view` や `char*` ポインタなど）を引数 `args` に渡さないでください。
 * \n
 * **理由:** 内部で `dest.reserve()` を実行した際、メモリの再確保が発生すると
 * `dest` の古いバッファ領域が解放されます。このとき `args` がその解放された
 * 領域を指していると、ダングリングポインタへのアクセスとなり、
 * 未定義動作（クラッシュやデータ破損）を引き起こします。
 * \n
 * **対策:** `dest` の内容を含めて結合したい場合は、一度別の変数にコピーしてから渡してください。
 */
template <typename... Args>
void assign_concat(std::string& dest, Args const&... args) {
	// 1. 必要サイズを計算
	size_t new_len = (std::string_view(args).size() + ...);

	// 2. 現在のキャパシティより大きくなる場合のみ reserve (拡張)
	//    (小さくなる場合は何もしない = メモリを解放しない)
	if (new_len > dest.capacity()) {
		dest.reserve(new_len);
	}

	// 3. 中身をクリア (サイズ0, キャパシティ維持)
	dest.clear();

	// 4. 書き込み
	(dest.append(args), ...);
}

} // namespace utils
