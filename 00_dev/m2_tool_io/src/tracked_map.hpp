/**
 * @file tracked_map.hpp
 * @brief 更新されたキーを追跡する機能を持つマップクラスを提供するヘッダファイル。
 */
#pragma once

#include <condition_variable>
#include <map>
#include <mutex>
#include <set>
#include <utility> // std::forward, std::pair
#include <functional> // std::less
#include <memory> // std::allocator, std::allocator_traits

/**
 * @namespace utils
 * @brief ユーティリティ関数やクラスをまとめた名前空間
 */
namespace utils {

/**
 * @class TrackedMap
 * @brief 更新されたキーを追跡する機能を持つ、シングルスレッド用のマップクラス。
 *
 * @note 追跡機能を担保するため、非constなイテレータを返す関数 (begin, end, findなど) や
 * 参照を返す operator[] は意図的に実装していません。
 * 要素の読み取りは at() (コピーを返す), const_iterator、
 * 書き込みは insert(), insert_or_assign(), emplace(), try_emplace() などをそれぞれ使用してください。
 *
 * @tparam _Key キーの型
 * @tparam _Tp 値の型
 * @tparam _Compare キーの比較関数の型
 * @tparam _Alloc アロケータの型
 */
template <typename _Key, typename _Tp, typename _Compare = std::less<_Key>,
		typename _Alloc = std::allocator<std::pair<const _Key, _Tp>>>
class TrackedMap {
public:
	// --- 型定義 ---
	typedef std::map<_Key, _Tp, _Compare, _Alloc>		map_type;
	typedef typename map_type::key_type					key_type;
	typedef typename map_type::mapped_type				mapped_type;
	typedef typename map_type::value_type				value_type;
	typedef typename map_type::key_compare				key_compare;
	typedef typename map_type::allocator_type			allocator_type;
	typedef typename map_type::size_type				size_type;
	typedef typename map_type::iterator					iterator;
	typedef typename map_type::const_iterator			const_iterator;
	typedef typename map_type::reverse_iterator			reverse_iterator;
	typedef typename map_type::const_reverse_iterator	const_reverse_iterator;

	typedef std::set<key_type, _Compare, typename std::allocator_traits<_Alloc>::template rebind_alloc<key_type>>	set_type;
	typedef typename set_type::key_type					set_key_type;

private:
	// --- メンバ変数 ---
	/// @brief 内部マップ
	map_type map;
	/// @brief 更新されたキーを保持するセット
	set_type updated_keys;

public:
	// --- コンストラクタ ---
	TrackedMap() = default;

	// --- イテレータ ---
	const_iterator begin() const noexcept { return map.begin(); }
	const_iterator end() const noexcept { return map.end(); }
	const_reverse_iterator rbegin() const noexcept { return map.rbegin(); }
	const_reverse_iterator rend() const noexcept { return map.rend(); }
	const_iterator cbegin() const noexcept { return map.cbegin(); }
	const_iterator cend() const noexcept { return map.cend(); }
	const_reverse_iterator crbegin() const noexcept { return map.crbegin(); }
	const_reverse_iterator crend() const noexcept { return map.crend(); }

	// --- 要素アクセス ---
	/**
	 * @brief 指定されたキーに対応する値を取得する。
	 * @param[in] key キー
	 * @param[in] flag_erase_updated_keys (オプション) trueの場合、取得したキーを更新済みリストから削除する。
	 * @return キーに対応する値。
	 * @throws std::out_of_range キーが存在しない場合。
	 */
	mapped_type at(const key_type& key, bool flag_erase_updated_keys = false) {
		mapped_type value = map.at(key);
		if (flag_erase_updated_keys) {
			updated_keys.erase(key);
		}
		return value;
	}

	// --- ゲッター (TrackedMap固有) ---
	/**
	 * @brief 全ての要素のコピーを取得する。
	 * @param[in] flag_clear_updated_keys (オプション) trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return マップ全体のコピー。
	 */
	map_type get_all(bool flag_clear_updated_keys = false) {
		map_type copy = map;
		if (flag_clear_updated_keys) {
			updated_keys.clear();
		}
		return copy;
	}

	/**
	 * @brief 更新されたキーのセットを取得する。
	 * @param[in] flag_clear_updated_keys (オプション) trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return 更新されたキーのセットのコピー。
	 */
	set_type get_updated_keys(bool flag_clear_updated_keys = false) {
		set_type copy = updated_keys;
		if (flag_clear_updated_keys) {
			updated_keys.clear();
		}
		return copy;
	}

	/**
	 * @brief 更新された要素（キーと値のペア）のマップを取得する。
	 * @param[in] flag_clear_updated_keys (オプション) trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return 更新された要素のみを含む新しいマップ。
	 */
	map_type get_updated_elements(bool flag_clear_updated_keys = false) {
		map_type updated_elements;
		for (const auto& key : updated_keys) {
			auto it = map.find(key);
			if (it != map.end()) {
				updated_elements[key] = it->second;
			}
		}
		if (flag_clear_updated_keys) {
			updated_keys.clear();
		}
		return updated_elements;
	}

	/**
	 * @brief 更新済みキーのリストをクリアする。
	 */
	void clear_updated_keys() {
		updated_keys.clear();
	}

	// --- 修飾子 ---
	/**
	 * @brief マップに要素を挿入する。
	 * @details
	 * 挿入が成功した場合、そのキーを `updated_keys` に追加します。
	 * @tparam P 挿入する値の型
	 * @param[in] x 挿入する値 (std::pair)。左辺値ならコピー、右辺値ならムーブされます。
	 * @return 挿入に成功した場合はtrue、キーが既に存在した場合はfalse。
	 */
	template <class P>
	bool insert(P&& x) {
		auto result = map.insert(std::forward<P>(x));
		if (result.second) {
			updated_keys.insert(result.first->first); // 更新されたキーを保存
		}
		return result.second;
	}

	/**
	 * @brief マップに要素を挿入または代入する。
	 * @details
	 * 挿入・代入どちらの場合でもキーは `updated_keys` に追加されます。
	 * @tparam M 挿入/代入する値の型
	 * @param[in] key キー
	 * @param[in] x 値
	 * @return 新規に挿入された場合はtrue、代入された場合はfalse。
	 */
	template <class M>
	bool insert_or_assign(const key_type& key, M&& x) {
		auto result = map.insert_or_assign(key, std::forward<M>(x));
		updated_keys.insert(key); // キーが更新されたことを記録
		return result.second;
	}

	/**
	 * @brief 要素をインプレース構築する。
	 * @param[in] args 値の構築引数
	 * @return 挿入された要素へのイテレータと、挿入が成功したか(true)を示すペア。
	 */
	template <class... Args>
	std::pair<const_iterator, bool> emplace(Args&&... args) {
		auto result = map.emplace(std::forward<Args>(args)...);
		if (result.second) {
			updated_keys.insert(result.first->first);
		}
		return result;
	}

	/**
	 * @brief ヒント付きで要素をインプレース構築する。
	 * @param[in] hint 挿入位置のヒント
	 * @param[in] args 値の構築引数
	 * @return 挿入された要素へのイテレータ。
	 */
	template <class... Args>
	const_iterator emplace_hint(const_iterator hint, Args&&... args) {
		auto old_size = map.size();
		auto it = map.emplace_hint(hint, std::forward<Args>(args)...);
		if (map.size() > old_size) {
			updated_keys.insert(it->first); // 新規挿入
		}
		return it;
	}

	/**
	 * @brief キーが存在しない場合のみ要素をインプレース構築する (C++17)。
	 * @param[in] key キー
	 * @param[in] args 値の構築引数
	 * @return 挿入された(または既存の)要素へのイテレータと、挿入が成功したか(true)を示すペア。
	 */
	template <class... Args>
	std::pair<const_iterator, bool> try_emplace(const key_type& key, Args&&... args) {
		auto result = map.try_emplace(key, std::forward<Args>(args)...);
		if (result.second) {
			updated_keys.insert(result.first->first);
		}
		return result;
	}

	/**
	 * @brief キーが存在しない場合のみ要素をインプレース構築する (C++17)。
	 * @param[in] key キー (ムーブされます)
	 * @param[in] args 値の構築引数
	 * @return 挿入された(または既存の)要素へのイテレータと、挿入が成功したか(true)を示すペア。
	 */
	template <class... Args>
	std::pair<const_iterator, bool> try_emplace(key_type&& key, Args&&... args) {
		auto result = map.try_emplace(std::move(key), std::forward<Args>(args)...);
		if (result.second) {
			updated_keys.insert(result.first->first);
		}
		return result;
	}

	/**
	 * @brief キーが存在しない場合のみ要素をインプレース構築する (C++17)。
	 * @param[in] hint 挿入位置のヒント
	 * @param[in] key キー (ムーブされます)
	 * @param[in] args 値の構築引数
	 * @return 挿入された(または既存の)要素へのイテレータと、挿入が成功したか(true)を示すペア。
	 */
	template <class... Args>
	const_iterator try_emplace(const_iterator hint, key_type&& key, Args&&... args) {
		auto old_size = map.size();
		auto it = map.try_emplace(hint, std::move(key), std::forward<Args>(args)...);
		if (map.size() > old_size) {
			updated_keys.insert(it->first);
		}
		return it;
	}

	/**
	 * @brief 指定されたキーの要素を削除する。
	 * @param[in] key 削除するキー
	 * @return 削除された要素の数 (0または1)。
	 */
	size_type erase(const key_type& key) {
		updated_keys.erase(key);
		return map.erase(key);
	}

	/**
	 * @brief イテレータが指す要素を削除する。
	 * @param[in] pos 削除する要素を指すイテレータ。
	 * @return 削除された要素の次の要素を指すイテレータ。
	 */
	const_iterator erase(const_iterator pos) {
		updated_keys.erase(pos->first);
		return map.erase(pos);
	}

	/**
	 * @brief 範囲 [first, last) の要素を削除する。
	 * @param[in] first 範囲の開始イテレータ
	 * @param[in] last 範囲の終了イテレータ
	 * @return 削除された最後の要素の次の要素を指すイテレータ。
	 */
	const_iterator erase(const_iterator first, const_iterator last) {
		for (auto it = first; it != last; ++it) {
			updated_keys.erase(it->first);
		}
		return map.erase(first, last);
	}

	/**
	 * @brief 全ての要素を削除する。
	 */
	void clear() {
		updated_keys.clear();
		map.clear();
	}

	/**
	 * @brief 別のTrackedMapと内容を交換する。
	 * @param[in, out] other 交換対象のマップ
	 */
	void swap(TrackedMap& other) noexcept(
		std::allocator_traits<allocator_type>::is_always_equal::value
		&& std::is_nothrow_swappable<key_compare>::value)
	{
		using std::swap;
		swap(map, other.map);
		swap(updated_keys, other.updated_keys);
	}

	// --- ルックアップ ---
	/**
	 * @brief 指定されたキーがマップに存在するかを確認する。
	 * @param[in] key 確認するキー
	 * @return キーが存在すればtrue, そうでなければfalse。
	 */
	bool contains(const key_type& key) const {
		return map.find(key) != map.end();
	}

	/**
	 * @brief 指定されたキーを持つ要素の数を返す (0または1)。
	 * @param[in] key 確認するキー
	 * @return 要素の数。
	 */
	size_type count(const key_type& key) const {
		return map.count(key);
	}

	/**
	 * @brief 指定されたキーを持つ要素を検索する。
	 * @param[in] key 検索するキー
	 * @return 要素を指すイテレータ。見つからなければend()。
	 */
	const_iterator find(const key_type& key) const {
		return map.find(key);
	}

	/**
	 * @brief 指定されたキーと等価なキーの範囲を返す。
	 * @param[in] key 検索するキー
	 * @return 等価なキーの範囲を示すイテレータのペア。
	 */
	std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
		return map.equal_range(key);
	}

	/**
	 * @brief 指定されたキーより小さくない最初の要素を指すイテレータを返す。
	 * @param[in] key キー
	 * @return 指定されたキー以上の最初の要素を指すイテレータ。
	 */
	const_iterator lower_bound(const key_type& key) const {
		return map.lower_bound(key);
	}

	/**
	 * @brief 指定されたキーより大きい最初の要素を指すイテレータを返す。
	 * @param[in] key キー
	 * @return 指定されたキーより大きい最初の要素を指すイテレータ。
	 */
	const_iterator upper_bound(const key_type& key) const {
		return map.upper_bound(key);
	}

	// --- 容量 ---
	/**
	 * @brief マップが空かどうかを確認する。
	 * @return 空の場合はtrue。
	 */
	bool empty() const {
		return map.empty();
	}

	/**
	 * @brief マップの要素数を取得する。
	 * @return 要素数。
	 */
	size_type size() const {
		return map.size();
	}

	// --- オブザーバ ---
	/**
	 * @brief キー比較オブジェクトを取得する。
	 * @return キー比較オブジェクト。
	 */
	key_compare key_comp() const {
		return map.key_comp();
	}

	/**
	 * @brief 値比較オブジェクト (value_type用) を取得する。
	 * @return 値比較オブジェクト。
	 */
	typename map_type::value_compare value_comp() const {
		return map.value_comp();
	}
};

// --- (グローバル) swap ---
/**
 * @brief 2つのTrackedMapの内容を交換する。
 * @tparam _Key キーの型
 * @tparam _Tp 値の型
 * @tparam _Compare キーの比較関数の型
 * @tparam _Alloc アロケータの型
 * @param[in, out] lhs 交換対象の左側のマップ
 * @param[in, out] rhs 交換対象の右側のマップ
 */
template <typename _Key, typename _Tp, typename _Compare, typename _Alloc>
void swap(TrackedMap<_Key, _Tp, _Compare, _Alloc>& lhs,
		TrackedMap<_Key, _Tp, _Compare, _Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
	lhs.swap(rhs);
}


/**
 * @class SafeTrackedMap
 * @brief 更新されたキーを追跡する機能を持つ、スレッドセーフなマップクラス。
 * @details
 * 内部でstd::mutexを用いて排他制御を行うことで、複数のスレッドからの同時アクセスを安全に処理します。
 * 要素が追加または更新されると、そのキーが内部のセットに記録されます。
 * さらに、std::condition_variableを利用して、データが更新されるまで待機する機能も提供します。
 *
 * @note スレッドセーフ性を担保するため、イテレータを返す関数 (begin, end, findなど) や
 * 参照を返す operator[] は意図的に実装していません。
 * 要素の読み取りは at() (コピーを返す)、
 * 書き込みは insert(), insert_or_assign(), emplace(), try_emplace() を使用してください。
 *
 * @tparam _Key キーの型
 * @tparam _Tp 値の型
 * @tparam _Compare キーの比較関数の型
 * @tparam _Alloc アロケータの型
 */
template <typename _Key, typename _Tp, typename _Compare = std::less<_Key>,
	typename _Alloc = std::allocator<std::pair<const _Key, _Tp> > >
class SafeTrackedMap {
public:
	// --- 型定義 ---
	typedef std::map<_Key, _Tp, _Compare, _Alloc>	map_type;
	typedef typename map_type::key_type				key_type;
	typedef typename map_type::mapped_type			mapped_type;
	typedef typename map_type::value_type			value_type;
	typedef typename map_type::key_compare			key_compare;
	typedef typename map_type::allocator_type		allocator_type;
	typedef typename map_type::size_type			size_type;
	// iterator 型は、内部でのみ使用 (e.g., insert結果のfirst)
	typedef typename map_type::iterator				iterator;

	typedef std::set<key_type, key_compare, typename std::allocator_traits<allocator_type>::template rebind_alloc<key_type>>	set_type;
	typedef typename set_type::key_type				set_key_type;

private:
	// --- メンバ変数 ---
	/// @brief 主要なデータを格納する内部マップ
	map_type map;
	/// @brief 排他制御用のミューテックス (再帰ロックを許可)
	mutable std::recursive_mutex mtx;
	/// @brief データ更新を待機するスレッドに通知するための条件変数
	std::condition_variable cv;
	/// @brief insertやinsert_or_assignで更新されたキーを格納するセット
	set_type updated_keys;

	// --- 内部ヘルパー関数 (ロック取得済み前提) ---
	/**
	 * @brief データが更新されるまで待機するプライベート関数。
	 * @details
	 * この関数は、updated_keysセットが空の場合に呼び出され、
	 * 他のスレッドによってデータが更新され通知（notify）が来るまで、ロックを解放して待機します。
	 * @param[in, out] lock 待機中にアンロック/再ロックされるユニークロック。
	 */
	void _wait_for_updates(std::unique_lock<std::recursive_mutex>& lock) {
		cv.wait(lock, [this] { return !updated_keys.empty(); }); // updated_keysがセットされるまで待機
	}

	/**
	 * @brief flagがTRUEならupdated_keysをクリアする。
	 * @param[in] flag_clear_updated_keys updated_keysをクリアするかどうかのフラグ。
	 * @return flag_clear_updated_keysの値をそのまま返す。
	 */
	bool _if_true_clear_updated_keys(bool flag_clear_updated_keys) {
		if(flag_clear_updated_keys){
			updated_keys.clear();
		}
		return flag_clear_updated_keys;
	}

	/**
	 * @brief flagがTRUEならupdated_keysからkeyをeraseする。
	 * @param[in] key 削除するキー。
	 * @param[in] flag_erase_updated_keys 削除するかどうかのフラグ。
	 * @return flag_erase_updated_keysの値をそのまま返す。
	 */
	bool _if_true_erase_updated_keys(const set_key_type& key, bool flag_erase_updated_keys) {
		if(flag_erase_updated_keys){
			updated_keys.erase(key);
		}
		return flag_erase_updated_keys;
	}

	/**
	 * @brief flagがTRUEならupdated_keysが発生するまで待機する。
	 * @param[in, out] lock 待機中にアンロック/再ロックされるユニークロック。
	 * @param[in] flag_wait_for_updates 待機するかどうかのフラグ。
	 * @return flag_wait_for_updatesの値をそのまま返す。
	 */
	bool _if_true_wait_for_updates(std::unique_lock<std::recursive_mutex> &lock, bool flag_wait_for_updates) {
		if(flag_wait_for_updates){
			_wait_for_updates(lock);
		}
		return flag_wait_for_updates;
	}

	/**
	 * @brief 全ての要素のコピーを取得する。
	 * @param[in] flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return マップ全体のコピー。
	 */
	map_type _get_all(bool flag_clear_updated_keys) {
		map_type copy = map;
		_if_true_clear_updated_keys(flag_clear_updated_keys);	// updated_keysをクリア
		return copy;
	}

	/**
	 * @brief 更新されたキーのリストを取得する。
	 * @details
	 * 現在 `updated_keys` に記録されている全てのキーのコピーを返します。
	 * @param[in] flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return 更新されたキーのセットのコピー。
	 */
	set_type _get_updated_keys(bool flag_clear_updated_keys) {
		set_type copy = updated_keys;
		_if_true_clear_updated_keys(flag_clear_updated_keys);	// updated_keysをクリア
		return copy;
	}

	/**
	 * @brief 更新された要素（キーと値のペア）のマップを取得する。
	 * @details
	 * `updated_keys` に記録されているキーに対応する要素を全て抽出し、新しいマップとして返します。
	 * @param[in] flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return 更新された要素のみを含む新しいマップ。
	 */
	map_type _get_updated_elements(bool flag_clear_updated_keys) {
		map_type updated_elements;
		for (const auto& key : updated_keys) {
			auto it = map.find(key);
			if (it != map.end()) { // mapに存在する場合のみ追加
				updated_elements[key] = it->second;
			}
		}
		_if_true_clear_updated_keys(flag_clear_updated_keys);	// updated_keysをクリア
		return updated_elements;
	}

public:
	/// @brief コンストラクタ
	SafeTrackedMap() = default;

	// --- 手動ロック制御 ---

	/**
	 * @brief 複雑な操作のために、マップのロックを手動で取得する。
	 * @details
	 * この関数は std::unique_lock を返します。
	 * このロックオブジェクトが生存している間、マップはロックされます。
	 * スコープを抜けると自動的にロックが解放されます (RAII)。
	 * * mutexが recursive_mutex のため、ロック取得中に他のメンバ関数
	 * (例: contains, at, insert) を呼び出してもデッドロックしません。
	 *
	 * @return マップのミューテックスに対する std::unique_lock
	 *
	 * ```cpp
	 * SafeTrackedMap<int, std::string> myMap;
	 * // ...
	 * {
	 * auto lock = myMap.acquire_lock(); // ロック取得
	 * // 複数の操作をアトミックに実行
	 * if (myMap.contains(1)) {
	 * auto val = myMap.at(1);
	 * myMap.insert_or_assign(2, val + "_new");
	 * }
	 * } // スコープを抜けると 'lock' が破棄され、自動でロック解放
	 * ```
	 */
	std::unique_lock<std::recursive_mutex> acquire_lock() {
		return std::unique_lock<std::recursive_mutex>(mtx);
	}

/**
	 * @brief マップを読み取り専用でロックし、指定された処理を実行する。
	 * @details
	 * イテレータを外部に漏らさず、安全に内部マップを走査するために使用します。
	 * この関数に渡されたラムダ式が実行されている間、マップはロックされます。
	 * @tparam Func 呼び出し可能なオブジェクト (例: ラムダ式)
	 * @param[in] f 内部の const マップ (const map_type&) を引数に取る関数
	 */
	template <typename Func>
	void inspect_safely(Func f) {
		std::lock_guard<std::recursive_mutex> lock(mtx);
		// ロック中に、内部の const map を引数としてラムダを実行
		f(static_cast<const map_type&>(map));
	}

	/**
	 * @brief マップをロックし、安全な読み取り・書き込み操作をアトミックに実行する。
	 * @details
	 * この関数に渡されたラムダ式が実行されている間、マップはロックされます。
	 * ラムダは2つの引数を取ります:
	 * 1. const map_type& (internal_map_const_ref):
	 * 内部マップへの *const* 参照。const_iterator を使った
	 * 効率的な走査 (lower_bound, find など) のために使用します。
	 * 2. SafeTrackedMap& (self):
	 * マップインスタンス自身への参照。*書き込み*操作
	 * (insert_or_assign, erase など) を安全に呼び出すために使用します。
	 * これにより、updated_keys の追跡が保証されます。
	 *
	 * @note internal_map_const_ref を使って値を変更することはできません。
	 * 変更は必ず self.insert_or_assign(...) などを使用してください。
	 *
	 * @tparam Func 呼び出し可能なオブジェクト (ラムダ式)
	 * @param[in] f ラムダ式 ( (const map_type&, SafeTrackedMap&) -> void )
	 *
	 * ```cpp
	 * myMap.operate_safely( [](const auto& map_const_ref, auto& self) {
	 * // 効率的な走査 (const_iterator を使用)
	 * auto it = map_const_ref.lower_bound(10);
	 * if (it != map_const_ref.end() && it->second == "OldValue") {
	 * // 安全な書き込み (updated_keys が追跡される)
	 * self.insert_or_assign(it->first, "NewValue");
	 * }
	 * });
	 * ```
	 */
	template <typename Func>
	void operate_safely(Func f) {
		std::lock_guard<std::recursive_mutex> lock(mtx);
		// const参照(走査用) と non-constな自分自身(書き込み用) を渡す
		f(static_cast<const map_type&>(map), *this);
	}

	/**
	 * @brief いずれかの要素に更新が発生するまで待機する。
	 * @details 他のスレッドが要素を挿入/更新するまで、この関数を呼び出したスレッドをブロックします。
	 */
	void wait_for_updates() {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		_wait_for_updates(lock);
	}

	/**
	 * @brief 指定されたキーが更新されるまで待機する。(単一キー)
	 * @param[in] key 待機対象のキー
	 */
	void wait_for_updates(const key_type& key) {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		// 指定されたキーが updated_keys セットに見つかるまで待機
		cv.wait(lock, [this, &key] {
			return updated_keys.find(key) != updated_keys.end();
		});
	}

	/**
	 * @brief 指定されたキーのいずれかが更新されるまで待機する。(複数キー)
	 * @details initializer_list (例: {1, 2, 3}) でキーを指定します。
	 * @param[in] keys 待機対象のキーのリスト
	 */
	void wait_for_updates(std::initializer_list<key_type> keys) {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		// keys リスト内のどれか1つでも updated_keys に見つかるまで待機
		cv.wait(lock, [this, &keys] {
			for (const auto& key : keys) {
				if (updated_keys.find(key) != updated_keys.end()) {
					return true; // いずれか1つが見つかれば待機終了
				}
			}
			return false; // 1つも見つからない場合は待機継続
		});
	}

	/**
	 * @brief 指定されたキーのいずれかが更新されるまで待機する。(複数キー、コンテナ版)
	 * @details std::vector, std::set, std::list などイテレート可能なコンテナでキーを指定します。
	 * @tparam KeyContainer キーのコンテナの型
	 * @param[in] keys 待機対象のキーのコンテナ
	 */
	template <typename KeyContainer>
	void wait_for_updates(const KeyContainer& keys) {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		// keys コンテナ内のどれか1つでも updated_keys に見つかるまで待機
		cv.wait(lock, [this, &keys] {
			for (const auto& key : keys) {
				if (updated_keys.find(key) != updated_keys.end()) {
					return true; // いずれか1つが見つかれば待機終了
				}
			}
			return false; // 1つも見つからない場合は待機継続
		});
	}

	// --- 要素アクセス ---
	/**
	 * @brief 指定されたキーに対応する値を取得する。
	 * @param[in] key キー
	 * @param[in] flag_wait_for_updates (オプション) trueの場合、キーが更新されるまで待機する。
	 * @param[in] flag_erase_updated_keys (オプション) trueの場合、取得したキーを更新済みリストから削除する。
	 * @return キーに対応する値 (コピー)。
	 * @throws std::out_of_range キーが存在しない場合。
	 */
	mapped_type at(const key_type& key, bool flag_wait_for_updates = false, bool flag_erase_updated_keys = false) {
		std::unique_lock<std::recursive_mutex> lock(mtx);	// ロックを取得
		if(flag_wait_for_updates){
			cv.wait(lock, [this, &key] { return updated_keys.find(key) != updated_keys.end(); }); // keyがセットされるまで待機
		}
		mapped_type value = map.at(key); // コピーを取得
		_if_true_erase_updated_keys(key, flag_erase_updated_keys);	// 更新情報を解除
		return value; // ロックはunique_lockのデストラクタで解放
	}

	// --- ゲッター (SafeTrackedMap固有) ---
	/**
	 * @brief 全ての要素のコピーを取得する。
	 * @param[in] flag_wait_for_updates (オプション) trueの場合、更新キーが発生するまで待機する。
	 * @param[in] flag_clear_updated_keys (オプション) trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return マップ全体のコピー。
	 */
	map_type get_all(bool flag_wait_for_updates = false, bool flag_clear_updated_keys = false) {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		_if_true_wait_for_updates(lock, flag_wait_for_updates);
		return _get_all(flag_clear_updated_keys);
	}

	/**
	 * @brief 更新されたキーのリストを取得する。
	 * @details
	 * 現在 `updated_keys` に記録されている全てのキーのコピーを返します。
	 * @param[in] flag_wait_for_updates (オプション) trueの場合、更新キーが発生するまで待機する。
	 * @param[in] flag_clear_updated_keys (オプション) trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return 更新されたキーのセットのコピー。
	 */
	set_type get_updated_keys(bool flag_wait_for_updates = false, bool flag_clear_updated_keys = false) {
		std::unique_lock<std::recursive_mutex> lock(mtx);	// ロックを取得
		_if_true_wait_for_updates(lock, flag_wait_for_updates);
		return _get_updated_keys(flag_clear_updated_keys);	// ロックはunique_lockのデストラクタで解放
	}

	/**
	 * @brief 更新された要素（キーと値のペア）のマップを取得する。
	 * @details
	 * `updated_keys` に記録されているキーに対応する要素を全て抽出し、新しいマップとして返します。
	 * @param[in] flag_wait_for_updates (オプション) trueの場合、更新キーが発生するまで待機する。
	 * @param[in] flag_clear_updated_keys (オプション) trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return 更新された要素のみを含む新しいマップ。
	 */
	map_type get_updated_elements(bool flag_wait_for_updates = false, bool flag_clear_updated_keys = false) {
		std::unique_lock<std::recursive_mutex> lock(mtx);	// ロックを取得
		_if_true_wait_for_updates(lock, flag_wait_for_updates);
		return _get_updated_elements(flag_clear_updated_keys);	// ロックはunique_lockのデストラクタで解放
	}

	/**
	 * @brief 更新済みキーのリストをクリアする。
	 */
	void clear_updated_keys() {
		std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
		updated_keys.clear();	// ロックはlock_guardのデストラクタで解放
	}

	// --- 修飾子 ---
	/**
	 * @brief マップに要素を挿入する。
	 * @details
	 * 挿入が成功した場合、そのキーを `updated_keys` に追加し、待機中のスレッドに通知します。
	 * @tparam P 挿入する値の型 (例: std::pair<const _Key, _Tp>)
	 * @param[in] x 挿入する値。左辺値ならコピー、右辺値ならムーブされます。
	 * @return 挿入に成功した場合はtrue、キーが既に存在した場合はfalse。
	 */
	template <class P>
	bool insert(P&& x) {
		bool inserted = false;
		std::pair<iterator, bool> result;
		{
			std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
			result = map.insert(std::forward<P>(x));
			if(result.second){
				updated_keys.insert(result.first->first);	// 更新されたキーを保存
				inserted = true;
			}
		} // ロックを解放
		if(inserted){
			cv.notify_all();	// 更新を待機しているスレッドに通知
		}
		return inserted;
	}

	/**
	 * @brief マップに要素を挿入または代入する。
	 * @details
	 * 挿入・代入どちらの場合でもキーは `updated_keys` に追加し、待機中のスレッドに通知します。
	 * @tparam M 挿入/代入する値の型
	 * @param[in] key キー
	 * @param[in] x 値
	 * @return 新規に挿入された場合はtrue、代入された場合はfalse。
	 */
	template <class M>
	bool insert_or_assign(const key_type& key, M&& x) {
		bool inserted = false;
		std::pair<iterator, bool> result;
		{
			std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
			result = map.insert_or_assign(key, std::forward<M>(x));
			updated_keys.insert(key);  // 更新されたキーを保存
			inserted = result.second;
		} // ロックを解放
		cv.notify_all();	// 更新を待機しているスレッドに通知
		return inserted;
	}

	/**
	 * @brief 要素をインプレース構築する。
	 * @param[in] args 値の構築引数
	 * @return 挿入が成功した場合はtrue、キーが既に存在した場合はfalse。
	 */
	template <class... Args>
	bool emplace(Args&&... args) {
		std::pair<iterator, bool> result;
		bool inserted = false;
		{
			std::lock_guard<std::recursive_mutex> lock(mtx);
			result = map.emplace(std::forward<Args>(args)...);
			if (result.second) {
				updated_keys.insert(result.first->first);
				inserted = true;
			}
		}
		if (inserted) {
			cv.notify_all();
		}
		return inserted;
	}

	/**
	 * @brief キーが存在しない場合のみ要素をインプレース構築する (C++17)。
	 * @param[in] key キー
	 * @param[in] args 値の構築引数
	 * @return 挿入が成功した場合はtrue、キーが既に存在した場合はfalse。
	 */
	template <class... Args>
	bool try_emplace(const key_type& key, Args&&... args) {
		std::pair<iterator, bool> result;
		bool inserted = false;
		{
			std::lock_guard<std::recursive_mutex> lock(mtx);
			result = map.try_emplace(key, std::forward<Args>(args)...);
			if (result.second) {
				updated_keys.insert(result.first->first);
				inserted = true;
			}
		}
		if (inserted) {
			cv.notify_all();
		}
		return inserted;
	}

	/**
	 * @brief キーが存在しない場合のみ要素をインプレース構築する (C++17)。
	 * @param[in] key キー (ムーブされます)
	 * @param[in] args 値の構築引数
	 * @return 挿入が成功した場合はtrue、キーが既に存在した場合はfalse。
	 */
	template <class... Args>
	bool try_emplace(key_type&& key, Args&&... args) {
		std::pair<iterator, bool> result;
		bool inserted = false;
		{
			std::lock_guard<std::recursive_mutex> lock(mtx);
			// key はムーブされる可能性があるため、result.first->first を使用
			result = map.try_emplace(std::move(key), std::forward<Args>(args)...);
			if (result.second) {
				updated_keys.insert(result.first->first);
				inserted = true;
			}
		}
		if (inserted) {
			cv.notify_all();
		}
		return inserted;
	}

	/**
	 * @brief 指定されたキーの要素を削除する。
	 * @param[in] key 削除するキー
	 * @return 削除された要素の数 (0または1)。
	 */
	size_type erase(const key_type& key) {
		std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
		updated_keys.erase(key);	// 更新リストからも削除
		return map.erase(key);	// ロックはlock_guardのデストラクタで解放
	}

	/**
	 * @brief 全ての要素を削除する。
	 */
	void clear() {
		std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
		updated_keys.clear();
		map.clear();	// ロックはlock_guardのデストラクタで解放
	}

	/**
	 * @brief 別のSafeTrackedMapと内容を交換する。
	 * @details std::lockを使いデッドロックセーフに両方のmutexをロックしてswapする。
	 * @param[in, out] other 交換対象のマップ
	 */
	void swap(SafeTrackedMap& other) noexcept(
		std::allocator_traits<allocator_type>::is_always_equal::value
		&& std::is_nothrow_swappable<key_compare>::value)
	{
		if (this == &other) {
			return;
		}

		// デッドロックを回避するために std::lock を使用
		std::lock(mtx, other.mtx);

		// ロックが取得されたことを保証するために std::lock_guard を adopt モードで使用
		std::lock_guard<std::recursive_mutex> lock_this(mtx, std::adopt_lock);
		std::lock_guard<std::recursive_mutex> lock_other(other.mtx, std::adopt_lock);

		using std::swap;
		swap(map, other.map);
		swap(updated_keys, other.updated_keys);

		// 両方のマップが更新された可能性があるため、両方の待機スレッドに通知
		cv.notify_all();
		other.cv.notify_all();
	}

	// --- ルックアップ ---
	/**
	 * @brief 指定されたキーがマップに存在するかを確認する。
	 * @param[in] key 確認するキー
	 * @return キーが存在すればtrue, そうでなければfalse。
	 */
	bool contains(const key_type& key) {
		std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
		return map.find(key) != map.end();	// ロックはlock_guardのデストラクタで解放
	}

	/**
	 * @brief 指定されたキーを持つ要素の数を返す (0または1)。
	 * @param[in] key 確認するキー
	 * @return 要素の数。
	 */
	size_type count(const key_type& key) {
		std::lock_guard<std::recursive_mutex> lock(mtx);
		return map.count(key);
	}

	/**
	 * @brief 指定されたキーと等価なキーの範囲を返す。
	 * @details
	 * std::map (非multimap) のため、範囲には最大1つの要素が含まれます。
	 * @param[in] key キー
	 * @return lower_boundの結果とupper_boundの結果のペア (コピー)。
	 */
	std::pair<std::optional<value_type>, std::optional<value_type>> equal_range(const key_type& key) {
		std::lock_guard<std::recursive_mutex> lock(mtx);
		auto range = map.equal_range(key);

		std::optional<value_type> first;
		if (range.first != map.end()) {
			first = *(range.first);
		}

		std::optional<value_type> second;
		if (range.second != map.end()) {
			second = *(range.second);
		}

		return {first, second};
	}

	/**
	 * @brief 指定されたキーより小さくない最初の要素を返す。
	 * @param[in] key キー
	 * @return 見つかった要素のコピー。見つからなければ std::nullopt。
	 */
	std::optional<value_type> lower_bound(const key_type& key) {
		std::lock_guard<std::recursive_mutex> lock(mtx);
		auto it = map.lower_bound(key);
		if (it != map.end()) {
			return *it;
		}
		return std::nullopt;
	}

	/**
	 * @brief 指定されたキーより大きい最初の要素を返す。
	 * @param[in] key キー
	 * @return 見つかった要素のコピー。見つからなければ std::nullopt。
	 */
	std::optional<value_type> upper_bound(const key_type& key) {
		std::lock_guard<std::recursive_mutex> lock(mtx);
		auto it = map.upper_bound(key);
		if (it != map.end()) {
			return *it;
		}
		return std::nullopt;
	}

	// --- 容量 ---
	/**
	 * @brief マップが空かどうかをスレッドセーフに確認する。
	 * @return 空の場合はtrue。
	 */
	bool empty() const {
		std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
		return map.empty();	// ロックはlock_guardのデストラクタで解放
	}

	/**
	 * @brief マップの要素数をスレッドセーフに取得する。
	 * @return 要素数。
	 */
	size_type size() const {
		std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
		return map.size();	// ロックはlock_guardのデストラクタで解放
	}

	// --- オブザーバ ---
	/**
	 * @brief キー比較オブジェクトを取得する。
	 * @return キー比較オブジェクト。
	 */
	key_compare key_comp() const {
		std::lock_guard<std::recursive_mutex> lock(mtx);
		return map.key_comp();
	}

	/**
	 * @brief 値比較オブジェクト (value_type用) を取得する。
	 * @return 値比較オブジェクト。
	 */
	typename map_type::value_compare value_comp() const {
		std::lock_guard<std::recursive_mutex> lock(mtx);
		return map.value_comp();
	}
};

// --- (グローバル) swap ---
/**
 * @brief 2つのSafeTrackedMapの内容を交換する。
 * @tparam _Key キーの型
 * @tparam _Tp 値の型
 * @tparam _Compare キーの比較関数の型
 * @tparam _Alloc アロケータの型
 * @param[in, out] lhs 交換対象の左側のマップ
 * @param[in, out] rhs 交換対象の右側のマップ
 */
template <typename _Key, typename _Tp, typename _Compare, typename _Alloc>
void swap(SafeTrackedMap<_Key, _Tp, _Compare, _Alloc>& lhs,
		SafeTrackedMap<_Key, _Tp, _Compare, _Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
	lhs.swap(rhs);
}


} // namespace utils
