/**
 * @file audio_player.hpp
 * @brief SDL2_mixerを使用して、BGMとSEの再生を管理するAudioPlayerクラスと、そのライフサイクルを管理するシステムクラスの宣言。
 */
#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

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

/**
 * @class AudioSystem
 * @brief SDLのオーディオサブシステムとSDL2_mixerの初期化・終了を管理する専任のシングルトンクラス。
 * @details プログラム内で最初にAudioPlayerがインスタンス化された際に自動的に初期化され、
 * アプリケーション終了時に安全にリソースを解放します。
 */
class AudioSystem {
private:
	// C++17: システムが生存しているかどうかのフラグ
	static inline bool is_active_ = false;

	/**
	 * @brief プライベートコンストラクタ。内部でグローバルな初期化を行います。
	 */
	AudioSystem(int flags, int frequency, Uint16 format, int channels, int chunksize);

	/**
	 * @brief プライベートデストラクタ。内部でグローバルな終了処理を行います。
	 */
	~AudioSystem();

public:
	// コピー・ムーブを完全に禁止
	AudioSystem(const AudioSystem&) = delete;
	AudioSystem& operator=(const AudioSystem&) = delete;
	AudioSystem(AudioSystem&&) = delete;
	AudioSystem& operator=(AudioSystem&&) = delete;

	/**
	 * @brief システムの唯一のインスタンスを取得し、初期化を保証します。
	 * @note プログラム全体での初回呼び出し時のみ引数が適用されます。2回目以降の呼び出しでは引数は無視されます。
	 */
	static AudioSystem& initialize(
		int flags = 0,
		int frequency = 44100,
		Uint16 format = MIX_DEFAULT_FORMAT,
		int channels = 2,
		int chunksize = 2048
	);

	/**
	 * @brief オーディオシステムが現在有効（初期化済みかつ未破棄）かどうかを確認します。
	 */
	static bool is_active() noexcept {
		return is_active_;
	}
};

} // namespace details

/**
 * @class AudioException
 * @brief オーディオ関連の例外クラス。
 */
class AudioException : public std::runtime_error {
public:
	explicit AudioException(const std::string& message)
		: std::runtime_error(message) {}
};

/**
 * @class AudioPlayer
 * @brief SDL2_mixerを使用して、BGMとSEの再生を管理するクラス。
 * @details 複数インスタンス化したり、グローバルに配置しても安全に動作するように設計されています。
 */
class AudioPlayer {
private:
	// SEのキャッシュ (オンメモリ)
	std::unordered_map<std::string, Mix_Chunk*> seCache;

	// 現在再生中のBGM (ストリーミング)
	Mix_Music* currentBgm;

	// 内部ヘルパー: リソースの解放
	void clearResources();

public:
	/**
	 * @brief コンストラクタ。
	 * @details 初回生成時に AudioSystem が自動的に初期化されます。
	 * @note プログラム全体での初回呼び出し時のみ引数が適用されます。2回目以降の呼び出しでは引数は無視されます。
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

	/**
	 * @brief デストラクタ。
	 * @details グローバル終了処理の巻き込みを避けるため、システムが生きている場合のみリソースを解放します。
	 */
	~AudioPlayer();

	// コピー・ムーブを完全に禁止（リソースの二重解放防止）
	AudioPlayer(const AudioPlayer&) = delete;
	AudioPlayer& operator=(const AudioPlayer&) = delete;
	AudioPlayer(AudioPlayer&&) = delete;
	AudioPlayer& operator=(AudioPlayer&&) = delete;

	/**
	 * @brief 効果音(SE)を再生する。
	 * \n
	 * 短い音声用。初回再生時にメモリにキャッシュされます。
	 * @note 同時再生数の上限に達した場合、再生はスキップされます。
	 * @param[in] filepath 音声ファイルのパス
	 */
	void playSE(const std::string& filepath);

	/**
	 * @brief BGMをループ再生する。
	 * \n
	 * 長い音声用。ストリーミング再生され、メモリを圧迫しません。
	 * @param[in] filepath 音声ファイルのパス
	 */
	void playBGM(const std::string& filepath);

	/**
	 * @brief BGMを停止する。
	 */
	void stopBGM();

	/**
	 * @brief 音量を設定する (0〜128)。
	 * @param[in] volume 音量
	 */
	void setVolume(int volume);
};

} // namespace sdl
