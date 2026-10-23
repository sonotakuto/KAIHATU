/**
 * @file speech_announcer.cpp
 * @brief SpeechAnnouncer の実装。
 */
#include "includes.hpp"
#include "speech_announcer.hpp"

namespace speech {

atp3011::Config SpeechAnnouncer::make_config() {
	atp3011::Config cfg;	// GPIO332, /dev/ttyUL0 などは既定値を使用
	// 本アプリは GLib (sgex::UnixSignal) で SIGINT/SIGTERM を扱うため、
	// ライブラリ側で sigaction を上書きさせない。
	cfg.installSignalHandlers = false;
	return cfg;
}

SpeechAnnouncer::SpeechAnnouncer() :
	speaker_(make_config())
{
	thread_ = std::thread(&SpeechAnnouncer::run, this);
}

SpeechAnnouncer::~SpeechAnnouncer() {
	{
		std::lock_guard<std::mutex> lock(mtx_);
		quit_ = true;
		pending_.reset();
	}
	speaker_.requestStop();
	cv_.notify_all();
	if (thread_.joinable()) { thread_.join(); }
}

void SpeechAnnouncer::say(std::string text) {
	if (text.empty()) { return; }
	{
		std::lock_guard<std::mutex> lock(mtx_);
		pending_ = std::move(text);
	}
	speaker_.requestStop();	// 読み上げ中のものがあれば中断 (次の speak() 開始時にクリアされる)
	cv_.notify_one();
}

void SpeechAnnouncer::cancel() {
	{
		std::lock_guard<std::mutex> lock(mtx_);
		pending_.reset();
	}
	speaker_.requestStop();
}

void SpeechAnnouncer::run() {
	while (true) {
		std::string text;
		{
			std::unique_lock<std::mutex> lock(mtx_);
			cv_.wait(lock, [this] { return quit_ || pending_.has_value(); });
			if (quit_) { return; }
			text = std::move(pending_.value());
			pending_.reset();
		}

//#ifdef STUB_SPEECH
//		DEV_PRINT("[Speech(STUB)] {}\n", text);
//#else
		const auto result = speaker_.speak(text);
		if (result == atp3011::Atp3011Speaker::Result::Failed) {
			DEV_PRINT(stderr, "[Speech] 読み上げに失敗しました: {}\n", text);
		}
//#endif
	}
}

} // namespace speech
