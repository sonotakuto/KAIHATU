#pragma once

#include <condition_variable>
#include <map>
#include <mutex>
#include <set>

namespace utils {

/**
 * @class TrackedMap
 * @brief 更新されたキーを追跡する機能を持つ、シングルスレッド用のマップクラス。
 * @tparam _Key キーの型
 * @tparam _Tp 値の型
 * @tparam _Compare キーの比較関数の型
 * @tparam _Alloc アロケータの型
 */
template <typename _Key, typename _Tp, typename _Compare = std::less<_Key>,
		typename _Alloc = std::allocator<std::pair<const _Key, _Tp>>>
class TrackedMap {
public:
	// 型定義
	typedef std::map<_Key, _Tp, _Compare, _Alloc>	map_type;
	typedef typename map_type::key_type				key_type;
	typedef typename map_type::mapped_type			mapped_type;
	typedef typename map_type::value_type			value_type;
	typedef typename map_type::key_compare			key_compare;
	typedef typename map_type::allocator_type		allocator_type;
	typedef typename map_type::size_type			size_type;
	typedef typename map_type::iterator				iterator;

	typedef std::set<key_type, _Compare, typename std::allocator_traits<_Alloc>::template rebind_alloc<key_type>>	set_type;
	typedef typename set_type::key_type				set_key_type;

private:
	// --- メンバ変数 ---
	/// @brief 内部マップ
	map_type map;
	/// @brief 更新されたキーを保持するセット
	set_type updated_keys;

public:
	// デフォルトコンストラクタ
	TrackedMap() = default;

	/**
	 * @brief マップに要素を挿入する。
	 * @details
	 * 指定された要素をマップに挿入します。挿入が成功した場合（キーが既存でなかった場合）、
	 * そのキーを `updated_keys` に追加し、待機中のスレッドに通知します。
	 * @tparam P 挿入する値の型
	 * @param x 挿入する値 (std::pair)
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
	 * 指定されたキーが存在すれば値を上書きし、存在しなければ新しい要素として挿入します。
	 * どちらの場合でもキーは「更新された」とみなし、`updated_keys` に追加して待機中のスレッドに通知します。
	 * @tparam M 挿入/代入する値の型
	 * @param key キー
	 * @param x 値
	 * @return 新規に挿入された場合はtrue、代入された場合はfalse。
	 */
	template <class M>
	bool insert_or_assign(const key_type& key, M&& x) {
		auto result = map.insert_or_assign(key, std::forward<M>(x));
		updated_keys.insert(key); // キーが更新されたことを記録
		return result.second;
	}

	/**
	 * @brief 指定されたキーに対応する値を取得する。
	 * @param key キー
	 * @param flag_erase_updated_keys trueの場合、取得したキーを更新済みリストから削除する。
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

	/**
	 * @brief 全ての要素のコピーを取得する。
	 * @param flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
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
	 * @param flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
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
	 * @param flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
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
	 * @brief 指定されたキーがマップに存在するかを確認する。
	 * @param key 確認するキー
	 * @return キーが存在すればtrue, そうでなければfalse。
	 */
	bool contains(const key_type& key) const {
		return map.find(key) != map.end();
	}

	/**
	 * @brief 更新済みキーのリストをクリアする。
	 */
	void clear_updated_keys() {
		updated_keys.clear();
	}

	/**
	 * @brief 指定されたキーの要素を削除する。
	 * @param key 削除するキー
	 * @return 削除された要素の数 (0または1)。
	 */
	size_type erase(const key_type& key) {
		updated_keys.erase(key);
		return map.erase(key);
	}

	/**
	 * @brief 全ての要素を削除する。
	 */
	void clear() {
		updated_keys.clear();
		map.clear();
	}

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
};

/**
 * @class SafeTrackedMap
 * @brief 更新されたキーを追跡する機能を持つ、スレッドセーフなマップクラス。
 * @details
 * 内部でstd::mutexを用いて排他制御を行うことで、複数のスレッドからの同時アクセスを安全に処理します。
 * 要素が追加または更新されると、そのキーが内部のセットに記録されます。
 * さらに、std::condition_variableを利用して、データが更新されるまで待機する機能も提供します。
 * @tparam _Key キーの型
 * @tparam _Tp 値の型
 * @tparam _Compare キーの比較関数の型
 * @tparam _Alloc アロケータの型
 */
template <typename _Key, typename _Tp, typename _Compare = std::less<_Key>,
	typename _Alloc = std::allocator<std::pair<const _Key, _Tp> > >
class SafeTrackedMap {
public:
	typedef std::map<_Key, _Tp, _Compare, _Alloc>	map_type;
	typedef typename map_type::key_type			key_type;
	typedef typename map_type::mapped_type		mapped_type;
	typedef typename map_type::value_type		value_type;
	typedef typename map_type::key_compare		key_compare;
	typedef typename map_type::allocator_type	allocator_type;
	typedef typename map_type::size_type		size_type;
	typedef typename map_type::iterator			iterator;

	typedef std::set<key_type, key_compare, typename std::allocator_traits<allocator_type>::template rebind_alloc<key_type>>	set_type;
	typedef typename set_type::key_type			set_key_type;

private:
	// --- メンバ変数 ---
	/// @brief 主要なデータを格納する内部マップ
	map_type map;
	/// @brief 排他制御用のミューテックス
	mutable std::mutex mtx;
	/// @brief データ更新を待機するスレッドに通知するための条件変数
	std::condition_variable cv;
	/// @brief insertやinsert_or_assignで更新されたキーを格納するセット
	set_type updated_keys;

	/**
	 * @brief データが更新されるまで待機するプライベート関数。
	 * @details
	 * この関数は、updated_keysセットが空の場合に呼び出され、
	 * 他のスレッドによってデータが更新され通知（notify）が来るまで、ロックを解放して待機します。
	 * @param lock 待機中にアンロック/再ロックされるユニークロック。
	 */
	void _wait_for_updates(std::unique_lock<std::mutex>& lock) {
		cv.wait(lock, [this] { return !updated_keys.empty(); }); // updated_keysがセットされるまで待機
	}

	/**
	 * @brief flagがTRUEならupdated_keysをクリアする。
	 * @param flag_clear_updated_keys updated_keysをクリアするかどうかのフラグ。
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
	 * @param key 削除するキー。
	 * @param flag_erase_updated_keys 削除するかどうかのフラグ。
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
	 * @param lock 待機中にアンロック/再ロックされるユニークロック。
	 * @param flag_wait_for_updates 待機するかどうかのフラグ。
	 * @return flag_wait_for_updatesの値をそのまま返す。
	 */
	bool _if_true_wait_for_updates(std::unique_lock<std::mutex> &lock, bool flag_wait_for_updates) {
		if(flag_wait_for_updates){
			_wait_for_updates(lock);
		}
		return flag_wait_for_updates;
	}

	map_type _get_all(bool flag_clear_updated_keys) {
		map_type copy = map;
		_if_true_clear_updated_keys(flag_clear_updated_keys);	// updated_keysをクリア
		return copy;
	}

	// updated_keysのコピーを取得する関数
	set_type _get_updated_keys(bool flag_clear_updated_keys) {
		set_type copy = updated_keys;
		_if_true_clear_updated_keys(flag_clear_updated_keys);	// updated_keysをクリア
		return copy;
	}

	// updateされた全てのキーについての要素一覧を取得
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

	/**
	 * @brief 更新が発生するまで待機する。
	 * @details 他のスレッドが要素を挿入/更新するまで、この関数を呼び出したスレッドをブロックします。
	 */
	void wait_for_updates() {
		std::unique_lock<std::mutex> lock(mtx);
		_wait_for_updates(lock);
	}

	/**
	 * @brief マップに要素を挿入する。
	 * @details
	 * 指定された要素をマップに挿入します。挿入が成功した場合（キーが既存でなかった場合）、
	 * そのキーを `updated_keys` に追加し、待機中のスレッドに通知します。
	 * @tparam P 挿入する値の型 (例: std::pair<const _Key, _Tp>)
	 * @param x 挿入する値
	 * @return 挿入に成功した場合はtrue、キーが既に存在した場合はfalse。
	 */
	template <class P>
	bool insert(P&& x) {
		bool inserted = false;
		std::pair<iterator, bool> result;
		{
			std::lock_guard<std::mutex> lock(mtx);	// ロックを取得
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
	 * 指定されたキーが存在すれば値を上書きし、存在しなければ新しい要素として挿入します。
	 * どちらの場合でもキーは「更新された」とみなし、`updated_keys` に追加して待機中のスレッドに通知します。
	 * @tparam M 挿入/代入する値の型
	 * @param key キー
	 * @param x 値
	 * @return 新規に挿入された場合はtrue、代入された場合はfalse。
	 */
	template <class M>
	bool insert_or_assign(const key_type& key, M&& x) {
		bool inserted = false;
		std::pair<iterator, bool> result;
		{
			std::lock_guard<std::mutex> lock(mtx);	// ロックを取得
			result = map.insert_or_assign(key, std::forward<M>(x));
			updated_keys.insert(key);  // 更新されたキーを保存
			inserted = result.second;
		} // ロックを解放
		cv.notify_all();	// 更新を待機しているスレッドに通知
		return inserted;
	}

	/**
	 * @brief 指定されたキーに対応する値を取得する。
	 * @param key キー
	 * @param flag_wait_for_updates trueの場合、キーが更新されるまで待機する。
	 * @param flag_erase_updated_keys trueの場合、取得したキーを更新済みリストから削除する。
	 * @return キーに対応する値。
	 * @throws std::out_of_range キーが存在しない場合。
	 */
	mapped_type at(const key_type& key, bool flag_wait_for_updates = false, bool flag_erase_updated_keys = false) {
		std::unique_lock<std::mutex> lock(mtx);	// ロックを取得
		if(flag_wait_for_updates){
			cv.wait(lock, [this, &key] { return updated_keys.find(key) != updated_keys.end(); }); // keyがセットされるまで待機
		}
		mapped_type value = map.at(key);
		_if_true_erase_updated_keys(key, flag_erase_updated_keys);	// 更新情報を解除
		return value; // ロックはunique_lockのデストラクタで解放
	}

	/**
	 * @brief 全ての要素のコピーを取得する。
	 * @param flag_wait_for_updates trueの場合、更新キーが発生するまで待機する。
	 * @param flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return マップ全体のコピー。
	 */
	map_type get_all(bool flag_wait_for_updates = false, bool flag_clear_updated_keys = false) {
		std::unique_lock<std::mutex> lock(mtx);
		_if_true_wait_for_updates(lock, flag_wait_for_updates);
		return _get_all(flag_clear_updated_keys);
	}

	/**
	 * @brief 更新されたキーのリストを取得する。
	 * @details
	 * 現在 `updated_keys` に記録されている全てのキーのコピーを返します。
	 * @param flag_wait_for_updates trueの場合、更新キーが発生するまで待機する。
	 * @param flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return 更新されたキーのセットのコピー。
	 */
	set_type get_updated_keys(bool flag_wait_for_updates = false, bool flag_clear_updated_keys = false) {
		std::unique_lock<std::mutex> lock(mtx);	// ロックを取得
		_if_true_wait_for_updates(lock, flag_wait_for_updates);
		return _get_updated_keys(flag_clear_updated_keys);	// ロックはunique_lockのデストラクタで解放
	}

	/**
	 * @brief 更新された要素（キーと値のペア）のマップを取得する。
	 * @details
	 * `updated_keys` に記録されているキーに対応する要素を全て抽出し、新しいマップとして返します。
	 * @param flag_wait_for_updates trueの場合、更新キーが発生するまで待機する。
	 * @param flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return 更新された要素のみを含む新しいマップ。
	 */
	map_type get_updated_elements(bool flag_wait_for_updates = false, bool flag_clear_updated_keys = false) {
		std::unique_lock<std::mutex> lock(mtx);	// ロックを取得
		_if_true_wait_for_updates(lock, flag_wait_for_updates);
		return _get_updated_elements(flag_clear_updated_keys);	// ロックはunique_lockのデストラクタで解放
	}

	/**
	 * @brief 指定されたキーがマップに存在するかを確認する。
	 * @param key 確認するキー
	 * @return キーが存在すればtrue, そうでなければfalse。
	 */
	bool contains(const key_type& key) {
		std::lock_guard<std::mutex> lock(mtx);	// ロックを取得
		return map.find(key) != map.end();	// ロックはlock_guardのデストラクタで解放
	}

	/**
	 * @brief 更新済みキーのリストをクリアする。
	 */
	void clear_updated_keys() {
		std::lock_guard<std::mutex> lock(mtx);	// ロックを取得
		updated_keys.clear();	// ロックはlock_guardのデストラクタで解放
	}

	/**
	 * @brief 指定されたキーの要素を削除する。
	 * @param key 削除するキー
	 * @return 削除された要素の数 (0または1)。
	 */
	size_type erase(const key_type& key) {
		std::lock_guard<std::mutex> lock(mtx);	// ロックを取得
		updated_keys.erase(key);	// 更新リストからも削除
		return map.erase(key);	// ロックはlock_guardのデストラクタで解放
	}

	/**
	 * @brief 全ての要素を削除する。
	 */
	void clear() {
		std::lock_guard<std::mutex> lock(mtx);	// ロックを取得
		updated_keys.clear();
		map.clear();	// ロックはlock_guardのデストラクタで解放
	}

	/**
	 * @brief マップが空かどうかをスレッドセーフに確認する。
	 * @return 空の場合はtrue。
	 */
	bool empty() const {
		std::lock_guard<std::mutex> lock(mtx);	// ロックを取得
		return map.empty();	// ロックはlock_guardのデストラクタで解放
	}

	/**
	 * @brief マップの要素数をスレッドセーフに取得する。
	 * @return 要素数。
	 */
	size_type size() const {
		std::lock_guard<std::mutex> lock(mtx);	// ロックを取得
		return map.size();	// ロックはlock_guardのデストラクタで解放
	}
};

} // namespace utils