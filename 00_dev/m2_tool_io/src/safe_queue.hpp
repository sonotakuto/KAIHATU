/**
 * @file safe_queue.hpp
 * @brief スレッドセーフなFIFOキューを提供するヘッダファイル。
 */
#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

#ifdef __has_include
 #if __has_include(<version>)
  #include <version>
 #endif
#endif

/**
 * @namespace utils
 * @brief ユーティリティ関数やクラスをまとめた名前空間
 */
namespace utils {
/**
 * @class SafeQueue
 * @brief ミューテックスと条件変数で保護された、スレッドセーフなFIFOキュー。
 * @tparam _Tp キューに格納する要素の型。
 * @tparam _Sequence 内部で使われるコンテナの型。デフォルトはstd::deque。
 */
template<typename _Tp, typename _Sequence = std::deque<_Tp> >
class SafeQueue {
public:
	using que_type		= std::queue<_Tp, _Sequence>;
	using value_type	= typename que_type::value_type;
	using size_type		= typename que_type::size_type;
	using cv_type		= std::condition_variable;
	using notifier_type	= std::function<void()>;

private:
	que_type que;			// 内部キュー
	mutable std::mutex mtx;	// 排他制御用ミューテックス
	cv_type cv;				// 条件変数
	notifier_type notifier; // 通知用コールバック

public:
	// デフォルトコンストラクタ
	SafeQueue() = default;

	// コピー不可にする（誤用防止）
	SafeQueue(const SafeQueue&) = delete;
	SafeQueue& operator=(const SafeQueue&) = delete;

	// --- 基本動作の変更 ---
	/**
	 * @brief 要素が追加された際に呼び出されるコールバック関数を設定します。
	 * @param[in] func 要素追加時に呼び出される関数オブジェクト。
	 * @details 何も設定しない場合は通知は行われません。
	 */
	void set_notifier(const notifier_type& func) {
		std::lock_guard<std::mutex> lock(mtx);
		notifier = func;
	}

	// --- 容量 ---
	/**
	 * @brief キューが空かどうかをスレッドセーフに確認する。
	 * @return キューが空の場合はtrue、そうでない場合はfalse。
	 */
	bool empty() const {
		std::lock_guard<std::mutex> lock(mtx);
		return que.empty();
	}

	/**
	 * @brief キューのサイズをスレッドセーフに取得する。
	 * @return キューに含まれる要素の数。
	 */
	size_type size() const {
		std::lock_guard<std::mutex> lock(mtx);
		return que.size();
	}

	// --- 取り出し (Consumers) ---
	/**
	 * @brief キューから要素を取り出す（ブロッキング）。キューが空の場合、要素が追加されるまで待機する。
	 * @return 取り出された要素。
	 */
	value_type pop(){
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this] { return !que.empty(); }); // データが入るまで待機
		value_type value = std::move(que.front());
		que.pop();
		return value;
	}

	/**
	 * @brief キューから全ての要素を取り出す（ブロッキング）。キューが空の場合、要素が追加されるまで待機する。
	 * @return 取り出された要素を含むキュー。
	 */
	que_type pop_all() {
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this] { return !que.empty(); }); // データが入るまで待機
		que_type result;
		// std::queue::swap はポインタのすげ替えのみなので O(1) で高速
		result.swap(que);
		return result;
	}


	/**
	 * @brief キューから要素を試行的に取り出す（非ブロッキング）。
	 * @return 要素が取り出せた場合はその値、キューが空だった場合はstd::nullopt。
	 */
	std::optional<value_type> try_pop(){
		std::lock_guard<std::mutex> lock(mtx);

		if (que.empty()){
			return std::nullopt;
		}else{
			value_type value = std::move(que.front());
			que.pop();
			return value;
		}
	}

	/**
	 * @brief キューから全ての要素を試行的に取り出す（非ブロッキング）。
	 * @return 取り出された要素を含むキュー。元のキューが空の場合は空のキューを返す。
	 */
	que_type try_pop_all() {
		std::lock_guard<std::mutex> lock(mtx);
		if (que.empty()) {
			return {};
		}
		que_type result;
		// std::queue::swap はポインタのすげ替えのみなので O(1) で高速
		result.swap(que);
		return result;
	}

	/**
	 * @brief キューから要素を取り出す（タイムアウト付きブロッキング）。
	 * @details キューが空の場合、指定されたタイムアウト時間まで要素が追加されるのを待機する。
	 * @param[in] timeout 待機する最大時間。
	 * @return 要素が取り出せた場合はその値、タイムアウトした場合はstd::nullopt。
	 */
	template<typename Rep, typename Period>
	std::optional<value_type> pop_wait_for(const std::chrono::duration<Rep, Period>& timeout) {
		std::unique_lock<std::mutex> lock(mtx);
		// cv.wait_for は、タイムアウトするか、述語(第3引数)がtrueになるまで待機する
		if (cv.wait_for(lock, timeout, [this] { return !que.empty(); })) {
			// 述語がtrueになった（＝キューに要素が来た）場合
			value_type value = std::move(que.front());
			que.pop();
			return value;
		} else {
			// タイムアウトした場合
			return std::nullopt;
		}
	}

	/**
	 * @brief キューから全ての要素を取り出す（タイムアウト付きブロッキング）。
	 * @details キューが空の場合、指定されたタイムアウト時間まで要素が追加されるのを待機する。
	 * @param[in] timeout 待機する最大時間。
	 * @return 要素が取り出せた場合はその値を含むキュー、タイムアウトした場合は空のキュー。
	 */
	template<typename Rep, typename Period>
	que_type pop_all_wait_for(const std::chrono::duration<Rep, Period>& timeout) {
		std::unique_lock<std::mutex> lock(mtx);
		// cv.wait_for は、タイムアウトするか、述語(第3引数)がtrueになるまで待機する
		if (cv.wait_for(lock, timeout, [this] { return !que.empty(); })) {
			// 述語がtrueになった（＝キューに要素が来た）場合
			que_type result;
			// std::queue::swap はポインタのすげ替えのみなので O(1) で高速
			result.swap(que);
			return result;
		} else {
			// タイムアウトした場合
			return {};
		}
	}

	/**
	 * @brief キューから要素を取り出す（タイムアウト付きブロッキング）。
	 * @details キューが空の場合、指定された時刻まで要素が追加されるのを待機する。
	 * @param[in] timeout_time 待機を打ち切る絶対時刻。
	 * @return 要素が取り出せた場合はその値、タイムアウトした場合はstd::nullopt。
	 */
	template<class Clock, class Duration>
	std::optional<value_type> pop_wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time) {
		std::unique_lock<std::mutex> lock(mtx);
		// cv.wait_until は、タイムアウトするか、述語(第3引数)がtrueになるまで待機する
		if (cv.wait_until(lock, timeout_time, [this] { return !que.empty(); })) {
			// 時間内にデータが来て、キューが空でなくなった場合
			value_type value = std::move(que.front());
			que.pop();
			return value;
		} else {
			// タイムアウトした場合
			return std::nullopt;
		}
	}

	/**
	 * @brief キューから全ての要素を取り出す（タイムアウト付きブロッキング）。
	 * @details キューが空の場合、指定された時刻まで要素が追加されるのを待機する。
	 * @param[in] timeout_time 待機を打ち切る絶対時刻。
	 * @return 要素が取り出せた場合はその値を含むキュー、タイムアウトした場合は空のキュー。
	 */
	template<class Clock, class Duration>
	que_type pop_all_wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time) {
		std::unique_lock<std::mutex> lock(mtx);
		// cv.wait_until は、タイムアウトするか、述語(第3引数)がtrueになるまで待機する
		if (cv.wait_until(lock, timeout_time, [this] { return !que.empty(); })) {
			// 時間内にデータが来て、キューが空でなくなった場合
			que_type result;
			// std::queue::swap はポインタのすげ替えのみなので O(1) で高速
			result.swap(que);
			return result;
		} else {
			// タイムアウトした場合
			return {};
		}
	}

	// --- 追加 (Producers) ---
	/**
	 * @brief キューに要素をコピーして追加（プッシュ）し、待機中のスレッドに通知する。
	 * @param[in] value 追加する要素。
	 */
	void push(const value_type& value){
		{
			std::lock_guard<std::mutex> lock(mtx);
			que.push(value);
		}
		cv.notify_one();  // 待機中のスレッドに通知
		if (notifier) {
			notifier();
		}
	}

	/**
	 * @brief キューに要素をムーブして追加（プッシュ）し、待機中のスレッドに通知する。
	 * @details std::unique_ptr 等も格納可能になります。
	 * @param[in] value 追加する要素。
	 */
	void push(value_type&& value){
		{
			std::lock_guard<std::mutex> lock(mtx);
			que.push(std::move(value));
		}
		cv.notify_one();
		if (notifier) {
			notifier();
		}
	}

	/**
	 * @brief キューに要素をコンストラクトして追加（プッシュ）し、待機中のスレッドに通知する。
	 * @tparam Args コンストラクタ引数の型。
	 * @param[in] args コンストラクタ引数のリスト。
	 */
	template <typename... Args>
	void emplace(Args&&... args) {
		{
			std::lock_guard<std::mutex> lock(mtx);
			que.emplace(std::forward<Args>(args)...);
		}
		cv.notify_one();
		if (notifier) {
			notifier();
		}
	}

	/**
	 * @brief 範囲（コンテナやリスト）内の要素を一括追加する。
	 * @details ロック取得が1回で済むため、大量のデータを追加する場合に高速です。
	 * @tparam Range 範囲を表す型 (std::vector, std::list, std::initializer_list など)
	 * @param[in] range 追加する要素の範囲。
	 */
	template <typename Range>
	void push_range(Range&& range) {
		{
			std::lock_guard<std::mutex> lock(mtx);

// C++23 の std::queue::push_range が使えるか確認
#if defined(__cpp_lib_containers_ranges)
			// C++23: 標準の一括追加 (最適化されている可能性が高い)
			que.push_range(std::forward<Range>(range));
#else
			// C++17以下: 手動ループ (ロックは1回なので十分高速)
			for (auto&& item : range) {
				// 可能な限りムーブ、できなければコピー
				que.push(std::forward<decltype(item)>(item));
			}
#endif
		}
		// 一括追加されたので、待機中の全てのスレッドを起こすのが一般的
		cv.notify_all();
		if (notifier) {
			notifier();
		}
	}

	/**
	 * @brief イニシャライザリスト内の要素を一括追加する。
	 * @details ロック取得が1回で済むため、大量のデータを追加する場合に高速です。
	 * @param[in] list 追加する要素のリスト。
	 */
	void push_range(std::initializer_list<value_type> list) {
		push_range<std::initializer_list<value_type>>(list);
	}
};
} // namespace utils
