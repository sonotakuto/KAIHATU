/**
 * @file utility.hpp
 * @brief 汎用的なユーティリティ関数やクラスの定義
 */
#pragma once

#include <utility> // std::move

/**
 * @namespace utils
 * @brief ユーティリティ関数やクラスをまとめた名前空間
 */
namespace utils {

/**
 * @brief 値を一時的に設定するRAII（スコープベース）ラッパー
 *
 * このクラスは、コンストラクタでターゲット変数を新しい値に設定し、
 * デストラクタ（スコープを抜ける時）で元の値を復元します。
 *
 * @tparam T 管理する値の型
 */
template <typename T>
class scoped_value_guard {
public:
	/**
	 * @brief ガードを構築します。
	 *
	 * 'target' の現在の値を退避し、'target' を 'new_value' に設定します。
	 *
	 * @param[in] target 管理対象の変数への参照
	 * @param[in] new_value スコープ内で設定する一時的な値
	 */
	scoped_value_guard(T& target, T new_value)
		: target_ref_(target), old_value_(std::move(target)) // 1. 元の値を退避
	{
		target_ref_ = std::move(new_value); // 2. 新しい値を設定
	}

	/**
	 * @brief ガードを破棄します。
	 *
	 * （コンストラクタで退避した）元の値を 'target' に復元します。
	 */
	~scoped_value_guard() {
		target_ref_ = std::move(old_value_); // 3. 元の値を復元
	}

	// このガードオブジェクト自体のコピーやムーブは危険なため禁止します。
	scoped_value_guard(const scoped_value_guard&) = delete;
	scoped_value_guard& operator=(const scoped_value_guard&) = delete;
	scoped_value_guard(scoped_value_guard&&) = delete;
	scoped_value_guard& operator=(scoped_value_guard&&) = delete;

private:
	T& target_ref_; // 監視対象の変数への参照
	T  old_value_;  // 退避しておいた元の値
};

} // namespace utils
