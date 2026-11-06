/*
 * ATP3011 読み上げライブラリ（ヘッダ）
 *
 * クラス一覧（単一責任の原則に基づく分離）
 *   ScopedFd            : ファイル記述子のRAIIラッパ（close漏れ防止）
 *   SignalStopGuard     : SIGINT/SIGTERMで停止フラグを立てる（デストラクタで元の設定に復元）
 *   SysfsGpio           : sysfs GPIOの export / direction / value 設定
 *   SerialPort          : UARTの open / 9600bps 8N1設定 / 送信 / 応答監視
 *   AudioStream         : arecord -> パイプ -> aplay の起動・停止（デストラクタで必ず回収）
 *   Atp3011Speaker      : 上記を組み合わせた読み上げ処理全体の統括
 *   SpeechFormatter     : データシート仕様に基づくテキスト自動分類・タグ付与機能
 *
 * 要件: C++17 / Linux(POSIX)
 */
#ifndef ATP3011_ATP3011_HPP
#define ATP3011_ATP3011_HPP

#include <sys/types.h>

#include <atomic>
#include <csignal>
#include <functional>
#include <string>

namespace atp3011 {

// ---------------------------------------------------------------------------
// 設定値（元コードのハードコード値をデフォルトとして保持）
// ---------------------------------------------------------------------------
struct AudioConfig {
    std::string recordDevice   = "hw:1,1";   // arecord -D
    std::string playbackDevice = "hw:0,0";   // aplay   -D
    std::string format         = "S16_LE";   // -f
    int         sampleRate     = 44100;      // -r
    int         channels       = 2;          // -c
};

struct Config {
    int         gpioNumber           = 332;
    std::string gpioBasePath         = "/sys/class/gpio";
    std::string uartDevice           = "/dev/ttyUL0";
    int         probeTimeoutMs       = 1000;  // '?' 送信後の応答確認時間
    int         audioStartupDelayMs  = 1000;   // 音声デバイス起動待ち
    int         responsePollMs       = 100;   // 読み上げ完了待ちのpoll周期
    bool        installSignalHandlers = true; // SIGINT/SIGTERMを本ライブラリで扱うか
    AudioConfig audio;
};

// ===========================================================================
// ScopedFd
// ===========================================================================
/**
 * @class ScopedFd
 * @brief ファイル記述子（File Descriptor）の寿命を管理するRAIIラッパクラス。
 *
 * デストラクタで自動的に ::close() を呼び出すことで、ファイル記述子のリークを防止します。
 * コピーは禁止されていますが、ムーブによる所有権の移転が可能です。
 */
class ScopedFd {
public:
    ScopedFd() noexcept = default;
    explicit ScopedFd(int fd) noexcept : fd_(fd) {}
    ~ScopedFd() { reset(); }

    ScopedFd(const ScopedFd&)            = delete;
    ScopedFd& operator=(const ScopedFd&) = delete;
    ScopedFd(ScopedFd&& other) noexcept : fd_(other.release()) {}
    ScopedFd& operator=(ScopedFd&& other) noexcept {
        if (this != &other)
            reset(other.release());
        return *this;
    }

    int  get() const noexcept   { return fd_; }
    bool valid() const noexcept { return fd_ >= 0; }
    int  release() noexcept;
    void reset(int fd = -1) noexcept;

private:
    int fd_ = -1;
};

// ===========================================================================
// SignalStopGuard
// ===========================================================================
/**
 * @class SignalStopGuard
 * @brief SIGINT (Ctrl+C) や SIGTERM などの終了シグナルを安全に捕捉・管理するクラス。
 *
 * インスタンス生成時にシグナルハンドラを登録し、破棄時に元のハンドラ設定へと自動復元します。
 * プロセス共通の停止フラグを制御するため、同時に複数のインスタンスを有効にしないよう注意してください。
 */
class SignalStopGuard {
public:
    SignalStopGuard();
    ~SignalStopGuard();

    SignalStopGuard(const SignalStopGuard&)            = delete;
    SignalStopGuard& operator=(const SignalStopGuard&) = delete;

    /// @brief シグナルが要求されたかどうかを返す（スレッドセーフ）
    static bool requested() noexcept;

private:
    static void handler(int);

    struct sigaction oldInt_{};
    struct sigaction oldTerm_{};
    bool             installed_ = false;
};

// ===========================================================================
// SysfsGpio
// ===========================================================================
/**
 * @class SysfsGpio
 * @brief Linuxの sysfs (/sys/class/gpio) インターフェースを通じてGPIO制御を行うクラス。
 *
 * 指定されたGPIO番号の export、方向設定（out）、およびHIGH（1）出力の設定を安全に行います。
 */
class SysfsGpio {
public:
    SysfsGpio(int number, std::string basePath);

    /// @brief GPIOを出力モードに設定し、HIGH(1)にする（未exportなら自動exportを実行）
    bool setupOutputHigh();

    /// @brief sysfsの指定パスへ文字列を書き込む共通ヘルパー関数
    static bool writeFile(const std::string& path, const std::string& value);

private:
    std::string gpioDir() const;

    int         number_;
    std::string basePath_;
};

// ===========================================================================
// SerialPort
// ===========================================================================
/**
 * @class SerialPort
 * @brief UARTシリアル通信を制御するクラス。
 *
 * POSIXターミナル制御（termios）を用いた生データモード（9600bps, 8N1）の設定、
 * テキスト送信（tcdrain）、および `poll()` を活用したタイムアウト付き受信監視を行います。
 */
class SerialPort {
public:
    using StopPredicate = std::function<bool()>;

    SerialPort() = default;
    ~SerialPort() = default;  // ScopedFdによって安全にクローズされる

    SerialPort(const SerialPort&)            = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    bool open(const std::string& device);   // O_RDWR | O_NOCTTY
    void close() noexcept;
    bool isOpen() const noexcept { return fd_.valid(); }

    bool configure();                       // 9600 bps, 8N1設定
    bool sendText(const std::string& text); // 文字列を最後まで送信し tcdrain を実行
    bool checkResponse(int timeoutMs);      // 指定時間だけ応答の有無を確認・表示

    /// @brief 何らかの応答を受信するまで待機する（stop述語が真を返した場合は中断）
    bool waitForResponse(const StopPredicate& stop, int pollMs);
    void flushInput() noexcept;             // tcflush(TCIFLUSH)

private:
    enum class PollResult { Readable, Timeout, Interrupted, Error };
    PollResult pollReadable(int timeoutMs) const;

    ScopedFd fd_;
};

// ===========================================================================
// AudioStream
// ===========================================================================
/**
 * @class AudioStream
 * @brief `arecord`（録音）と `aplay`（再生）をパイプで直結して実行するバックグラウンド音声ストリーム管理クラス。
 *
 * デストラクタで未停止の子プロセスに対して自動的に `SIGTERM` を送信し、
 * `waitpid` で確実に回収することでゾンビプロセスの発生を防止します。
 */
class AudioStream {
public:
    explicit AudioStream(AudioConfig config = AudioConfig{});
    ~AudioStream();

    AudioStream(const AudioStream&)            = delete;
    AudioStream& operator=(const AudioStream&) = delete;

    bool start();
    void stop() noexcept;
    bool running() const noexcept { return recorder_ > 0 || player_ > 0; }

private:
    AudioConfig config_;
    pid_t       recorder_ = -1;
    pid_t       player_   = -1;
};

// ===========================================================================
// Atp3011Speaker
// ===========================================================================
/**
 * @class Atp3011Speaker
 * @brief GPIO初期化、UART通信、音声ストリームを統合し、音声合成LSI（ATP3011）の読み上げ処理全体を統括するクラス。
 *
 * 指定された文字列の送信から完了応答の待機までの一連のシーケンスをブロッキングで実行します。
 */
class Atp3011Speaker {
public:
    enum class Result {
        Completed,   // 応答を受信し正常終了
        Incomplete,  // 停止要求・通信エラー等で完了前に終了
        Failed       // 初期化・送信失敗
    };

    explicit Atp3011Speaker(Config config = Config{});

    Atp3011Speaker(const Atp3011Speaker&)            = delete;
    Atp3011Speaker& operator=(const Atp3011Speaker&) = delete;

    /// @brief 指定テキストを読み上げる（ブロッキング処理。1インスタンスにつき同時実行は1つまで）
    Result speak(const std::string& text);

    /// @brief 他スレッドから安全に読み上げ停止を要求する（スレッドセーフ）
    void requestStop() noexcept { stop_.store(true); }

private:
    bool stopRequested() const noexcept;

    Config           config_;
    std::atomic<bool> stop_{false};
};

// ===========================================================================
// SpeechFormatter
// ===========================================================================
/**
 * @class SpeechFormatter
 * @brief ATP3011データシート（14章）の仕様に基づくテキスト自動分類・タグ付与クラス。
 *
 * 入力された任意の文字列から「数値」「小文字ローマ字」「大文字を含む英字（型番など）」を
 * 自動的に認識・分離し、ATP3011に最適なタグ（<NUMK>, <ALPHA>）を付与した音声記号列を生成します。
 */
class SpeechFormatter {
public:
    // 静的ユーティリティクラスのためインスタンス化を禁止
    SpeechFormatter() = delete;
    ~SpeechFormatter() = delete;

    SpeechFormatter(const SpeechFormatter&)            = delete;
    SpeechFormatter& operator=(const SpeechFormatter&) = delete;
    SpeechFormatter(SpeechFormatter&&)                 = delete;
    SpeechFormatter& operator=(SpeechFormatter&&)      = delete;

    /**
     * @brief 英数字文字列を ATP3011 の英数読みタグに変換する
     */
    static std::string toAlphaTag(const std::string& text);

    /**
     * @brief 数値を ATP3011 の桁読みタグに変換する（助数詞オプション対応）
     */
    static std::string toNumkTag(const std::string& numberStr, const std::string& counter = "");

    /**
     * @brief 数値を ATP3011 の棒読みタグに変換する
     */
    static std::string toNumTag(const std::string& numberStr);

    /**
     * @brief 入力文字列を解析し、「数字」「小文字ローマ字」「大文字英数」のグループに自動分解・変換する
     */
    static std::string formatForAtp3011(const std::string& input);
};

} // namespace atp3011

#endif // ATP3011_ATP3011_HPP
