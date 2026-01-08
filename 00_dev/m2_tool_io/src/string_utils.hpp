/**
 * @file string_utils.hpp
 * @brief 文字列操作ユーティリティ関数群の定義。
 */
#pragma once
#include <string>
#include <string_view>
#include <algorithm>

/**
 * @namespace str_util
 * @brief 文字列操作ユーティリティ関数群を提供する名前空間。
 */
namespace str_util {

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

} // namespace str_util
