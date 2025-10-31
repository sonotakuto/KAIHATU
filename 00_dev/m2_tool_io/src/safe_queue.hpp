#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

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
	typedef std::queue<_Tp, _Sequence>		que_type;
	typedef typename que_type::value_type	value_type;
	typedef typename que_type::size_type	size_type;

private:
	que_type que;					// 内部キュー
	mutable std::mutex mtx;			// 排他制御用ミューテックス
	std::condition_variable cv;		// 条件変数

public:
	// デフォルトコンストラクタ
	SafeQueue() = default;

	/**
	 * @brief キューに要素を追加（プッシュ）し、待機中のスレッドに通知する（スレッドセーフ）。
	 * @param[in] value 追加する要素。
	 */
	void push(const value_type& value){
		{
			std::lock_guard<std::mutex> lock(mtx);
			que.push(value);
		}
		cv.notify_one();  // 待機中のスレッドに通知
	}

	/**
	 * @brief キューから要素を取り出す（ブロッキング）。キューが空の場合、要素が追加されるまで待機する。
	 * @return 取り出された要素。
	 */
	value_type pop(){
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this] { return !que.empty(); }); // データが入るまで待機
		value_type value = que.front();
		que.pop();
		return value;
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
			value_type value = que.front();
			que.pop();
			return value;
		}
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
			value_type value = que.front();
			que.pop();
			return value;
		} else {
			// タイムアウトした場合
			return std::nullopt;
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
			value_type value = que.front();
			que.pop();
			return value;
		} else {
			// タイムアウトした場合
			return std::nullopt;
		}
	}

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
};
} // namespace utils
