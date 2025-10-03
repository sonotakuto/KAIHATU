/**
 * @file
 * @brief
 */
#include "./includes.hpp"
#include "./tracking_communicator.hpp"

bool connect_to_tracker() {
	return g_tool_tracking_socket.connect(TRACKER_HOST, TRACKER_PORT);
}

void run_tracking_listener() {

}