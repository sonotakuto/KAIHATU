/**
 * @file opt_extension.hpp
 * @brief std::optional に対する拡張機能を定義したヘッダファイル。
 */
#pragma once
#include <optional>
#include <type_traits>
#include <utility>

/**
 * @namespace stdex
 * @brief 標準ライブラリの拡張機能を格納する名前空間。
 */
namespace stdex {

namespace internal {

// T型が operator!= を持っているか判定するメタ関数
template <typename T, typename = void>
struct has_inequality : std::false_type {};

template <typename T>
struct has_inequality<T, std::void_t<decltype(std::declval<const T&>() != std::declval<const T&>())>> : std::true_type {};

// 判定結果を定数として保持 (C++17)
template <typename T>
inline constexpr bool has_inequality_v = has_inequality<T>::value;

} // namespace internal

/**
 * @brief オプショナル値をマージする（コピー版）。
 * @details
 * source に値が存在する場合に処理を行います。
 * 以下のいずれかの場合に target を source の値で更新（コピー）します。
 * 1. target が std::nullopt である。
 * 2. T型が比較可能(operator!=)で、target と source の値が異なる。
 * 3. T型が比較不能で、source に値がある（無条件更新）。
 * @tparam T 格納されている値の型。
 * @param[in,out] target 更新対象のオプショナル値。
 * @param[in] source 適用する新しいオプショナル値。
 * @return 更新が行われた場合 true、変更がない場合 false。
 */
template<typename T>
bool opt_merge(std::optional<T>& target, const std::optional<T>& source) {
	bool changed = false;
	if (source.has_value()) {
		bool should_update = false;

		if (!target.has_value()) {
			should_update = true;
		} else {
			// コンパイル時に分岐: 比較可能なら比較し、そうでなければ強制更新
			if constexpr (internal::has_inequality_v<T>) {
				if (target.value() != source.value()) {
					should_update = true;
				}
			} else {
				should_update = true;
			}
		}

		if (should_update) {
			target = source; // コピー代入
			changed = true;
		}
	}
	return changed;
}

/**
 * @brief オプショナル値をマージする（ムーブ版）。
 * @tparam T 格納されている値の型。
 * @param[in,out] target 更新対象のオプショナル値。
 * @param[in] source 適用する新しいオプショナル値（右辺値参照）。
 * @return 更新が行われた場合 true、変更がない場合 false。
 * @details
 * std::unique_ptr などのムーブ専用型に対応したオーバーロードです。
 * 更新条件はコピー版と同様ですが、更新時は source から target へムーブ代入されます。
 * 実行後、source の状態は不定となります。
 */
template<typename T>
bool opt_merge(std::optional<T>& target, std::optional<T>&& source) {
	bool changed = false;
	if (source.has_value()) {
		bool should_update = false;

		if (!target.has_value()) {
			should_update = true;
		} else {
			if constexpr (internal::has_inequality_v<T>) {
				if (target.value() != source.value()) {
					should_update = true;
				}
			} else {
				should_update = true;
			}
		}

		if (should_update) {
			target = std::move(source); // ムーブ代入
			changed = true;
		}
	}
	return changed;
}

// --- 構造体一括マージ用のヘルパー ---

/**
 * @brief 複数のメンバ変数をまとめてマージする（コピー版）
 * @details C++17の畳み込み式(Fold Expression)を使用しています。
 * @param[in] target 更新対象の構造体インスタンス。
 * @param[in] source 適用する新しい構造体インスタンス。
 * @param[in] members マージ対象のメンバ変数のポインタ。
 * @return 変更があったかどうか (true: 変更あり, false: 変更なし)
 */
template <typename T, typename... MemberPtrs>
bool merge_struct(T& target, const T& source, MemberPtrs... members) {
	bool changed = false;
	// 畳み込み式: 全てのメンバポインタに対して opt_merge を実行し、結果を |= で集約
	((changed |= opt_merge(target.*members, source.*members)), ...);
	return changed;
}

/**
 * @brief 複数のメンバ変数をまとめてマージする（ムーブ版）
 * @details C++17の畳み込み式(Fold Expression)を使用しています。
 * @param[in] target 更新対象の構造体インスタンス。
 * @param[in] source 適用する新しい構造体インスタンス（右辺値参照）。
 * @param[in] members マージ対象のメンバ変数のポインタ。
 * @return 変更があったかどうか (true: 変更あり, false: 変更なし)
 */
template <typename T, typename... MemberPtrs>
bool merge_struct(T& target, T&& source, MemberPtrs... members) {
	bool changed = false;
	// ムーブ版の opt_merge を呼び出す。source.*members を std::move でキャスト
	((changed |= opt_merge(target.*members, std::move(source.*members))), ...);
	return changed;
}

/**
 * @brief クラスの コピー/ムーブ両方の merge 関数（他のインスタンスでこのインスタンスを更新する関数）を自動生成するマクロ
 * @param[in] ClassName (マクロ引数) クラス自身の型名
 * @param[in] ... (マクロ引数) マージ対象のメンバ変数のポインタ (例: &ClassName::member)
 * @param[in] other (関数引数) 更新に使用するインスタンス。
 * @return (関数戻り値) 変更があったかどうか (true: 変更あり, false: 変更なし)
 */
#define STDEX_DEFINE_MERGE_FUNC(ClassName, ...) \
	bool merge(const ClassName& other) { \
		return stdex::merge_struct(*this, other, __VA_ARGS__); \
	} \
	bool merge(ClassName&& other) { \
		return stdex::merge_struct(*this, std::move(other), __VA_ARGS__); \
	}

} // namespace stdex
