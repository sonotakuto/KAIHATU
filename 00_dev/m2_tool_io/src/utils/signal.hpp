#pragma once
#include <mutex>
#include <condition_variable>
#include <chrono>

/**
 * @namespace utils
 * @brief ユーティリティ関数やクラスをまとめた名前空間
 */
namespace utils {

class Signal {
private:
	std::mutex mtx;
	std::condition_variable cv;
	bool signaled = false;

public:
	// デフォルトコンストラクタ
	Signal() = default;

	// 【重要】コピーとムーブを禁止（誤用防止）
	// std::mutex等を持っているのでコピーは危険、かつ意図しない動作の元になる
	Signal(const Signal&) = delete;
	Signal& operator=(const Signal&) = delete;
	Signal(Signal&&) = delete;
	Signal& operator=(Signal&&) = delete;

	// デストラクタ（通常はデフォルトでOK）
	~Signal() = default;

	/**
	 * @brief シグナルを通知します。待機中のスレッドがあれば1つだけ起床させます。
	 */
	void notify_one();

	/**
	 * @brief シグナルを通知します。待機中の全てのスレッドを起床させます。
	 */
	void notify_all();

	/**
	 * @brief シグナルが通知されるまで待機します。通知されると自動的にリセットされます。
	 */
	void wait();

	/**
	 * @brief 指定された時間だけシグナルの通知を待機します。
	 * @param[in] rel_time 待機する時間。
	 * @return シグナルが通知されて起床した場合はtrue、タイムアウトした場合はfalseを返します。
	 */
	template<class Rep, class Period>
	bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) {
		std::unique_lock<std::mutex> lock(mtx);
		// 述語がtrueになるかタイムアウトするまで待機
		if (cv.wait_for(lock, rel_time, [this] { return signaled; })) {
			signaled = false;	// 自動リセット
			return true;
		}
		return false;
	}

	/**
	 * @brief 指定された時刻までシグナルの通知を待機します。
	 * @param[in] abs_time 待機を終了する時刻。
	 * @return シグナルが通知されて起床した場合はtrue、タイムアウトした場合はfalseを返します。
	 */
	template<class Clock, class Duration>
	bool wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
		std::unique_lock<std::mutex> lock(mtx);
		if (cv.wait_until(lock, abs_time, [this] { return signaled; })) {
			signaled = false;	// 自動リセット
			return true;
		}
		return false;
	}
};

} // namespace utils
