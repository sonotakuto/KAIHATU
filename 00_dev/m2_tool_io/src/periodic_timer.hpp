/**
 * @file periodic_timer.hpp
 * @brief 正確な周期実行をサポートするタイマークラスのヘッダファイル。
 */
#pragma once
#include <chrono>
#include <thread>

/**
 * @namespace utils
 * @brief ユーティリティ関数やクラスをまとめた名前空間
 */
namespace utils {

/**
 * @brief 正確な周期実行をサポートするタイマークラス。
 */
class PeriodicTimer {
public:
	using Clock = std::chrono::steady_clock;
	using TimePoint = std::chrono::time_point<Clock>;

	/**
	 * @brief コンストラクタ。タイマーの基準時刻を現在時刻に初期化します。
	 */
	PeriodicTimer() : next_time_(std::chrono::steady_clock::now()) {}

	/**
	 * @brief 次の周期の目標時刻を計算・更新し、その時刻を返します。
	 * @details この関数自体は待機を行いません。外部の wait_until 等で使用するために利用します。
	 * @tparam Rep durationの内部表現の型
	 * @tparam Period durationの比率の型
	 * @param[in] period 待機したい周期
	 * @param[in] skip_on_overrun (オプション・デフォルト false) trueの場合、処理が周期を超えたら超過分をスキップする
	 * @param[out] out_skipped_ticks (オプション) スキップした回数を格納するポインタ
	 * @return 更新された次回の目標時刻 (TimePoint)
	 */
	template<typename Rep, typename Period>
	TimePoint update_target_time(
		const std::chrono::duration<Rep, Period>& period,
		bool skip_on_overrun = false,
		size_t* out_skipped_ticks = nullptr
	) {
		size_t skipped_ticks = 0;

		// 次の理想的な目標時刻を計算
		next_time_ += period;

		if (skip_on_overrun) {
			// --- スキップモードの時の処理 ---
			const auto now = Clock::now();

			// 現在時刻が、本来の次の目標時刻を既に過ぎているかチェック
			if (now >= next_time_) {
				// オーバーラン発生
				// 理想時刻を基準に、さらに何周期分スキップすれば 'now' を越えられるか計算
				skipped_ticks = ((now - next_time_) / period) + 1;

				// 次の目標時刻を、未来の適切な周期に設定する
				next_time_ += skipped_ticks * period;
			}
		}

		// スキップした回数を出力引数に設定
		if (out_skipped_ticks) {
			*out_skipped_ticks = skipped_ticks;
		}

		return next_time_;
	}

	/**
	 * @brief 次の周期の開始時刻まで待機します。
	 * @tparam Rep durationの内部表現の型
	 * @tparam Period durationの比率の型
	 * @param[in] period 待機したい周期
	 * @param[in] skip_on_overrun (オプション・デフォルト false) trueの場合、処理が周期を超えたら超過分をスキップする
	 * @return スキップした周期の回数。超過しなかった場合は0。
	 */template<typename Rep, typename Period>
	size_t wait_for_next(const std::chrono::duration<Rep, Period>& period, bool skip_on_overrun = false) {
		size_t skipped_ticks = 0;
		const auto target = update_target_time(period, skip_on_overrun, &skipped_ticks);
		std::this_thread::sleep_until(target);
		return skipped_ticks;
	}

	/**
	 * @brief タイマーの基準時刻を現在時刻にリセットします。
	 * @details 長時間待機した後などに呼び出すことで、次の待機が即座に開始されるのを防ぎます。
	 */
	void reset() {
		next_time_ = Clock::now();
	}

	/**
	 * @brief 現在保持している目標時刻を取得します。
	 */
	TimePoint get_current_target() const {
		return next_time_;
	}

private:
	TimePoint next_time_;
};

} // namespace utils
