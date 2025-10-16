#pragma once
#include <chrono>
#include <thread>

namespace utils {

/**
 * @brief 正確な周期実行をサポートするタイマークラス。
 */
class PeriodicTimer {
public:
	/**
	 * @brief コンストラクタ。タイマーの基準時刻を現在時刻に初期化します。
	 */
	PeriodicTimer() : next_time_(std::chrono::steady_clock::now()) {}

	/**
	 * @brief 次の周期の開始時刻まで待機します。
	 * @tparam Rep durationの内部表現の型
	 * @tparam Period durationの比率の型
	 * @param period 待機したい周期
	 * @param skip_on_overrun trueの場合、処理が周期を超えたら超過分をスキップする
	 * @return スキップした周期の回数。超過しなかった場合は0。
	 */template<typename Rep, typename Period>
	size_t wait_for_next(const std::chrono::duration<Rep, Period>& period, bool skip_on_overrun = false) {
		size_t skipped_ticks = 0;

		// 次の理想的な目標時刻を計算
		next_time_ += period;

		if (skip_on_overrun) {
			// --- スキップモードの時の処理 ---
			const auto now = std::chrono::steady_clock::now();

			// 現在時刻が、本来の次の目標時刻を既に過ぎているかチェック
			if (now >= next_time_) {
				// オーバーラン発生
				// 理想時刻を基準に、さらに何周期分スキップすれば 'now' を越えられるか計算
				skipped_ticks = ((now - next_time_) / period) + 1;

				// 次の目標時刻を、未来の適切な周期に設定する
				next_time_ += skipped_ticks * period;
			}
		}
		std::this_thread::sleep_until(next_time_);
		return skipped_ticks;
	}

	/**
	 * @brief タイマーの基準時刻を現在時刻にリセットします。
	 * @details 長時間待機した後などに呼び出すことで、次の待機が即座に開始されるのを防ぎます。
	 */
	void reset() {
		next_time_ = std::chrono::steady_clock::now();
	}

private:
	std::chrono::time_point<std::chrono::steady_clock> next_time_;
};

} // namespace utils