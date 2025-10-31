#include <any>
#include <optional>
#include <functional> // std::reference_wrapper
#include <type_traits> // std::is_reference_v, std::remove_reference_t

/**
 * @namespace stdex
 * @brief 標準ライブラリの拡張機能を格納する名前空間。
 */
namespace stdex{

// --- ヘルパー (C++17のstd::optional<T&>対応) ---

/**
 * @brief T の型に応じて、any_cast_optional の戻り値型を決定します。
 * T が値型 (int)				-> std::optional<int>
 * T が T& (int&)				-> std::optional<std::reference_wrapper<int>>
 * T が const T& (const int&)	-> std::optional<std::reference_wrapper<const int>>
 */
template<typename T>
struct optional_cast_return_type {
	using type = std::optional<T>;
};

template<typename T>
struct optional_cast_return_type<T&> {
	using type = std::optional<std::reference_wrapper<T>>;
};

template<typename T>
struct optional_cast_return_type<const T&> {
	using type = std::optional<std::reference_wrapper<const T>>;
};

template<typename T>
using optional_cast_return_type_t = typename optional_cast_return_type<T>::type;


// --- 1. 参照渡し (例外あり) バージョン ---
// 元のコードの仕様（空なら nullopt、型不一致なら bad_any_cast）に従います。

/**
 * @brief std::any を std::optional に安全に変換します。 (std::any& 版)
 *
 * std::any_cast<T>(std::any&) に対応します。
 * T に応じて適切な std::optional (値 or reference_wrapper) を返します。
 *
 * @tparam T std::any_cast<T> で指定する型 (e.g., int, int&, const int&)
 * @param[in] a 対象の std::any
 * @retval std::nullopt (a.has_value() が false の場合)
 * @retval 格納された値 (a.has_value() が true で、型がTと一致した場合)
 * @throws std::bad_any_cast (a.has_value() が true だが、型がTと一致しない場合)
 */
template<typename T>
auto any_cast_optional(std::any& a) -> optional_cast_return_type_t<T> {
	if (!a.has_value()) {
		return std::nullopt;
	}

	if constexpr (std::is_reference_v<T>) {
		// T が T& または const T& の場合
		// std::any_cast<T>(a) は T& または const T& を返す
		return std::ref(std::any_cast<T>(a)); // std::ref or std::cref
	} else {
		// T が値型の場合 (コピー)
		return std::any_cast<T>(a);
	}
}

/**
 * @brief std::any を std::optional に安全に変換します。 (const std::any& 版)
 *
 * std::any_cast<T>(const std::any&) に対応します。
 * T が T& (非const参照) の場合、std::bad_any_cast がスローされます。
 *
 * @tparam T std::any_cast<T> で指定する型 (e.g., int, const int&)
 * @param[in] a 対象の std::any
 * @retval std::nullopt (a.has_value() が false の場合)
 * @retval 格納された値 (a.has_value() が true で、型がTと一致した場合)
 * @throws std::bad_any_cast (a.has_value() が true だが、型がTと一致しない、または const から T& を要求した場合)
 */
template<typename T>
auto any_cast_optional(const std::any& a) -> optional_cast_return_type_t<T> {
	if (!a.has_value()) {
		return std::nullopt;
	}

	if constexpr (std::is_reference_v<T>) {
		// T が T& または const T& の場合
		// T& の場合は bad_any_cast がスローされる
		return std::cref(std::any_cast<T>(a));
	} else {
		// T が値型の場合 (コピー)
		return std::any_cast<T>(a);
	}
}

/**
 * @brief std::any を std::optional に安全に変換します。 (std::any&& 版)
 *
 * std::any_cast<T>(std::any&&) に対応します。
 * C++17 では T は値型である必要があります。
 *
 * @tparam T std::any_cast<T> で指定する型 (値型のみ)
 * @param[in] a 対象の std::any (ムーブされます)
 * @retval std::nullopt (a.has_value() が false の場合)
 * @retval 格納された値 (a.has_value() が true で、型がTと一致した場合)
 * @throws std::bad_any_cast (a.has_value() が true だが、型がTと一致しない場合)
 */
template<typename T>
auto any_cast_optional(std::any&& a) -> optional_cast_return_type_t<T> {
	// C++17 の std::any_cast(std::any&&) は T が値型である必要がある
	static_assert(!std::is_reference_v<T>, "C++17 std::any_cast(std::any&&) requires T to be a value type.");

	if (!a.has_value()) {
		return std::nullopt;
	}

	// T は値型のみ (ムーブ)
	return std::any_cast<T>(std::move(a));
}


// --- 2. ポインタ渡し (例外なし) バージョン ---
// std::any_cast のポインタ版（例外をスローしない）に対応します。

/**
 * @brief std::any から T* を安全に取り出します。 (std::any* 版、例外なし)
 *
 * std::any_cast<T>(std::any*) に対応します。
 *
 * @tparam T 取り出したい型 (e.g., int)
 * @param[in] a 対象の std::any へのポインタ
 * @retval std::nullopt (a が nullptr、a->has_value() が false、または型がTと一致しない場合)
 * @retval std::optional<T*> (型がTと一致した場合)
 */
template<typename T>
std::optional<T*> any_cast_optional(std::any* a) noexcept {
	// std::any_cast<T>(std::any*) は、a が nullptr、空、または型が一致しない場合 nullptr を返す
	T* ptr = std::any_cast<T>(a);
	if (ptr) {
		return ptr;
	}
	return std::nullopt;
}

/**
 * @brief std::any から const T* を安全に取り出します。 (const std::any* 版、例外なし)
 *
 * std::any_cast<T>(const std::any*) に対応します。
 *
 * @tparam T 取り出したい型 (e.g., int)
 * @param[in] a 対象の std::any へのポインタ
 * @retval std::nullopt (a が nullptr、a->has_value() が false、または型がTと一致しない場合)
 * @retval std::optional<const T*> (型がTと一致した場合)
 */
template<typename T>
std::optional<const T*> any_cast_optional(const std::any* a) noexcept {
	// std::any_cast<T>(const std::any*)
	const T* ptr = std::any_cast<T>(a);
	if (ptr) {
		return ptr;
	}
	return std::nullopt;
}

} // namespace stdex
