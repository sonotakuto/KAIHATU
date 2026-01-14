#include "signal.hpp"

namespace utils {

void Signal::notify_one() {
	{
		std::lock_guard<std::mutex> lock(mtx);
		signaled = true;
	}
	cv.notify_one();
}

void Signal::notify_all() {
	{
		std::lock_guard<std::mutex> lock(mtx);
		signaled = true;
	}
	cv.notify_all();
}

void Signal::wait() {
	std::unique_lock<std::mutex> lock(mtx);
	cv.wait(lock, [this] { return signaled; });
	signaled = false;	// 自動リセット
}

} // namespace utils
