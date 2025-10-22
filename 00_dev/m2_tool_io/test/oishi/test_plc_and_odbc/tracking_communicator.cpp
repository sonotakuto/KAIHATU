/**
 * @file
 * @brief
 */
#include "./includes.hpp"
#include "./tracking_communicator.hpp"

namespace tracker {

/// @brief 工具追跡システムとの通信ソケット。
inline sg::Socket socket;

bool connect_to_tracker() {
	return socket.connect(HOST, PORT);
}

void run_tracking_listener() {

}
} // namespace tracker