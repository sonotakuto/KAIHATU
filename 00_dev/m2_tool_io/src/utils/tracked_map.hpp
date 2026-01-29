/**
 * @file tracked_map.hpp
 * @brief 更新されたキーを追跡する機能を持つマップクラスを提供するヘッダファイル。
 */
#pragma once

#include <condition_variable>
#include <initializer_list>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <type_traits>
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
 * @tparam Key キーの型
 * @tparam Tp 値の型
 * @tparam Compare キーの比較関数の型
 * @tparam Alloc アロケータの型
 */
template <typename Key, typename Tp, typename Compare = std::less<Key>,
		typename Alloc = std::allocator<std::pair<const Key, Tp>>>
class TrackedMap {
public:
	// --- 型定義 ---
	using map_type					= std::map<Key, Tp, Compare, Alloc>;
	using key_type					= typename map_type::key_type;
	using mapped_type				= typename map_type::mapped_type;
	using value_type				= typename map_type::value_type;
	using key_compare				= typename map_type::key_compare;
	using allocator_type			= typename map_type::allocator_type;
	using size_type					= typename map_type::size_type;
	using iterator					= typename map_type::iterator;
	using const_iterator			= typename map_type::const_iterator;
	using reverse_iterator			= typename map_type::reverse_iterator;
	using const_reverse_iterator	= typename map_type::const_reverse_iterator;

	using set_type					= std::set<key_type, key_compare, typename std::allocator_traits<Alloc>::template rebind_alloc<key_type>>;
	using set_key_type				= typename set_type::key_type;

private:
	// --- メンバ変数 ---
	/// @brief 内部マップ
	map_type map;
	/// @brief 更新されたキーを保持するセット
	set_type updated_keys;

	// --- 内部ヘルパー関数 ---
	/**
	 * @brief flagがTRUEならupdated_keysをクリアする。
	 * @param[in] flag_clear_updated_keys updated_keysをクリアするかどうかのフラグ。
	 * @return flag_clear_updated_keysの値をそのまま返す。
	 */
	bool if_true_clear_updated_keys(bool flag_clear_updated_keys) {
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
	bool if_true_erase_updated_keys(const set_key_type& key, bool flag_erase_updated_keys) {
		if(flag_erase_updated_keys){
			updated_keys.erase(key);
		}
		return flag_erase_updated_keys;
	}

	/**
	 * @brief 更新された要素（キーと値のペア）のマップを取得する。
	 * @details
	 * `updated_keys` に記録されているキーに対応する要素を全て抽出し、新しいマップとして返します。
	 * @return 更新された要素のみを含む新しいマップ。
	 */
	map_type get_updated_elements_internal() const {
		map_type updated_elements;
		for (const auto& key : updated_keys) {
			auto it = map.find(key);
			if (it != map.end()) {	// mapに存在する場合のみ追加
				// C++17 try_emplace にヒント(end())を渡す
				// mapは昇順、updated_keysも昇順なので、常に末尾に追加される
				updated_elements.try_emplace(updated_elements.end(), key, it->second);
			}
		}
		return updated_elements;
	}

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

	/**
	 * @brief 更新された要素が存在するかを確認する。
	 * @return 更新されたキーが1つ以上あればtrue、なければfalse。
	 */
	bool is_updated() const {
		return !updated_keys.empty();
	}

	/**
	 * @brief 指定されたキーが更新されたかを確認する。
	 * @param[in] key 確認するキー。
	 * @return キーが更新されていればtrue、なければfalse。
	 */
	bool is_updated(const key_type& key) const {
		return updated_keys.find(key) != updated_keys.end();
	}

	/**
	 * @brief 指定されたキー群の中に更新されたキーが存在するかを確認する。
	 * @tparam KeyContainer キーを含むコンテナの型 (std::vector, std::list, std::set, etc.)
	 * @param[in] keys 確認するキー群。
	 * @return 更新されたキーが1つ以上あればtrue、なければfalse。
	 */
	template <typename KeyContainer>
	bool is_updated(const KeyContainer& keys) const {
		for (const auto& key : keys) {
			if (updated_keys.find(key) != updated_keys.end()) {
				return true;
			}
		}
		return false;
	}

	/**
	 * @brief 指定されたキー群の中に更新されたキーが存在するかを確認する。
	 * @param[in] keys 確認するキー群 (initializer_list)。
	 * @return 更新されたキーが1つ以上あればtrue、なければfalse。
	 */
	bool is_updated(std::initializer_list<key_type> keys) const {
		return is_updated<std::initializer_list<key_type>>(keys);
	}

	// --- ルックアップ ---
	/**
	 * @brief 指定されたキーがマップに存在するかを確認する。
	 * @param[in] key 確認するキー
	 * @return キーが存在すればtrue, そうでなければfalse。
	 */
	bool contains(const key_type& key) const {
#if __cplusplus >= 202002L
		// C++20以上: 標準の contains を使用 (意図が明確で最適)
		return map.contains(key);
#else
		// C++17以下: find != end イディオムを使用
		return map.find(key) != map.end();
#endif
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

	// --- 単一要素アクセス ---
	/**
	 * @brief 指定されたキーに対応する値への const 参照を取得する (const版)。
	 * @details
	 * std::map::at に準拠。
	 * 更新フラグの消去はできません。
	 * @param[in] key キー
	 * @return キーに対応する値への const 参照。
	 * @throws std::out_of_range キーが存在しない場合。
	 */
	const mapped_type& at(const key_type& key) const {
		return map.at(key);
	}

	/**
	 * @brief 指定されたキーに対応する値への const 参照を取得する (更新フラグ操作付き)。
	 * @details
	 * 戻り値は const 参照ですが、メソッド自体は内部状態 (updated_keys) を変更する可能性があるため
	 * 非 const メンバー関数として定義します。
	 * @param[in] key キー
	 * @param[in] flag_erase_updated_keys trueの場合、取得したキーを更新済みリストから削除する。
	 * @return キーに対応する値への const 参照。
	 * @throws std::out_of_range キーが存在しない場合。
	 */
	const mapped_type& at(const key_type& key, bool flag_erase_updated_keys) {
		const mapped_type& value = map.at(key); // 参照を取得
		if_true_erase_updated_keys(key, flag_erase_updated_keys);
		return value; // 参照を返す (mapから削除しない限り安全)
	}

	/**
	 * @brief 指定されたキーに対応する値を取得する。キーが存在しない場合はstd::nulloptを返す (const版)。
	 * @details
	 * これは const メンバ関数であり、内部状態を変更しない。
	 * 更新フラグの消去はできません。
	 * @param[in] key キー
	 * @return キーに対応する値、またはstd::nullopt。
	 */
	std::optional<mapped_type> try_at(const key_type& key) const {
		auto it = map.find(key);
		if (it != map.end()) {
			return it->second;
		} else {
			return std::nullopt;
		}
	}

	/**
	 * @brief 指定されたキーに対応する値を取得する (更新フラグ操作付き)。
	 * @details
	 * 戻り値は値のコピーですが、メソッド自体は内部状態 (updated_keys) を変更する可能性があるため
	 * 非 const メンバー関数として定義します。
	 * @param[in] key キー
	 * @param[in] flag_erase_updated_keys trueの場合、取得したキーを更新済みリストから削除する。
	 * @return キーに対応する値、またはstd::nullopt。
	 */
	std::optional<mapped_type> try_at(const key_type& key, bool flag_erase_updated_keys) {
		auto it = map.find(key);
		if (it != map.end()) {
			if_true_erase_updated_keys(key, flag_erase_updated_keys);
			return it->second;
		} else {
			return std::nullopt;
		}
	}

	/**
	 * @brief 指定されたキーに対応する値を取得する。キーが存在しない場合はデフォルト値を返す (const版)。
	 * @details
	 * デフォルト値は左辺値・右辺値の両方を受け入れ、適切にコピーまたはムーブされる。
	 * これは const メンバ関数であり、内部状態を変更しない。
	 * 更新フラグの消去はできません。
	 * @tparam U デフォルト値の型 (mapped_type に変換可能であること)
	 * @param[in] key キー
	 * @param[in] default_value キーが存在しない場合に返すデフォルト値 (Forwarding Reference)
	 * @return キーに対応する値、またはデフォルト値。
	 */
	template <typename U>
	mapped_type at_or(const key_type& key, U&& default_value) const {
		auto it = map.find(key);
		if (it != map.end()) {
			return it->second;
		} else {
			// 左辺値ならコピー、右辺値ならムーブして返す
			return std::forward<U>(default_value);
		}
	}

	/**
	 * @brief 指定されたキーに対応する値を取得する (更新フラグ操作付き)。
	 * @details
	 * デフォルト値は左辺値・右辺値の両方を受け入れ、適切にコピーまたはムーブされる。
	 * 戻り値は値のコピーですが、メソッド自体は内部状態 (updated_keys) を変更する可能性があるため
	 * 非 const メンバー関数として定義します。
	 * @tparam U デフォルト値の型 (mapped_type に変換可能であること)
	 * @param[in] key キー
	 * @param[in] default_value キーが存在しない場合に返すデフォルト値 (Forwarding Reference)
	 * @param[in] flag_erase_updated_keys trueの場合、取得したキーを更新済みリストから削除する。
	 * @return キーに対応する値、またはデフォルト値。
	 */
	template <typename U>
	mapped_type at_or(const key_type& key, U&& default_value, bool flag_erase_updated_keys) {
		auto it = map.find(key);
		if (it != map.end()) {
			if_true_erase_updated_keys(key, flag_erase_updated_keys);
			return it->second;
		} else {
			return std::forward<U>(default_value);
		}
	}

	// --- 一括操作 (Traversals) ---
	// 全要素走査
	/**
	 * @brief 全ての要素に対して関数を適用する (ゼロコピー・読み取り専用・const版)。
	 * @details
	 * マップ内の全要素を走査し、コールバック関数を実行します。
	 * コールバック関数には (const Key&, const Value&) が渡されます。
	 * 値の書き換えはできません。
	 * 更新フラグの消去はできません。
	 * @tparam Func 関数オブジェクト ( void(const key_type&, const mapped_type&) )
	 * @param[in] action 適用する処理
	 */
	template <typename Func>
	void for_each(Func action) const {
		for (const auto& [key, value] : map) {
			action(key, value);
		}
	}

	/**
	 * @brief 全ての要素に対して関数を適用する (ゼロコピー・読み取り専用・const版)。
	 * @details
	 * マップ内の全要素を走査し、コールバック関数を実行します。
	 * コールバックには (const Key&, const Value&) が渡されます。
	 * 値の書き換えはできません。
	 * action にはconst参照で渡されますが、メソッド自体は内部状態 (updated_keys) を変更する可能性があるため
	 * 非 const メンバー関数として定義します。
	 * @tparam Func 関数オブジェクト ( void(const key_type&, const mapped_type&) )
	 * @param[in] action 適用する処理
	 * @param[in] flag_clear_updated_keys (オプション) trueなら処理後に更新履歴をクリア
	 */
	template <typename Func>
	void for_each(Func action, bool flag_clear_updated_keys) {
		for (const auto& [key, value] : map) {
			action(key, value);
		}
		if_true_clear_updated_keys(flag_clear_updated_keys);
	}

	// 更新要素走査
	/**
	 * @brief 更新された全ての要素に対して関数を適用する (ゼロコピー・読み取り専用・const版)。
	 * @details
	 * コールバック関数には (const Key&, const Value&) が渡されます。
	 * 値の書き換えはできません。
	 * 更新フラグの消去はできません。
	 * @tparam Func 関数オブジェクト ( void(const key_type&, const mapped_type&) )
	 * @param[in] action 適用する処理
	 */
	template <typename Func>
	void for_each_updated(Func action) const {
		for (const auto& key : updated_keys) {
			if (auto it = map.find(key); it != map.end()) {
				action(key, static_cast<const mapped_type&>(it->second));
			}
		}
	}

	/**
	 * @brief 更新された全ての要素に対して関数を適用する (ゼロコピー・読み取り専用・更新フラグ操作付き)。
	 * @details
	 * コールバック関数には (const Key&, const Value&) が渡されます。
	 * 値の書き換えはできません。
	 * action にはconst参照で渡されますが、メソッド自体は内部状態 (updated_keys) を変更する可能性があるため
	 * 非 const メンバー関数として定義します。
	 * @tparam Func 関数オブジェクト ( void(const key_type&, const mapped_type&) )
	 * @param[in] action 適用する処理
	 * @param[in] flag_clear_updated_keys trueなら処理後に更新履歴をクリア
	 */
	template <typename Func>
	void for_each_updated(Func action, bool flag_clear_updated_keys) {
		for (const auto& key : updated_keys) {
			if (auto it = map.find(key); it != map.end()) {
				action(key, static_cast<const mapped_type&>(it->second));
			}
		}
		if_true_clear_updated_keys(flag_clear_updated_keys);
	}

	// --- 一括取得 (Getters) ---
	/**
	 * @brief 全ての要素への const 参照を取得する (const版)。
	 * @details
	 * 内部の map への直接参照を返します。
	 * 更新済みキーのリストには影響を与えません。
	 * @return マップ全体への const 参照。
	 */
	const map_type& get_all() const {
		return map;
	}

	/**
	 * @brief 全ての要素への const 参照を取得し、指定があれば更新履歴をクリアする (更新フラグ操作付き)。
	 * @details
	 * 戻り値は const 参照ですが、フラグが true の場合は内部状態 (updated_keys) を変更します。
	 * map 本体と updated_keys は独立しているため、クリア操作を行っても map への参照は安全です。
	 * @param[in] flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return マップ全体への const 参照。
	 */
	const map_type& get_all(bool flag_clear_updated_keys) {
		if_true_clear_updated_keys(flag_clear_updated_keys);
		return map;
	}

	/**
	 * @brief 更新されたキーのセットへの const 参照を取得する (const版)。
	 * @details
	 * 内部のセットへの直接参照を返します。
	 * 更新済みキーのリストには影響を与えません。
	 * @return 更新されたキーのセットへの const 参照。
	 */
	const set_type& get_updated_keys() const {
		return updated_keys;
	}

	/**
	 * @brief 更新されたキーのセットのコピーを取得し、指定があれば更新履歴をクリアする (更新フラグ操作付き)。
	 * @details
	 * 参照返しでは「取得後にクリア」する前の状態を保持できないため、このオーバーロードは常にコピーを返します。
	 * @note 引数に false を渡した場合でもコピーが発生するため、クリア不要な場合は引数なし版を使用してください。
	 * @param[in] flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return 更新されたキーのセットのコピー。
	 */
	set_type get_updated_keys(bool flag_clear_updated_keys) {
		set_type copy = updated_keys; // ここでコピー発生
		if_true_clear_updated_keys(flag_clear_updated_keys);
		return copy;
	}

	/**
	 * @brief 更新された要素（キーと値のペア）のマップを取得する (const版)。
	 * @details
	 * `updated_keys` に記録されているキーに対応する要素を全て抽出し、新しいマップとして返します。
	 * 構造上、新しいマップを生成する必要があるため、常に実体（コピー）を返します。
	 * 更新フラグの消去はできません。
	 * @return 更新された要素のみを含む新しいマップ。
	 */
	map_type get_updated_elements() const {
		return get_updated_elements_internal();
	}

	/**
	 * @brief 更新された要素（キーと値のペア）のマップを取得する (更新フラグ操作付き)。
	 * @details
	 * `updated_keys` に記録されているキーに対応する要素を全て抽出し、新しいマップとして返します。
	 * 構造上、新しいマップを生成する必要があるため、常に実体（コピー）を返します。
	 * 戻り値は値のコピーですが、メソッド自体は内部状態 (updated_keys) を変更する可能性があるため
	 * 非 const メンバー関数として定義します。
	 * @param[in] flag_clear_updated_keys (オプション・デフォルト false) trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return 更新された要素のみを含む新しいマップ。
	 */
	map_type get_updated_elements(bool flag_clear_updated_keys = false) {
		map_type updated_elements = get_updated_elements_internal();
		if_true_clear_updated_keys(flag_clear_updated_keys);
		return updated_elements;
	}

	// --- 修飾子 (Modifiers) ---
	/**
	 * @brief 更新済みキーのリストをクリアする。
	 */
	void clear_updated_keys() {
		updated_keys.clear();
	}

	/**
	 * @brief 要素をその場で変更する。
	 * @details
	 * 指定されたキーに対応する要素が存在する場合、Modifier関数オブジェクトを呼び出して値を変更します。
	 * キー検索は1回のみ、値のコピーは発生しません。
	 *
	 * @note
	 * - Modifierが bool を返す場合: true が返された時のみ更新とみなします (最適化用)。
	 *
	 * - Modifierが void を返す場合: 常に更新されたとみなします (利便性用)。
	 * @tparam Modifier 関数オブジェクト。 bool(mapped_type&) または void(mapped_type&)
	 * @param key キー
	 * @param modifier 変更を行う関数。bool を返す場合は、変更があったときに true を返してください。
	 * @return キーが見つかり、modifierが実行された場合は true。
	 */
	template <typename Modifier>
	bool modify(const key_type& key, Modifier modifier) {
		auto it = map.find(key);
		if (it != map.end()) {
			// ラムダ式の戻り値の型を取得
			using ReturnType = std::invoke_result_t<Modifier, mapped_type&>;

			if constexpr (std::is_same_v<ReturnType, bool>) {
				// --- パターンA: boolを返す (利用者責任) ---
				if (modifier(it->second)) {
					updated_keys.insert(key);
					return true;
				}
			} else {
				// --- パターンB: voidを返す (常に更新扱い) ---
				modifier(it->second);
				updated_keys.insert(key);// 無条件で更新とみなす
				return true;
			}
		}
		return false;
	}

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
	 * @brief 範囲内の要素をマップに挿入する。
	 * @tparam Range 挿入する要素の範囲の型 (begin/endが定義されていること)
	 * @param[in] range 挿入する要素の範囲
	 */
	template <typename Range>
	size_type insert_range(Range&& range) {
		size_type inserted_count = 0;
		for (auto&& item : range) {
			inserted_count += insert(std::forward<decltype(item)>(item));	// 既存の追跡付きinsertに委譲
		}
		return inserted_count;
	}

	/**
	 * @brief 初期化子リストから要素を一括挿入する。
	 * @param[in] list 挿入する要素のリスト
	 * @return 挿入に成功した要素の数。
	 */
	size_type insert_range(std::initializer_list<value_type> list) {
		return insert_range<std::initializer_list<value_type>>(list);
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

	// --- その他のメンバ関数 ---
	/**
	 * @brief 別のTrackedMapと内容を交換する。
	 * @param[in, out] other 交換対象のマップ
	 */
	void swap(TrackedMap& other) noexcept (
		std::allocator_traits<allocator_type>::is_always_equal::value
		&& std::is_nothrow_swappable<key_compare>::value)
	{
		using std::swap;
		swap(map, other.map);
		swap(updated_keys, other.updated_keys);
	}

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
 * @tparam Key キーの型
 * @tparam Tp 値の型
 * @tparam Compare キーの比較関数の型
 * @tparam Alloc アロケータの型
 * @param[in, out] lhs 交換対象の左側のマップ
 * @param[in, out] rhs 交換対象の右側のマップ
 */
template <typename Key, typename Tp, typename Compare, typename Alloc>
void swap(TrackedMap<Key, Tp, Compare, Alloc>& lhs,
		TrackedMap<Key, Tp, Compare, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
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
 * @tparam Key キーの型
 * @tparam Tp 値の型
 * @tparam Compare キーの比較関数の型
 * @tparam Alloc アロケータの型
 */
template <typename Key, typename Tp, typename Compare = std::less<Key>,
	typename Alloc = std::allocator<std::pair<const Key, Tp> > >
class SafeTrackedMap {
public:
	// --- 型定義 ---
	using map_type					= std::map<Key, Tp, Compare, Alloc>;
	using key_type					= typename map_type::key_type;
	using mapped_type				= typename map_type::mapped_type;
	using value_type				= typename map_type::value_type;
	using key_compare				= typename map_type::key_compare;
	using allocator_type			= typename map_type::allocator_type;
	using size_type					= typename map_type::size_type;
	using iterator					= typename map_type::iterator;
	using const_iterator			= typename map_type::const_iterator;
	using reverse_iterator			= typename map_type::reverse_iterator;
	using const_reverse_iterator	= typename map_type::const_reverse_iterator;

	using set_type					= std::set<key_type, key_compare, typename std::allocator_traits<Alloc>::template rebind_alloc<key_type>>;
	using set_key_type				= typename set_type::key_type;

private:
	// --- メンバ変数 ---
	/// @brief 主要なデータを格納する内部マップ
	map_type map;
	/// @brief 排他制御用のミューテックス (再帰ロックを許可)
	mutable std::recursive_mutex mtx;
	/// @brief データ更新を待機するスレッドに通知するための条件変数
	std::condition_variable_any cv;
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
	void wait_for_updates_internal(std::unique_lock<std::recursive_mutex>& lock) const {
		cv.wait(lock, [this] { return !updated_keys.empty(); }); // updated_keysがセットされるまで待機
	}

	/**
	 * @brief 指定されたキーが更新されるまで待機するプライベート関数。
	 * @details
	 * この関数は、指定されたキーがupdated_keysセットに存在しない場合に呼び出され、
	 * 他のスレッドによってそのキーが更新され通知（notify）が来るまで、ロックを解放して待機します。
	 * @param[in, out] lock 待機中にアンロック/再ロックされるユニークロック。
	 * @param[in] key 待機するキー。
	 */
	void wait_for_updates_internal(std::unique_lock<std::recursive_mutex>& lock, const key_type& key) const {
		cv.wait(lock, [this, &key] { return updated_keys.find(key) != updated_keys.end(); });	// keyがセットされるまで待機
	}

	/**
	 * @brief flagがTRUEならupdated_keysが発生するまで待機する。
	 * @param[in, out] lock 待機中にアンロック/再ロックされるユニークロック。
	 * @param[in] flag_wait_for_updates 待機するかどうかのフラグ。
	 * @return flag_wait_for_updatesの値をそのまま返す。
	 */
	bool if_true_wait_for_updates(std::unique_lock<std::recursive_mutex> &lock, bool flag_wait_for_updates) const {
		if(flag_wait_for_updates){
			wait_for_updates_internal(lock);
		}
		return flag_wait_for_updates;
	}

	/**
	 * @brief flagがTRUEなら指定されたkeyの更新を待機する。
	 * @param[in, out] lock 待機中にアンロック/再ロックされるユニークロック。
	 * @param[in] key 待機するキー。
	 * @param[in] flag_wait_for_updates 待機するかどうかのフラグ。
	 * @return flag_wait_for_key_updateの値をそのまま返す。
	 */
	bool if_true_wait_for_updates(std::unique_lock<std::recursive_mutex> &lock, const key_type& key, bool flag_wait_for_updates) const {
		if(flag_wait_for_updates){
			wait_for_updates_internal(lock, key);
		}
		return flag_wait_for_updates;
	}

	/**
	 * @brief flagがTRUEならupdated_keysをクリアする。
	 * @param[in] flag_clear_updated_keys updated_keysをクリアするかどうかのフラグ。
	 * @return flag_clear_updated_keysの値をそのまま返す。
	 */
	bool if_true_clear_updated_keys(bool flag_clear_updated_keys) {
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
	bool if_true_erase_updated_keys(const set_key_type& key, bool flag_erase_updated_keys) {
		if(flag_erase_updated_keys){
			updated_keys.erase(key);
		}
		return flag_erase_updated_keys;
	}

	/**
	 * @brief 更新された要素（キーと値のペア）のマップを取得する。
	 * @details
	 * `updated_keys` に記録されているキーに対応する要素を全て抽出し、新しいマップとして返します。
	 * @return 更新された要素のみを含む新しいマップ。
	 */
	map_type get_updated_elements_internal() const {
		map_type updated_elements;
		for (const auto& key : updated_keys) {
			auto it = map.find(key);
			if (it != map.end()) {	// mapに存在する場合のみ追加
				// C++17 try_emplace にヒント(end())を渡す
				// mapは昇順、updated_keysも昇順なので、常に末尾に追加される
				updated_elements.try_emplace(updated_elements.end(), key, it->second);
			}
		}
		return updated_elements;
	}

	/**
	 * @brief マップに要素を挿入する。
	 * @details
	 * 挿入が成功した場合、そのキーを `updated_keys` に追加します。
	 * @tparam P 挿入する値の型 (例: std::pair<const Key, Tp>)
	 * @param[in] x 挿入する値。左辺値ならコピー、右辺値ならムーブされます。
	 * @return 挿入に成功した場合はtrue、キーが既に存在した場合はfalse。
	 */
	template <class P>
	bool insert_internal(P&& x) {
		auto result = map.insert(std::forward<P>(x));
		if (result.second) {
			updated_keys.insert(result.first->first); // 更新されたキーを保存
		}
		return result.second;
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
	 * 	auto lock = myMap.acquire_lock(); // ロック取得
	 * 	// 複数の操作をアトミックに実行
	 * 	if (myMap.contains(1)) {
	 * 		auto val = myMap.at(1);
	 * 		myMap.insert_or_assign(2, val + "_new");
	 * 	}
	 * } // スコープを抜けると 'lock' が破棄され、自動でロック解放
	 * ```
	 */
	std::unique_lock<std::recursive_mutex> acquire_lock() const {
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
	void inspect_safely(Func f) const {
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
	 * 	// 効率的な走査 (const_iterator を使用)
	 * 	auto it = map_const_ref.lower_bound(10);
	 * 	if (it != map_const_ref.end() && it->second == "OldValue") {
	 * 		// 安全な書き込み (updated_keys が追跡される)
	 * 		self.insert_or_assign(it->first, "NewValue");
	 * 	}
	 * });
	 * ```
	 */
	template <typename Func>
	void operate_safely(Func f) {
		std::lock_guard<std::recursive_mutex> lock(mtx);
		// const参照(走査用) と non-constな自分自身(書き込み用) を渡す
		f(static_cast<const map_type&>(map), *this);
	}

	// --- 更新待機 ---
	/**
	 * @brief いずれかの要素に更新が発生するまで待機する。
	 * @details 他のスレッドが要素を挿入/更新するまで、この関数を呼び出したスレッドをブロックします。
	 */
	void wait_for_updates() const {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		wait_for_updates_internal(lock);
	}

	/**
	 * @brief 指定されたキーが更新されるまで待機する。(単一キー)
	 * @param[in] key 待機対象のキー
	 */
	void wait_for_updates(const key_type& key) const {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		// 指定されたキーが updated_keys セットに見つかるまで待機
		wait_for_updates_internal(lock, key);
	}

	/**
	 * @brief 指定されたキーのいずれかが更新されるまで待機する。(複数キー、コンテナ版)
	 * @details std::vector, std::set, std::list などイテレート可能なコンテナでキーを指定します。
	 * @tparam KeyContainer キーのコンテナの型
	 * @param[in] keys 待機対象のキーのコンテナ
	 */
	template <typename KeyContainer>
	void wait_for_updates(const KeyContainer& keys) const {
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

	/**
	 * @brief 指定されたキーのいずれかが更新されるまで待機する。(複数キー)
	 * @details initializer_list (例: {1, 2, 3}) でキーを指定します。
	 * @param[in] keys 待機対象のキーのリスト
	 */
	void wait_for_updates(std::initializer_list<key_type> keys) const {
		wait_for_updates<std::initializer_list<key_type>>(keys);
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

	/**
	 * @brief 更新された要素が存在するかを確認する。
	 * @return 更新されたキーが1つ以上あればtrue、なければfalse。
	 */
	bool is_updated() const {
		std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
		return !updated_keys.empty();
	}

	/**
	 * @brief 指定されたキーが更新されたかを確認する。
	 * @param[in] key 確認するキー。
	 * @return キーが更新されていればtrue、なければfalse。
	 */
	bool is_updated(const key_type& key) const {
		std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
		return updated_keys.find(key) != updated_keys.end();
	}

	/**
	 * @brief 指定されたキー群の中に更新されたキーが存在するかを確認する。
	 * @tparam KeyContainer キーを含むコンテナの型 (std::vector, std::list, std::set, etc.)
	 * @param[in] keys 確認するキー群。
	 * @return 更新されたキーが1つ以上あればtrue、なければfalse。
	 */
	template <typename KeyContainer>
	bool is_updated(const KeyContainer& keys) const {
		std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
		for (const auto& key : keys) {
			if (updated_keys.find(key) != updated_keys.end()) {
				return true;
			}
		}
		return false;
	}

	/**
	 * @brief 指定されたキー群の中に更新されたキーが存在するかを確認する。
	 * @param[in] keys 確認するキー群 (initializer_list)。
	 * @return 更新されたキーが1つ以上あればtrue、なければfalse。
	 */
	bool is_updated(std::initializer_list<key_type> keys) const {
		return is_updated<std::initializer_list<key_type>>(keys);
	}

	// --- ルックアップ ---
	/**
	 * @brief 指定されたキーがマップに存在するかを確認する。
	 * @param[in] key 確認するキー
	 * @return キーが存在すればtrue, そうでなければfalse。
	 */
	bool contains(const key_type& key) const {
		std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
#if __cplusplus >= 202002L
		// C++20以上: 標準の contains を使用 (意図が明確で最適)
		return map.contains(key);
#else
		// C++17以下: find != end イディオムを使用
		return map.find(key) != map.end();
#endif
	}

	/**
	 * @brief 指定されたキーを持つ要素の数を返す (0または1)。
	 * @param[in] key 確認するキー
	 * @return 要素の数。
	 */
	size_type count(const key_type& key) const {
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
	std::pair<std::optional<value_type>, std::optional<value_type>> equal_range(const key_type& key) const {
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
	std::optional<value_type> lower_bound(const key_type& key) const {
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
	std::optional<value_type> upper_bound(const key_type& key) const {
		std::lock_guard<std::recursive_mutex> lock(mtx);
		auto it = map.upper_bound(key);
		if (it != map.end()) {
			return *it;
		}
		return std::nullopt;
	}

	// --- 単一要素アクセス ---
	/**
	 * @brief 指定されたキーに対応する値のコピーを取得する (const版)。
	 * @details 更新フラグの消去はできません。
	 * @param[in] key キー
	 * @param[in] flag_wait_for_updates (オプション・デフォルト false) trueの場合、キーが更新されるまで待機する。
	 * @return キーに対応する値 (コピー)。
	 * @throws std::out_of_range キーが存在しない場合。
	 */
	mapped_type at(const key_type& key, bool flag_wait_for_updates = false) const {
		std::unique_lock<std::recursive_mutex> lock(mtx);	// ロックを取得
		if_true_wait_for_updates(lock, key, flag_wait_for_updates);
		return map.at(key);	// コピーを返す
	}

	/**
	 * @brief 指定されたキーに対応する値のコピーを取得する (更新フラグ操作付き)。
	 * @details
	 * 戻り値は値のコピーですが、メソッド自体は内部状態 (updated_keys) を変更する可能性があるため
	 * 非 const メンバー関数として定義します。
	 * @param[in] key キー
	 * @param[in] flag_wait_for_updates trueの場合、キーが更新されるまで待機する。
	 * @param[in] flag_erase_updated_keys trueの場合、取得したキーを更新済みリストから削除する。
	 * @return キーに対応する値 (コピー)。
	 * @throws std::out_of_range キーが存在しない場合。
	 */
	mapped_type at(const key_type& key, bool flag_wait_for_updates, bool flag_erase_updated_keys) {
		std::unique_lock<std::recursive_mutex> lock(mtx);	// ロックを取得
		if_true_wait_for_updates(lock, key, flag_wait_for_updates);
		if_true_erase_updated_keys(key, flag_erase_updated_keys);	// 更新情報を解除
		return map.at(key);	// コピーを返す
	}

	/**
	 * @brief 指定されたキーに対応する値を取得する。キーが存在しない場合は std::nullopt を返す (const版)。
	 * @details 更新フラグの消去はできません。
	 * @param[in] key キー
	 * @return キーに対応する値のコピー、または std::nullopt。
	 */
	std::optional<mapped_type> try_at(const key_type& key) const {
		std::unique_lock<std::recursive_mutex> lock(mtx);	// ロックを取得
		auto it = map.find(key);
		if (it != map.end()) {
			return std::make_optional(it->second);	// コピーを返す
		} else {
			return std::nullopt;
		}
	}

	/**
	 * @brief 指定されたキーに対応する値を取得する (更新フラグ操作付き)。
	 * @details
	 * 戻り値は値のコピーですが、メソッド自体は内部状態 (updated_keys) を変更する可能性があるため
	 * 非 const メンバー関数として定義します。
	 * @param[in] key キー
	 * @param[in] flag_erase_updated_keys trueの場合、取得したキーを更新済みリストから削除する。
	 * @return キーに対応する値のコピー、または std::nullopt。
	 */
	std::optional<mapped_type> try_at(const key_type& key, bool flag_erase_updated_keys) {
		std::unique_lock<std::recursive_mutex> lock(mtx);	// ロックを取得
		auto it = map.find(key);
		if (it != map.end()) {
			if_true_erase_updated_keys(key, flag_erase_updated_keys);
			return std::make_optional(it->second);	// コピーを返す
		} else {
			return std::nullopt;
		}
	}

	/**
	 * @brief 指定されたキーに対応する値を取得する。キーが存在しない場合はデフォルト値を返す (const版)。
	 * @details デフォルト値は左辺値・右辺値の両方を受け入れ、適切にコピーまたはムーブされる。
	 * これは const メンバ関数であり、内部状態を変更しない。
	 * 更新フラグの消去はできません。
	 * @tparam U デフォルト値の型 (mapped_type に変換可能であること)
	 * @param[in] key キー
	 * @param[in] default_value キーが存在しない場合に返すデフォルト値 (Forwarding Reference)
	 * @return キーに対応する値、またはデフォルト値。
	 */
	template <typename U>
	mapped_type at_or(const key_type& key, U&& default_value) const {
		std::unique_lock<std::recursive_mutex> lock(mtx);	// ロックを取得
		auto it = map.find(key);
		if (it != map.end()) {
			return it->second;
		} else {
			// 左辺値ならコピー、右辺値ならムーブして返す
			return std::forward<U>(default_value);
		}
	}

	/**
	 * @brief 指定されたキーに対応する値を取得する (更新フラグ操作付き)。
	 * @details デフォルト値は左辺値・右辺値の両方を受け入れ、適切にコピーまたはムーブされる。
	 * 戻り値は値のコピーですが、メソッド自体は内部状態 (updated_keys) を変更する可能性があるため
	 * 非 const メンバー関数として定義します。
	 * @tparam U デフォルト値の型 (mapped_type に変換可能であること)
	 * @param[in] key キー
	 * @param[in] default_value キーが存在しない場合に返すデフォルト値 (Forwarding Reference)
	 * @param[in] flag_erase_updated_keys trueの場合、取得したキーを更新済みリストから削除する。
	 * @return キーに対応する値、またはデフォルト値。
	 */
	template <typename U>
	mapped_type at_or(const key_type& key, U&& default_value, bool flag_erase_updated_keys) {
		std::unique_lock<std::recursive_mutex> lock(mtx);	// ロックを取得
		auto it = map.find(key);
		if (it != map.end()) {
			if_true_erase_updated_keys(key, flag_erase_updated_keys);
			return it->second;
		} else {
			return std::forward<U>(default_value);
		}
	}

	// --- 一括操作 (Traversals) ---
	// 全要素走査
	/**
	 * @brief 全ての要素に対して関数を適用する (ゼロコピー・読み取り専用・const版)。
	 * @details
	 * マップ内の全要素を走査し、コールバック関数を実行します。
	 * コールバック関数には (const Key&, const Value&) が渡されます。
	 * コールバック内では map への書き込み操作や、長時間の処理を行わないでください。
	 * 更新フラグの消去はできません。
	 * @tparam Func 関数オブジェクト ( void(const key_type&, const mapped_type&) )
	 * @param[in] action 適用する処理
	 * @param[in] flag_wait_for_updates (オプション・デフォルト false) trueの場合、更新キーが発生するまで待機する。
	 */
	template <typename Func>
	void for_each(Func action, bool flag_wait_for_updates = false) const {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		if_true_wait_for_updates(lock, flag_wait_for_updates);
		for (const auto& [key, value] : map) {
			action(key, value);
		}
	}

	/**
	 * @brief 全ての要素に対して関数を適用する (ゼロコピー・読み取り専用・const版)。
	 * @details
	 * マップ内の全要素を走査し、コールバック関数を実行します。
	 * コールバックには (const Key&, const Value&) が渡されます。
	 * コールバック内では map への書き込み操作や、長時間の処理を行わないでください。
	 * action にはconst参照で渡されますが、メソッド自体は内部状態 (updated_keys) を変更する可能性があるため
	 * 非 const メンバー関数として定義します。
	 * @tparam Func 関数オブジェクト ( void(const key_type&, const mapped_type&) )
	 * @param[in] action 適用する処理
	 * @param[in] flag_wait_for_updates trueの場合、更新キーが発生するまで待機する。
	 * @param[in] flag_clear_updated_keys trueなら処理後に更新履歴をクリア
	 */
	template <typename Func>
	void for_each(Func action, bool flag_wait_for_updates, bool flag_clear_updated_keys) {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		if_true_wait_for_updates(lock, flag_wait_for_updates);
		for (const auto& [key, value] : map) {
			action(key, value);
		}
		if_true_clear_updated_keys(flag_clear_updated_keys);
	}

	// 更新要素走査
	/**
	 * @brief 更新された全ての要素に対して関数を適用する (ゼロコピー・読み取り専用・const版)。
	 * @details
	 * コールバック関数には (const Key&, const Value&) が渡されます。
	 * コールバック内では map への書き込み操作や、長時間の処理を行わないでください。
	 * 更新フラグの消去はできません。
	 * @tparam Func 関数オブジェクト ( void(const key_type&, const mapped_type&) )
	 * @param[in] action 適用する処理
	 * @param[in] flag_wait_for_updates (オプション・デフォルト false) trueの場合、更新キーが発生するまで待機する。
	 */
	template <typename Func>
	void for_each_updated(Func action, bool flag_wait_for_updates = false) const {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		if_true_wait_for_updates(lock, flag_wait_for_updates);
		for (const auto& key : updated_keys) {
			if (auto it = map.find(key); it != map.end()) {
				action(key, static_cast<const mapped_type&>(it->second));
			}
		}
	}

	/**
	 * @brief 更新された全ての要素に対して関数を適用する (ゼロコピー・読み取り専用・更新フラグ操作付き)。
	 * @details
	 * コールバック関数には (const Key&, const Value&) が渡されます。
	 * コールバック内では map への書き込み操作や、長時間の処理を行わないでください。
	 * action にはconst参照で渡されますが、メソッド自体は内部状態 (updated_keys) を変更する可能性があるため
	 * 非 const メンバー関数として定義します。
	 * @tparam Func 関数オブジェクト ( void(const key_type&, const mapped_type&) )
	 * @param[in] action 適用する処理
	 * @param[in] flag_wait_for_updates trueの場合、更新キーが発生するまで待機する。
	 * @param[in] flag_clear_updated_keys trueなら処理後に更新履歴をクリア
	 */
	template <typename Func>
	void for_each_updated(Func action, bool flag_clear_updated_keys) {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		if_true_wait_for_updates(lock, flag_clear_updated_keys);
		for (const auto& key : updated_keys) {
			if (auto it = map.find(key); it != map.end()) {
				action(key, static_cast<const mapped_type&>(it->second));
			}
		}
		if_true_clear_updated_keys(flag_clear_updated_keys);
	}

	// --- 一括取得 (Getters) ---
	/**
	 * @brief 全ての要素のコピーを取得する (const版)。
	 * @param[in] flag_wait_for_updates (オプション・デフォルト false) trueの場合、更新キーが発生するまで待機する。
	 * @return マップ全体のコピー。
	 */
	map_type get_all(bool flag_wait_for_updates = false) const {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		if_true_wait_for_updates(lock, flag_wait_for_updates);
		return map;	// コピーを返す
	}

	/**
	 * @brief 全ての要素のコピーを取得する (更新フラグ操作付き)。
	 * @param[in] flag_wait_for_updates trueの場合、更新キーが発生するまで待機する。
	 * @param[in] flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return マップ全体のコピー。
	 */
	map_type get_all(bool flag_wait_for_updates, bool flag_clear_updated_keys) {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		if_true_wait_for_updates(lock, flag_wait_for_updates);
		if_true_clear_updated_keys(flag_clear_updated_keys);	// updated_keysをクリア
		return map;	// コピーを返す
	}

	/**
	 * @brief 更新されたキーのリストを取得する (const版)。
	 * @details
	 * 現在 `updated_keys` に記録されている全てのキーのコピーを返します。
	 * @param[in] flag_wait_for_updates (オプション・デフォルト false) trueの場合、更新キーが発生するまで待機する。
	 * @return 更新されたキーのセットのコピー。
	 */
	set_type get_updated_keys(bool flag_wait_for_updates = false) const {
		std::unique_lock<std::recursive_mutex> lock(mtx);	// ロックを取得
		if_true_wait_for_updates(lock, flag_wait_for_updates);
		return updated_keys;	// コピーを返す
	}

	/**
	 * @brief 更新されたキーのリストを取得する (更新フラグ操作付き)。
	 * @details
	 * 現在 `updated_keys` に記録されている全てのキーのコピーを返します。
	 * @param[in] flag_wait_for_updates trueの場合、更新キーが発生するまで待機する。
	 * @param[in] flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return 更新されたキーのセットのコピー。
	 */
	set_type get_updated_keys(bool flag_wait_for_updates, bool flag_clear_updated_keys) {
		std::unique_lock<std::recursive_mutex> lock(mtx);	// ロックを取得
		if_true_wait_for_updates(lock, flag_wait_for_updates);
		set_type copy = updated_keys;	// コピーを取得
		if_true_clear_updated_keys(flag_clear_updated_keys);	// updated_keysをクリア
		return copy;
	}

	/**
	 * @brief 更新された要素（キーと値のペア）のマップを取得する (const版)。
	 * @details
	 * `updated_keys` に記録されているキーに対応する要素を全て抽出し、新しいマップとして返します。
	 * 更新フラグの消去はできません。
	 * @param[in] flag_wait_for_updates (オプション・デフォルト false) trueの場合、更新キーが発生するまで待機する。
	 * @return 更新された要素のみを含む新しいマップ。
	 */
	map_type get_updated_elements(bool flag_wait_for_updates = false) const {
		std::unique_lock<std::recursive_mutex> lock(mtx);	// ロックを取得
		if_true_wait_for_updates(lock, flag_wait_for_updates);
		return get_updated_elements_internal();	// ロックはunique_lockのデストラクタで解放
	}

	/**
	 * @brief 更新された要素（キーと値のペア）のマップを取得する (更新フラグ操作付き)。
	 * @details
	 * `updated_keys` に記録されているキーに対応する要素を全て抽出し、新しいマップとして返します。
	 * 戻り値は値のコピーですが、メソッド自体は内部状態 (updated_keys) を変更する可能性があるため
	 * 非 const メンバー関数として定義します。
	 * @param[in] flag_wait_for_updates trueの場合、更新キーが発生するまで待機する。
	 * @param[in] flag_clear_updated_keys trueの場合、取得後に更新済みキーのリストをクリアする。
	 * @return 更新された要素のみを含む新しいマップ。
	 */
	map_type get_updated_elements(bool flag_wait_for_updates, bool flag_clear_updated_keys) {
		std::unique_lock<std::recursive_mutex> lock(mtx);	// ロックを取得
		if_true_wait_for_updates(lock, flag_wait_for_updates);
		map_type updated_elements = get_updated_elements_internal();	// 更新された要素を取得
		if_true_clear_updated_keys(flag_clear_updated_keys);	// updated_keysをクリア
		return updated_elements;	// ロックはunique_lockのデストラクタで解放
	}

	// --- 修飾子 (Modifiers) ---
	/**
	 * @brief 更新済みキーのリストをクリアする。
	 */
	void clear_updated_keys() {
		std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
		updated_keys.clear();	// ロックはlock_guardのデストラクタで解放
	}

	/**
	 * @brief 要素をその場で変更する。
	 * @details
	 * 指定されたキーに対応する要素が存在する場合、Modifier関数オブジェクトを呼び出して値を変更します。
	 * キー検索は1回のみ、値のコピーは発生しません。
	 *
	 * @note
	 * - Modifierが bool を返す場合: true が返された時のみ更新とみなします (最適化用)。
	 *
	 * - Modifierが void を返す場合: 常に更新されたとみなします (利便性用)。
	 * @tparam Modifier 関数オブジェクト。 bool(mapped_type&) または void(mapped_type&)
	 * @param key キー
	 * @param modifier 変更を行う関数。bool を返す場合は、変更があったときに true を返してください。
	 * @return キーが見つかり、modifierが実行された場合は true。
	 */
	template <typename Modifier>
	bool modify(const key_type& key, Modifier modifier) {
		std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得

		auto it = map.find(key);
		if (it != map.end()) {
			// ラムダ式の戻り値の型を取得
			using ReturnType = std::invoke_result_t<Modifier, mapped_type&>;

			if constexpr (std::is_same_v<ReturnType, bool>) {
				// --- パターンA: boolを返す (利用者責任) ---
				if (modifier(it->second)) {
					updated_keys.insert(key);
					cv.notify_all();
					return true;
				}
			} else {
				// --- パターンB: voidを返す (常に更新扱い) ---
				modifier(it->second);
				updated_keys.insert(key);	// 無条件で更新とみなす
				cv.notify_all();
				return true;
			}
		}
		return false;
	}

	/**
	 * @brief マップに要素を挿入する。
	 * @details
	 * 挿入が成功した場合、そのキーを `updated_keys` に追加し、待機中のスレッドに通知します。
	 * @tparam P 挿入する値の型 (例: std::pair<const Key, Tp>)
	 * @param[in] x 挿入する値。左辺値ならコピー、右辺値ならムーブされます。
	 * @return 挿入に成功した場合はtrue、キーが既に存在した場合はfalse。
	 */
	template <class P>
	bool insert(P&& x) {
		bool result;
		{
			std::lock_guard<std::recursive_mutex> lock(mtx);	// ロックを取得
			result = insert_internal(x);
		} // ロックを解放
		if(result){
			cv.notify_all();	// 更新を待機しているスレッドに通知
		}
		return result;
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
	 * @brief 範囲内の要素を一括挿入する。
	 * @details
	 * ロックを1回だけ取得してループ処理を行うため、個別にinsertするより高速です。
	 * @tparam Range 要素(pair)の範囲
	 * @param[in] range 挿入する要素の範囲
	 * @return 挿入に成功した要素の数。
	 */
	template <typename Range>
	size_type insert_range(Range&& range) {
		size_type inserted_count = 0;
		{
			std::lock_guard<std::recursive_mutex> lock(mtx);
			for (auto&& item : range) {
				inserted_count += insert_internal(std::forward<decltype(item)>(item));	// コピーまたはムーブ
			}
		}
		if (inserted_count > 0) {
			cv.notify_all();
		}
		return inserted_count;
	}

	/**
	 * @brief 初期化子リストから要素を一括挿入する。
	 * @param[in] list 挿入する要素のリスト
	 * @return 挿入に成功した要素の数。
	 */
	size_type insert_range(std::initializer_list<value_type> list) {
		return insert_range<std::initializer_list<value_type>>(list);
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

	// --- その他のメンバ関数 ---
	/**
	 * @brief 別のSafeTrackedMapと内容を交換する。
	 * @details std::lockを使いデッドロックセーフに両方のmutexをロックしてswapする。
	 * @param[in, out] other 交換対象のマップ
	 */
	void swap(SafeTrackedMap& other) noexcept (
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
 * @tparam Key キーの型
 * @tparam Tp 値の型
 * @tparam Compare キーの比較関数の型
 * @tparam Alloc アロケータの型
 * @param[in, out] lhs 交換対象の左側のマップ
 * @param[in, out] rhs 交換対象の右側のマップ
 */
template <typename Key, typename Tp, typename Compare, typename Alloc>
void swap(SafeTrackedMap<Key, Tp, Compare, Alloc>& lhs,
		SafeTrackedMap<Key, Tp, Compare, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
	lhs.swap(rhs);
}

} // namespace utils
