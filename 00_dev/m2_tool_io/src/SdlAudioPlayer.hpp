#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

namespace sdl {

// オーディオ関連の例外クラス
class AudioException : public std::runtime_error {
public:
	explicit AudioException(const std::string& message)
		: std::runtime_error(message) {}
};

class AudioPlayer {
private:
	// SEのキャッシュ (オンメモリ)
	std::unordered_map<std::string, Mix_Chunk*> seCache;

	// 現在再生中のBGM (ストリーミング)
	Mix_Music* currentBgm;

	bool initialized;

	// 内部ヘルパー: リソースの解放
	void clearResources();

public:
	/**
	 * コンストラクタ
	 * @param[in] flags (オプション・デフォルト 0) Mix_Init用フラグ (例: MIX_INIT_OGG | MIX_INIT_MP3)。0なら何もしない。
	 * @param[in] frequency (オプション・デフォルト 44100) 周波数 (Hz)。
	 * @param[in] format (オプション・デフォルト MIX_DEFAULT_FORMAT(16bit)) 音声フォーマット。
	 * @param[in] channels (オプション・デフォルト 2) チャンネル数。2=ステレオ, 1=モノラル。
	 * @param[in] chunksize (オプション・デフォルト 2048) バッファサイズ。小さいと低遅延だがCPU負荷増。2の乗数を指定。
	 */
	AudioPlayer(
		int flags = 0,
		int frequency = 44100,
		Uint16 format = MIX_DEFAULT_FORMAT,
		int channels = 2,
		int chunksize = 2048
	);
	~AudioPlayer();

	// コピー・ムーブを完全に禁止（リソースの二重解放防止）
	AudioPlayer(const AudioPlayer&) = delete;
	AudioPlayer& operator=(const AudioPlayer&) = delete;
	AudioPlayer(AudioPlayer&&) = delete;
	AudioPlayer& operator=(AudioPlayer&&) = delete;

	/**
	 * 効果音(SE)を再生する
	 * 短い音声用。初回再生時にメモリにキャッシュされます。
	 * @param[in] filepath 音声ファイルのパス
	 */
	void playSE(const std::string& filepath);

	/**
	 * BGMを再生する
	 * 長い音声用。ストリーミング再生され、メモリを圧迫しません。
	 * ループ再生されます。
	 * @param[in] filepath 音声ファイルのパス
	 */
	void playBGM(const std::string& filepath);

	/**
	 * BGMを停止する
	 */
	void stopBGM();

	/**
	 * 音量を設定する (0〜128)
	 * @param[in] volume 音量
	 */
	void setVolume(int volume);
};

} // namespace SdlWrapper
