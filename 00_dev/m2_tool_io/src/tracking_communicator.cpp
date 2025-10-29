/**
 * @file
 * @brief
 */
#include "./includes.hpp"
#include "./url_transfer.hpp"
#include "./tracking_communicator.hpp"

namespace tracker {

/**
 * @brief 現在時刻に基づいて "recording_YYYYMMDD_HHMMSS.mp4" 形式のファイル名を生成します。
 * @details SimpleGtkライブラリの sg::now() と fmt::format を使用します。
 * @return 生成されたファイル名文字列。
 */
std::string get_timestamp_filename_helper() {
	// 1. 現在時刻を取得
	auto now_tp = sg::now();
	// 2. fmtライブラリの chrono 機能を使って書式化
	//    "{:%Y%m%d_%H%M%S}" が strftime の "%Y%m%d_%H%M%S" に相当します。
	return fmt::format("recording_{:%Y%m%d_%H%M%S}.mp4", now_tp);
}

/// @brief 工具追跡システムとの通信ソケット。
inline sg::Socket socket;

bool connect_to_tracker() {
	return socket.connect(HOST, PORT);
}

void run_tracking_listener() {

}
} // namespace tracker
