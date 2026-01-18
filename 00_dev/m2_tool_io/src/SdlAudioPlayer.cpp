#include "SdlAudioPlayer.hpp"

namespace sdl {

AudioPlayer::AudioPlayer(int flags, int frequency, Uint16 format, int channels, int chunksize) :
	currentBgm(nullptr),
	initialized(false)
{
	// 1. SDLオーディオサブシステムの初期化
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		throw AudioException(std::string("SDL Init Failed: ") + SDL_GetError());
	}

	// 2. フォーマット(MP3/OGG等)の初期化
	if (flags) {
		int initted = Mix_Init(flags);
		if ((initted & flags) != flags) {
			// 失敗時: SDLを閉じてから投げる
			SDL_Quit();
			throw AudioException(std::string("Mix_Init: Some formats not loaded: ") + Mix_GetError());
		}
	}

	// 3. Mixerデバイスを開く (引数で指定された設定を使用)
	if (Mix_OpenAudio(frequency, format, channels, chunksize) < 0) {
		// 失敗時: Mix_Initの後始末(必要な場合)とSDLの後始末をしてから投げる
		if (flags) {
			Mix_Quit();
		}
		SDL_Quit();
		throw AudioException(std::string("Mixer Init Failed: ") + Mix_GetError());
	}

	initialized = true;
}

AudioPlayer::~AudioPlayer() {
	clearResources();

	if (initialized) {
		Mix_CloseAudio();
		Mix_Quit();
		SDL_Quit();
	}
}

void AudioPlayer::clearResources() {
	// BGMの解放
	if (currentBgm) {
		Mix_HaltMusic();
		Mix_FreeMusic(currentBgm);
		currentBgm = nullptr;
	}

	// SEキャッシュの全解放
	for (auto& pair : seCache) {
		if (pair.second) {
			Mix_FreeChunk(pair.second);
		}
	}
	seCache.clear();
}

void AudioPlayer::playSE(const std::string& filepath) {
	if (!initialized) throw AudioException("AudioPlayer is not initialized.");

	Mix_Chunk* chunk = nullptr;

	// キャッシュ確認
	auto it = seCache.find(filepath);
	if (it != seCache.end()) {
		chunk = it->second;
	} else {
		// 新規ロード
		chunk = Mix_LoadWAV(filepath.c_str());
		if (!chunk) {
			throw AudioException(std::string("Failed to load SE: ") + filepath + " -> " + Mix_GetError());
		}
		seCache[filepath] = chunk;
	}

	// 再生 (channel: -1=空きを探す, loop: 0=1回のみ)
	if (Mix_PlayChannel(-1, chunk, 0) == -1) {
		// 再生失敗時は例外を投げる（状況によりログのみにする設計もあり）
		throw AudioException(std::string("Failed to play SE: ") + Mix_GetError());
	}
}

void AudioPlayer::playBGM(const std::string& filepath) {
	if (!initialized) throw AudioException("AudioPlayer is not initialized.");

	// 既存のBGMがあれば止めて解放
	if (currentBgm) {
		Mix_HaltMusic();
		Mix_FreeMusic(currentBgm);
		currentBgm = nullptr;
	}

	// ストリーミング読み込み
	currentBgm = Mix_LoadMUS(filepath.c_str());
	if (!currentBgm) {
		throw AudioException(std::string("Failed to load BGM: ") + filepath + " -> " + Mix_GetError());
	}

	// 再生 (loop: -1=無限ループ)
	if (Mix_PlayMusic(currentBgm, -1) == -1) {
		throw AudioException(std::string("Failed to play BGM: ") + Mix_GetError());
	}
}

void AudioPlayer::stopBGM() {
	Mix_HaltMusic();
}

void AudioPlayer::setVolume(int volume) {
	// 0〜128の範囲にクランプ
	if (volume < 0) { volume = 0; }
	if (volume > 128) { volume = 128; }

	// 全チャンネルのSE音量設定
	Mix_Volume(-1, volume);
	// BGMの音量設定
	Mix_VolumeMusic(volume);
}

} // namespace SdlWrapper
