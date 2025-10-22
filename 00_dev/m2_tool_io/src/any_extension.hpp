/**
 * @file any_extension.hpp
 * @brief std::any型に対する拡張機能を提供するヘッダファイル。
 */
#pragma once
#include <any>
#include <optional>
#include <stdexcept>

/**
 * @namespace stdex
 * @brief 標準ライブラリの拡張機能を格納する名前空間。
 */
namespace stdex{
/**
 * @brief std::any を std::optional<T> に安全に変換します。
 * @tparam T 取り出したい型
 * @param a 対象の std::any
 * @retval std::nullopt (a.has_value() が false の場合)
 * @retval T型の値 (a.has_value() が true で、型がTと一致した場合)
 * @throws std::bad_any_cast (a.has_value() が true だが、型がTと一致しない場合)
 */
template<typename T>
std::optional<T> any_cast_optional(const std::any& a) {
	if (!a.has_value()) {
		return std::nullopt;
	}

	// 値があり、型が T と一致しない場合、ここで std::bad_any_cast が送出される
	return std::any_cast<T>(a);
}

} // namespace stdex