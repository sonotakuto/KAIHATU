/**
 * @file audio_player.cpp
 * @brief SDL2_mixerを使用して、BGMとSEの再生を管理するAudioPlayerクラスと、システム管理クラス(AudioSystem)の実装。
 */
#include "audio_player.hpp"

/**
 * @namespace sdl
 * @brief SDL関連のクラスや関数をまとめた名前空間。
 */
namespace sdl {

/**
 * @namespace sdl::details
 * @brief AudioPlayerの内部実装の詳細を隠蔽するための名前空間。
 * @note ユーザーが直接使用することを想定していないクラスや関数を配置しています。
 */
namespace details {

// ==========================================
// AudioSystem の実装
// ==========================================

AudioSystem::AudioSystem(int flags, int frequency, Uint16 format, int channels, int chunksize) {
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

	// 3. Mixerデバイスを開く
	if (Mix_OpenAudio(frequency, format, channels, chunksize) < 0) {
		// 失敗時: Mix_Initの後始末(必要な場合)とSDLの後始末をしてから投げる
		if (flags) {
			Mix_Quit();
		}
		SDL_Quit();
		throw AudioException(std::string("Mixer Init Failed: ") + Mix_GetError());
	}

	// 初期化成功時にフラグを立てる
	is_active_ = true;
}

AudioSystem::~AudioSystem() {
	// プログラム終了時にフラグを折る (これ以降 AudioPlayer は自身の破棄処理を行わなくなる)
	is_active_ = false;

	Mix_CloseAudio();
	Mix_Quit();
	SDL_Quit();
}

AudioSystem& AudioSystem::initialize(int flags, int frequency, Uint16 format, int channels, int chunksize) {
	// 関数内静的変数によるマイヤーズ・シングルトン。スレッドセーフで初回のみ初期化される。
	static AudioSystem instance(flags, frequency, format, channels, chunksize);
	return instance;
}

} // namespace details

// ==========================================
// AudioPlayer の実装
// ==========================================

AudioPlayer::AudioPlayer(int flags, int frequency, Uint16 format, int channels, int chunksize) :
	currentBgm(nullptr)
{
	// AudioPlayerが生成されたタイミングで、システムクラスの初期化を呼び出す
	// (既に初期化されている場合は何も起こらない)
	details::AudioSystem::initialize(flags, frequency, format, channels, chunksize);
}

AudioPlayer::~AudioPlayer() {
	// AudioSystem がまだ生きている(SDLが有効な)場合のみ、行儀よくリソースを解放する。
	// 既にシステムが破棄されている場合はOSのメモリ回収に任せ、クラッシュを防ぐ。
	if (details::AudioSystem::is_active()) {
		clearResources();
	}
}

void AudioPlayer::clearResources() {
	// BGMの解放
	if (currentBgm) {
		Mix_HaltMusic();
		Mix_FreeMusic(currentBgm);
		currentBgm = nullptr;
	}

	// 自身のインスタンスが再生しているSEのみを特定して停止
	int numChannels = Mix_AllocateChannels(-1); // 現在割り当てられている全チャンネル数を取得
	for (int i = 0; i < numChannels; ++i) {
		Mix_Chunk* playingChunk = Mix_GetChunk(i); // チャンネルiで再生中のチャンクを取得
		if (playingChunk) {
			// 再生中のチャンクが、自身のキャッシュ(seCache)に存在するか確認
			for (const auto& [filepath, chunk] : seCache) {
				if (chunk == playingChunk) {
					Mix_HaltChannel(i); // 一致した場合のみ、そのチャンネルを停止
					break;
				}
			}
		}
	}

	// SEキャッシュの全解放
	for (auto& [filepath, chunk] : seCache) {
		if (chunk) {
			Mix_FreeChunk(chunk);
		}
	}
	seCache.clear();
}

void AudioPlayer::playSE(const std::string& filepath) {
	if (!details::AudioSystem::is_active()) { throw AudioException("AudioSystem is not active."); }

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
		// チャンネル枯渇時は例外を投げず、再生をスキップする
		// (実行中に突発的に音が重なってもクラッシュさせないための措置)
	}
}

void AudioPlayer::playBGM(const std::string& filepath) {
	if (!details::AudioSystem::is_active()) { throw AudioException("AudioSystem is not active."); }

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
	if (details::AudioSystem::is_active()) {
		Mix_HaltMusic();
	}
}

void AudioPlayer::setVolume(int volume) {
	// 0〜128の範囲にクランプ
	if (volume < 0) { volume = 0; }
	if (volume > 128) { volume = 128; }

	if (details::AudioSystem::is_active()) {
		// 全チャンネルのSE音量設定
		Mix_Volume(-1, volume);
		// BGMの音量設定
		Mix_VolumeMusic(volume);
	}
}

} // namespace sdl
