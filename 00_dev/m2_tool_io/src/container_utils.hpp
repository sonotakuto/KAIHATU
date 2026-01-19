/**
 * @file container_utils.hpp
 * @brief コンテナ操作に関するユーティリティ関数を提供するヘッダファイル。
 */

#include <iostream>
#include <map>
#include <optional>
#include <type_traits>
#include <vector>

/**
 * @namespace utils
 * @brief ユーティリティ関数やクラスをまとめた名前空間
 */
namespace utils {

/**
 * @namespace detail
 * @brief utils名前空間内の詳細実装を格納する名前空間
 */
namespace detail {

	// --- 厳密な特性検出 (SFINAE) ---

	// 1. Map的な find() を持っているか？
	// 条件:
	//  a) find(key) が呼べる
	//  b) end() が呼べる
	//  c) find(key) != end() が比較できる
	//  d) find(key)->second が参照できる (★ここが重要)
	template <typename C, typename Key, typename = void>
	struct is_standard_map_find : std::false_type {};

	template <typename C, typename Key>
	struct is_standard_map_find<C, Key, std::void_t<
		decltype(std::declval<C>().find(std::declval<Key>()) != std::declval<C>().end()), // 比較チェック
		decltype(std::declval<C>().find(std::declval<Key>())->second)                     // 戻り値の振る舞いチェック
	>> : std::true_type {};


	// 2. contains() と at() を両方持っているか？
	// 条件:
	//  a) contains(key) が呼べる
	//  b) at(key) が呼べる
	template <typename C, typename Key, typename = void>
	struct supports_direct_lookup : std::false_type {};

	template <typename C, typename Key>
	struct supports_direct_lookup<C, Key, std::void_t<
		decltype(std::declval<C>().contains(std::declval<Key>())),
		decltype(std::declval<C>().at(std::declval<Key>()))
	>> : std::true_type {};


	// 3. 配列的な at() と size() を持っているか？
	// 条件:
	//  a) at(index) が呼べる
	//  b) size() が呼べる
	//  c) Keyが整数型である (これは関数内で判定)
	template <typename C, typename Key, typename = void>
	struct is_standard_vector_at : std::false_type {};

	template <typename C, typename Key>
	struct is_standard_vector_at<C, Key, std::void_t<
		decltype(std::declval<C>().at(std::declval<Key>())),
		decltype(std::declval<C>().size())
	>> : std::true_type {};


	// 4. operator[] と size() を持っているか？
	template <typename C, typename Key, typename = void>
	struct is_standard_array_subscript : std::false_type {};

	template <typename C, typename Key>
	struct is_standard_array_subscript<C, Key, std::void_t<
		decltype(std::declval<C>()[std::declval<Key>()]),
		decltype(std::declval<C>().size())
	>> : std::true_type {};


	// エラー遅延用
	template<class T> inline constexpr bool always_false_v = false;

} // namespace detail

// --- 公開インターフェース ---

/**
 * @brief 任意のコンテナから安全に値を取得し、std::optional で返します。
 * @details コンテナの種類に応じて最適なアクセス方法を自動選択します。
 * キーが存在しない、または範囲外の場合は std::nullopt を返します。
 * @tparam Container コンテナの型
 * @tparam Key キーの型
 * @param[in] c コンテナの参照
 * @param[in] key 取得するキーまたはインデックス
 * @return std::optional<ValueType> 取得した値、または nullopt
 */
template <typename Container, typename Key>
constexpr auto try_at(const Container& c, Key&& key) {
	// 標準的な Map (find -> iterator -> second)
	if constexpr (detail::is_standard_map_find<Container, Key>::value) {
		// 戻り値の型を推論 (例: int, std::string)
		using ValT = std::decay_t<decltype(c.find(key)->second)>;

		auto it = c.find(key);
		if (it != c.end()) {
			return std::optional<ValT>(it->second);
		}
		return std::optional<ValT>(std::nullopt);
	}
	// Direct lookup map (contains + at)
	else if constexpr (detail::supports_direct_lookup<Container, Key>::value) {
		using ValT = std::decay_t<decltype(c.at(key))>;

		if (c.contains(key)) {
			return std::optional<ValT>(c.at(key));
		}
		return std::optional<ValT>(std::nullopt);
	}
	// Vector (at + size, Keyは整数)
	else if constexpr (std::is_integral_v<std::decay_t<Key>> && detail::is_standard_vector_at<Container, Key>::value) {
		using ValT = std::decay_t<decltype(c.at(key))>;
		using SizeType = decltype(c.size());

		if (key >= 0 && static_cast<SizeType>(key) < c.size()) {
			return std::optional<ValT>(c.at(key));
		}
		return std::optional<ValT>(std::nullopt);
	}
	// Array (operator[] + size, Keyは整数)
	else if constexpr (std::is_integral_v<std::decay_t<Key>> && detail::is_standard_array_subscript<Container, Key>::value) {
		using ValT = std::decay_t<decltype(c[key])>;
		using SizeType = decltype(c.size());

		if (key >= 0 && static_cast<SizeType>(key) < c.size()) {
			return std::optional<ValT>(c[key]);
		}
		return std::optional<ValT>(std::nullopt);
	}
	// 対応不可
	else {
		static_assert(detail::always_false_v<Container>,
			"utils::try_at: Unsupported container interface. "
			"Container must support [find()], [contains()+at()], [at()+size()], or [operator[]+size()].");
	}
}

/**
 * @brief 任意のコンテナから安全に値を取得します。
 * @details コンテナの種類(Map, Vector, etc.)に応じて最適なアクセス方法を自動選択します。
 * キーが存在しない、または範囲外の場合はデフォルト値を返します。
 * @tparam Container コンテナの型
 * @tparam Key キーの型
 * @tparam Default デフォルト値の型
 * @param[in] c コンテナの参照
 * @param[in] key 取得するキーまたはインデックス
 * @param[in] def_val キーが存在しない場合に返すデフォルト値
 * @return キーに対応する値、またはデフォルト値
 */
template <typename Container, typename Key, typename Default>
constexpr auto at_or(const Container& c, Key&& key, Default&& def_val) {
	// 標準的な Map (find -> iterator -> second)
	if constexpr (detail::is_standard_map_find<Container, Key>::value) {
		auto it = c.find(key);
		if (it != c.end()) {
			return it->second;
		}
	}
	// Direct lookup map (contains + at)
	else if constexpr (detail::supports_direct_lookup<Container, Key>::value) {
		if (c.contains(key)) {
			return c.at(key);
		}
	}
	// Vector (at + size, Keyは整数)
	else if constexpr (std::is_integral_v<std::decay_t<Key>> && detail::is_standard_vector_at<Container, Key>::value) {
		using SizeType = decltype(c.size());
		if (key >= 0 && static_cast<SizeType>(key) < c.size()) {
			return c.at(key);
		}
	}
	// Array (operator[] + size, Keyは整数)
	else if constexpr (std::is_integral_v<std::decay_t<Key>> && detail::is_standard_array_subscript<Container, Key>::value) {
		using SizeType = decltype(c.size());
		if (key >= 0 && static_cast<SizeType>(key) < c.size()) {
			return c[key];
		}
	}
	// 対応不可
	else {
		static_assert(detail::always_false_v<Container>,
			"at_or: Unsupported container interface. "
			"Please ensure your container follows standard conventions (e.g., find() returns iterator with 'second', or has at()/size()).");
	}

	return std::forward<Default>(def_val);
}

} // namespace utils
