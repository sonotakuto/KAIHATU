/**
 * @file math_utils.hpp
 * @brief 算術関連の汎用ユーティリティ関数を定義します。
 */
#pragma once

#include <cstddef> // std::size_t のため
#include <stdexcept> // std::domain_error のため (将来的な拡張用)

/**
 * @namespace utils
 * @brief ユーティリティ関数やクラスをまとめた名前空間
 */
namespace utils {

/**
 * @brief C++17 互換の constexpr べき乗関数 (テンプレート版)。
 * @details
 * std::pow() は C++17 では constexpr ではないため、
 * コンパイル時定数を計算するためにforループで計算します。
 * @tparam T 基数(base)と戻り値の型 (例: int, long long, double)。
 * @param[in] base 基数。
 * @param[in] exp 指数 (0以上の整数)。
 * @return base の exp 乗 (base^exp)。
 */
template <typename T>
constexpr T constexpr_pow(T base, std::size_t exp) {
	T result = static_cast<T>(1);

	// 指数が0なら 1 を返す (0^0 も 1 とする)
	if (exp == 0) {
		return static_cast<T>(1);
	}

	// 指数が1以上の場合
	for (std::size_t i = 0; i < exp; i++) {
		result *= base;
	}
	return result;
}

} // namespace utils
