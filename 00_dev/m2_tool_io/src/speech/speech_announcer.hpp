/**
 * @file speech_announcer.hpp
 * @brief ATP3011読み上げをGUIスレッドから非同期に呼び出すためのラッパー。
 */
#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include "atp3011/atp3011.hpp"

//#if !defined(PRODUCTION) && !defined(STUB_SPEECH)
//#define STUB_SPEECH	// 開発環境の場合は読み上げをスタブ化
//#endif

namespace speech {

/**
 * @class SpeechAnnouncer
 * @brief 専用スレッドで読み上げを行う。say() は即座に戻り、読み上げ中なら中断して最新の文字列を読む。
 */
class SpeechAnnouncer {
public:
	SpeechAnnouncer();
	~SpeechAnnouncer();

	SpeechAnnouncer(const SpeechAnnouncer&) = delete;
	SpeechAnnouncer& operator=(const SpeechAnnouncer&) = delete;

	/// @brief 読み上げを依頼する (ノンブロッキング)。読み上げ中なら中断して置き換える。
	void say(std::string text);

	/// @brief 読み上げ中・待機中の読み上げを中止する (ノンブロッキング)。
	void cancel();

private:
	static atp3011::Config make_config();
	void run();

	atp3011::Atp3011Speaker		speaker_;
	std::mutex					mtx_;
	std::condition_variable		cv_;
	std::optional<std::string>	pending_;
	bool						quit_ = false;
	std::thread					thread_;	// 最後に宣言 (他メンバ初期化後に起動するため)
};

} // namespace speech
