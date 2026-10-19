/*
 * ATP3011 読み上げライブラリ（ヘッダ）
 *
 * atp3011_test_5_commented.cpp をクラス化したもの。
 *
 * クラス一覧（単一責任）
 *   ScopedFd            : ファイル記述子のRAIIラッパ（close漏れ防止）
 *   SignalStopGuard     : SIGINT/SIGTERMで停止フラグを立てる（デストラクタで元の設定に復元）
 *   SysfsGpio           : sysfs GPIOの export / direction / value 設定
 *   SerialPort          : UARTの open / 9600bps 8N1設定 / 送信 / 応答監視
 *   AudioStream         : arecord -> パイプ -> aplay の起動・停止（デストラクタで必ず回収）
 *   Atp3011Speaker      : 上記を組み合わせた読み上げ処理全体の統括
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
    int         audioStartupDelayMs  = 100;   // 音声デバイス起動待ち
    int         responsePollMs       = 100;   // 読み上げ完了待ちのpoll周期
    bool        installSignalHandlers = true; // SIGINT/SIGTERMを本ライブラリで扱うか
    AudioConfig audio;
};

// ---------------------------------------------------------------------------
// ScopedFd : ファイル記述子のRAII（コピー不可・ムーブ可）
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// SignalStopGuard : 停止シグナル受信フラグ（元の stop_requested / handle_signal）
//   生成時にフラグをクリアしてハンドラを登録し、破棄時に元のハンドラへ戻す。
//   フラグはプロセス共通（シグナルハンドラの性質上）なので、同時に複数の
//   インスタンスを有効にしないこと。
// ---------------------------------------------------------------------------
class SignalStopGuard {
public:
    SignalStopGuard();
    ~SignalStopGuard();

    SignalStopGuard(const SignalStopGuard&)            = delete;
    SignalStopGuard& operator=(const SignalStopGuard&) = delete;

    static bool requested() noexcept;

private:
    static void handler(int);

    struct sigaction oldInt_{};
    struct sigaction oldTerm_{};
    bool             installed_ = false;
};

// ---------------------------------------------------------------------------
// SysfsGpio : sysfs GPIO制御（元の write_sysfs / setup_gpio）
// ---------------------------------------------------------------------------
class SysfsGpio {
public:
    SysfsGpio(int number, std::string basePath);

    // 出力に設定し、HIGH(1)にする。未exportならexportしてから設定する。
    bool setupOutputHigh();

    // sysfsファイルへ文字列を書き込む共通関数。
    static bool writeFile(const std::string& path, const std::string& value);

private:
    std::string gpioDir() const;

    int         number_;
    std::string basePath_;
};

// ---------------------------------------------------------------------------
// SerialPort : UART制御（元の configure_serial / send_text /
//              check_uart_response / wait_for_speech_end）
// ---------------------------------------------------------------------------
class SerialPort {
public:
    using StopPredicate = std::function<bool()>;

    SerialPort() = default;
    ~SerialPort() = default;  // ScopedFdが閉じる

    SerialPort(const SerialPort&)            = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    bool open(const std::string& device);   // O_RDWR | O_NOCTTY
    void close() noexcept;
    bool isOpen() const noexcept { return fd_.valid(); }

    bool configure();                       // 9600 bps, 8N1
    bool sendText(const std::string& text); // 最後まで送信し、tcdrainする
    bool checkResponse(int timeoutMs);      // 指定時間だけ受信を確認・表示
    // 何らかの応答を受信するまで待機。stopが真を返したら false で抜ける。
    bool waitForResponse(const StopPredicate& stop, int pollMs);
    void flushInput() noexcept;             // tcflush(TCIFLUSH)

private:
    enum class PollResult { Readable, Timeout, Interrupted, Error };
    PollResult pollReadable(int timeoutMs) const;

    ScopedFd fd_;
};

// ---------------------------------------------------------------------------
// AudioStream : arecord | aplay（元の start_audio_stream / stop_audio_stream）
//   デストラクタで未停止の子プロセスを必ず終了・回収する（ゾンビ防止）。
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Atp3011Speaker : 読み上げ処理全体（元の main の処理フロー）
// ---------------------------------------------------------------------------
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

    // textを読み上げる。ブロッキング。1インスタンスにつき同時実行は1つまで。
    Result speak(const std::string& text);

    // 他スレッドから停止を要求する（スレッドセーフ）。speak()開始時にクリアされる。
    void requestStop() noexcept { stop_.store(true); }

private:
    bool stopRequested() const noexcept;

    Config           config_;
    std::atomic<bool> stop_{false};
};

} // namespace atp3011

#endif // ATP3011_ATP3011_HPP
