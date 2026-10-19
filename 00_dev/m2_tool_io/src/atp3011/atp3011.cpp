/*
 * ATP3011 読み上げライブラリ（実装）
 */
#include "atp3011.hpp"

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

namespace atp3011 {

// ===========================================================================
// 内部ヘルパ（このファイル内のみ）
// ===========================================================================
namespace {

// シグナルハンドラから更新する停止フラグ（元の stop_requested）
volatile std::sig_atomic_t g_stopRequested = 0;

constexpr std::size_t kRxBufferSize = 256;

bool isPrintableAscii(unsigned char c)
{
    return c >= 32 && c <= 126;
}

// 受信データの簡易表示（元 check_uart_response の表示形式）
std::string formatCompact(const unsigned char* buf, std::size_t n)
{
    std::ostringstream os;
    for (std::size_t i = 0; i < n; ++i) {
        if (isPrintableAscii(buf[i]))
            os << static_cast<char>(buf[i]);
        else
            os << "<0x" << std::hex << static_cast<int>(buf[i])
               << std::dec << ">";
    }
    return os.str();
}

// ASCII表示（元 wait_for_speech_end の表示形式）
std::string formatAscii(const unsigned char* buf, std::size_t n)
{
    std::string s;
    for (std::size_t i = 0; i < n; ++i) {
        if (isPrintableAscii(buf[i]))
            s += static_cast<char>(buf[i]);
        else if (buf[i] == '\r')
            s += "<CR>";
        else if (buf[i] == '\n')
            s += "<LF>";
        else
            s += "<NONPRINT>";
    }
    return s;
}

// 16進数表示（元 wait_for_speech_end の表示形式）
std::string formatHex(const unsigned char* buf, std::size_t n)
{
    std::ostringstream os;  // ローカルなのでstd::coutの書式状態を汚さない
    os << std::hex << std::uppercase << std::setfill('0');
    for (std::size_t i = 0; i < n; ++i)
        os << std::setw(2) << static_cast<unsigned int>(buf[i]) << ' ';
    return os.str();
}

// 子プロセスの終了を待つ（EINTR対応）。待機後はpidを-1に戻す。
void waitAndClear(pid_t& pid) noexcept
{
    if (pid > 0) {
        while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {}
        pid = -1;
    }
}

// fork して指定コマンドへ置き換える。
//   keepFd  : targetFd（STDIN/STDOUT）へ複製して使うパイプ端
//   closeFd : 子で不要なパイプ端
// 戻り値: 親では子のPID（失敗時 -1）。子はexec成功で戻らず、失敗時は _exit する。
// argv は fork 前に親で組み立て、子では確保処理をしない（マルチスレッド環境での安全性のため）。
pid_t spawnPipedChild(int keepFd, int closeFd, int targetFd,
                      const std::vector<std::string>& args)
{
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const std::string& a : args)
        argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid != 0)
        return pid;  // 親（pid>0）または fork失敗（pid<0）

    // --- 子プロセス ---
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);

    close(closeFd);

    if (keepFd != targetFd) {
        if (dup2(keepFd, targetFd) < 0)
            _exit(126);
        close(keepFd);
    }

    execvp(argv[0], argv.data());

    std::cerr << "Failed to start " << argv[0] << ": "
              << std::strerror(errno) << "\n";
    _exit(127);
}

} // namespace

// ===========================================================================
// ScopedFd
// ===========================================================================
int ScopedFd::release() noexcept
{
    int fd = fd_;
    fd_ = -1;
    return fd;
}

void ScopedFd::reset(int fd) noexcept
{
    if (fd_ >= 0)
        ::close(fd_);
    fd_ = fd;
}

// ===========================================================================
// SignalStopGuard
// ===========================================================================
void SignalStopGuard::handler(int)
{
    g_stopRequested = 1;
}

bool SignalStopGuard::requested() noexcept
{
    return g_stopRequested != 0;
}

SignalStopGuard::SignalStopGuard()
{
    g_stopRequested = 0;

    struct sigaction sa{};
    sa.sa_handler = &SignalStopGuard::handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, &oldInt_) != 0)
        return;
    if (sigaction(SIGTERM, &sa, &oldTerm_) != 0) {
        sigaction(SIGINT, &oldInt_, nullptr);  // 片方だけ登録された状態を残さない
        return;
    }
    installed_ = true;
}

SignalStopGuard::~SignalStopGuard()
{
    if (installed_) {
        sigaction(SIGINT, &oldInt_, nullptr);
        sigaction(SIGTERM, &oldTerm_, nullptr);
    }
}

// ===========================================================================
// SysfsGpio
// ===========================================================================
SysfsGpio::SysfsGpio(int number, std::string basePath)
    : number_(number), basePath_(std::move(basePath))
{
}

std::string SysfsGpio::gpioDir() const
{
    return basePath_ + "/gpio" + std::to_string(number_);
}

bool SysfsGpio::writeFile(const std::string& path, const std::string& value)
{
    int fd = open(path.c_str(), O_WRONLY);
    if (fd < 0) {
        std::cerr << "open " << path << ": "
                  << std::strerror(errno) << "\n";
        return false;
    }
    ScopedFd guard(fd);

    ssize_t n = write(guard.get(), value.data(), value.size());
    int savedErrno = errno;
    guard.reset();

    if (n != static_cast<ssize_t>(value.size())) {
        std::cerr << "write " << path << ": "
                  << std::strerror(savedErrno) << "\n";
        return false;
    }
    return true;
}

bool SysfsGpio::setupOutputHigh()
{
    struct stat st{};
    const std::string dir = gpioDir();

    if (stat(dir.c_str(), &st) != 0) {
        if (!writeFile(basePath_ + "/export", std::to_string(number_))) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (stat(dir.c_str(), &st) != 0) {
                std::cerr << "GPIO" << number_ << " export failed\n";
                return false;
            }
        }
    }

    if (!writeFile(dir + "/direction", "out"))
        return false;

    if (!writeFile(dir + "/value", "1"))
        return false;

    std::cout << "GPIO" << number_ << ": output, HIGH\n";
    return true;
}

// ===========================================================================
// SerialPort
// ===========================================================================
bool SerialPort::open(const std::string& device)
{
    if (fd_.valid()) {
        std::cerr << "open " << device << ": already opened\n";
        return false;
    }

    int fd = ::open(device.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0) {
        std::cerr << "open " << device << ": "
                  << std::strerror(errno) << "\n";
        return false;
    }
    fd_.reset(fd);
    return true;
}

void SerialPort::close() noexcept
{
    fd_.reset();
}

bool SerialPort::configure()
{
    struct termios tty{};

    if (tcgetattr(fd_.get(), &tty) != 0) {
        std::cerr << "tcgetattr: " << std::strerror(errno) << "\n";
        return false;
    }

    // 端末の行編集や文字変換などを無効にし、データをそのまま送受信する設定にする。
    cfmakeraw(&tty);

    if (cfsetispeed(&tty, B9600) != 0 ||
        cfsetospeed(&tty, B9600) != 0) {
        std::cerr << "set baud rate failed\n";
        return false;
    }

    tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    // 8ビットデータ、受信有効、モデム制御線を使わない設定にする。
    tty.c_cflag |= CS8 | CREAD | CLOCAL;

#ifdef CRTSCTS
    tty.c_cflag &= ~CRTSCTS;
#endif

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    // read()で最低受信バイト数を待たない設定にする。
    tty.c_cc[VMIN] = 0;
    // termios側の読み取りタイムアウトは設定せず、poll()で受信待ちを制御する。
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd_.get(), TCSANOW, &tty) != 0) {
        std::cerr << "tcsetattr: " << std::strerror(errno) << "\n";
        return false;
    }

    tcflush(fd_.get(), TCIOFLUSH);
    std::cout << "UART: 9600 baud, 8N1\n";
    return true;
}

bool SerialPort::sendText(const std::string& text)
{
    size_t sent = 0;

    while (sent < text.size()) {
        ssize_t n = write(fd_.get(), text.data() + sent, text.size() - sent);

        if (n < 0 && errno == EINTR)
            continue;

        if (n <= 0) {
            std::cerr << "UART write failed: "
                      << std::strerror(errno) << "\n";
            return false;
        }

        sent += static_cast<size_t>(n);
    }

    // UARTの送信キューに残っているデータが送信されるまで待つ。
    tcdrain(fd_.get());
    return true;
}

SerialPort::PollResult SerialPort::pollReadable(int timeoutMs) const
{
    struct pollfd pfd{};
    pfd.fd = fd_.get();
    pfd.events = POLLIN;

    int r = poll(&pfd, 1, timeoutMs);

    if (r < 0)
        return errno == EINTR ? PollResult::Interrupted : PollResult::Error;
    if (r == 0)
        return PollResult::Timeout;
    if (pfd.revents & POLLIN)
        return PollResult::Readable;

    // POLLERR/POLLHUP/POLLNVALのみ：無限ループ(ビジーループ)にならないようエラー扱いにする。
    errno = EIO;
    return PollResult::Error;
}

bool SerialPort::checkResponse(int timeoutMs)
{
    auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeoutMs);

    bool received = false;

    while (std::chrono::steady_clock::now() < deadline) {
        auto remaining =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now()
            ).count();

        switch (pollReadable(static_cast<int>(remaining))) {
        case PollResult::Interrupted:
            continue;
        case PollResult::Error:
            std::cerr << "UART poll failed\n";
            return received;
        case PollResult::Timeout:
            return received;
        case PollResult::Readable:
            break;
        }

        unsigned char buf[kRxBufferSize];
        ssize_t n = read(fd_.get(), buf, sizeof(buf));

        if (n > 0) {
            received = true;
            std::cout << "UART RX: "
                      << formatCompact(buf, static_cast<std::size_t>(n))
                      << "\n";
        }
    }

    return received;
}

bool SerialPort::waitForResponse(const StopPredicate& stop, int pollMs)
{
    std::cout << "Waiting for UART response...\n";

    while (!(stop && stop())) {
        switch (pollReadable(pollMs)) {
        case PollResult::Interrupted:
        case PollResult::Timeout:
            continue;
        case PollResult::Error:
            std::cerr << "UART poll failed: "
                      << std::strerror(errno) << "\n";
            return false;
        case PollResult::Readable:
            break;
        }

        unsigned char buf[kRxBufferSize];
        ssize_t n = read(fd_.get(), buf, sizeof(buf));

        if (n < 0 && errno == EINTR)
            continue;

        if (n < 0) {
            std::cerr << "UART read failed: "
                      << std::strerror(errno) << "\n";
            return false;
        }

        if (n == 0)
            continue;

        const std::size_t len = static_cast<std::size_t>(n);

        std::cout << "\nReceived " << n << " byte(s)\n";
        std::cout << "ASCII: " << formatAscii(buf, len) << "\n";
        std::cout << "HEX:   " << formatHex(buf, len) << "\n";
        std::cout << "Any UART response received. Stopping.\n";
        return true;
    }

    std::cout << "Stop requested.\n";
    return false;
}

void SerialPort::flushInput() noexcept
{
    if (fd_.valid())
        tcflush(fd_.get(), TCIFLUSH);
}

// ===========================================================================
// AudioStream
// ===========================================================================
AudioStream::AudioStream(AudioConfig config)
    : config_(std::move(config))
{
}

AudioStream::~AudioStream()
{
    stop();
}

bool AudioStream::start()
{
    if (running()) {
        std::cerr << "Audio streaming already started\n";
        return false;
    }

    // fds[0]が読み取り側、fds[1]が書き込み側。録音データの受け渡しに使う。
    int fds[2];

    if (pipe(fds) != 0) {
        std::cerr << "pipe: " << std::strerror(errno) << "\n";
        return false;
    }
    ScopedFd readEnd(fds[0]);
    ScopedFd writeEnd(fds[1]);

    const std::string rate = std::to_string(config_.sampleRate);
    const std::string ch   = std::to_string(config_.channels);

    // 録音側：音声データをパイプへ送る
    recorder_ = spawnPipedChild(
        writeEnd.get(), readEnd.get(), STDOUT_FILENO,
        {"arecord",
         "-D", config_.recordDevice,
         "-f", config_.format,
         "-r", rate,
         "-c", ch,
         "-t", "raw"});

    if (recorder_ < 0) {
        std::cerr << "fork arecord failed\n";
        return false;  // パイプ両端はScopedFdが閉じる
    }

    // 再生側：パイプから音声データを受け取る
    player_ = spawnPipedChild(
        readEnd.get(), writeEnd.get(), STDIN_FILENO,
        {"aplay",
         "-D", config_.playbackDevice,
         "-t", "raw",
         "-f", config_.format,
         "-r", rate,
         "-c", ch,
         "-"});

    if (player_ < 0) {
        std::cerr << "fork aplay failed\n";
        readEnd.reset();
        writeEnd.reset();
        kill(recorder_, SIGTERM);
        waitAndClear(recorder_);
        return false;
    }

    readEnd.reset();
    writeEnd.reset();

    std::cout << "Audio streaming started\n";
    return true;
}

void AudioStream::stop() noexcept
{
    if (!running())
        return;

    if (recorder_ > 0)
        kill(recorder_, SIGTERM);

    if (player_ > 0)
        kill(player_, SIGTERM);

    waitAndClear(recorder_);
    waitAndClear(player_);

    std::cout << "\nAudio streaming stopped\n";
}

// ===========================================================================
// Atp3011Speaker
// ===========================================================================
Atp3011Speaker::Atp3011Speaker(Config config)
    : config_(std::move(config))
{
}

bool Atp3011Speaker::stopRequested() const noexcept
{
    return stop_.load() || SignalStopGuard::requested();
}

Atp3011Speaker::Result Atp3011Speaker::speak(const std::string& text)
{
    if (text.empty()) {
        std::cerr << "Speech text is empty\n";
        return Result::Failed;
    }

    stop_.store(false);

    // Ctrl+Cや終了シグナルを受けたら停止フラグを立てる（関数を抜けると元に戻る）
    std::optional<SignalStopGuard> signalGuard;
    if (config_.installSignalHandlers)
        signalGuard.emplace();

    // 1. GPIO設定
    SysfsGpio gpio(config_.gpioNumber, config_.gpioBasePath);
    if (!gpio.setupOutputHigh())
        return Result::Failed;

    // 破棄順序: serial(close) -> audio(stop)  ※元コードの後片付け順と同じ
    AudioStream audio(config_.audio);
    SerialPort  serial;

    // 2. UART設定
    if (!serial.open(config_.uartDevice))
        return Result::Failed;

    if (!serial.configure())
        return Result::Failed;

    // 3. 通信確認：「?」を送信し、UARTの相手から応答があるかを確認する。
    std::cout << "Sending '?'\n";

    if (!serial.sendText("?"))
        return Result::Failed;

    if (!serial.checkResponse(config_.probeTimeoutMs))
        std::cout << "No response to '?'; continuing\n";

    serial.flushInput();

    // 4. 録音・再生を開始
    if (!audio.start())
        return Result::Failed;

    // 音声デバイスが起動する時間を少し確保
    std::this_thread::sleep_for(
        std::chrono::milliseconds(config_.audioStartupDelayMs));

    // 5. 指定文字列を送信（従来どおり末尾にCRを付ける）
    std::cout << "Speaking: " << text << "\n";

    if (!serial.sendText(text + "\r"))
        return Result::Failed;

    // UARTを閉じず、読み上げ完了の応答を待つ
    const bool completed = serial.waitForResponse(
        [this] { return stopRequested(); }, config_.responsePollMs);

    // UARTを閉じ、録音・再生プロセスを終了
    serial.close();
    audio.stop();

    if (completed) {
        std::cout << "Finished successfully!\n";
        return Result::Completed;
    }

    std::cout << "Stopped before completion.\n";
    return Result::Incomplete;
}

} // namespace atp3011
