/**
 * @file tracker_clients.hpp
 * @brief 工具追跡システムおよび録画サーバーと通信するためのクライアントクラス群。
 */
#pragma once

#include <optional>
#include <SimpleGtk>

#include "./url_transfer.hpp"

namespace tracker {

// --- 工具追跡クライアント 定数 ---
constexpr const char* TRACKER_IP	= "10.19.133.122";
constexpr int TRACKER_PORT			= 5000;
constexpr size_t TRACKER_MSG_LENGTH	= 16; // ★★★ 固定メッセージ長 (仮に16バイトとする) ★★★

// 固定長メッセージを生成するヘルパー関数
constexpr std::array<char, TRACKER_MSG_LENGTH> make_fixed_message(const char* msg) {
	std::array<char, TRACKER_MSG_LENGTH> arr{}; // ゼロ初期化
	std::string_view sv(msg);
	// コピーする文字数 = msgの長さ と バッファサイズ-1 (ヌル終端用) の小さい方
	size_t len_to_copy = std::min(sv.length(), TRACKER_MSG_LENGTH - 1);

	// 文字列をコピー
	for (size_t i = 0; i < len_to_copy; i++) {
		arr[i] = msg[i];
	}

	// 残りをスペースでパディング (ヌル終端の1つ手前まで)
	for (size_t i = len_to_copy; i < TRACKER_MSG_LENGTH - 1; i++) {
		arr[i] = ' ';
	}
	// arr[TRACKER_MSG_LENGTH - 1] はゼロ初期化されているので、ヌル終端は保証される

	return arr;
}

// --- プロトコルメッセージ定数 (固定長) ---
constexpr auto TRACKER_MSG_CONNECT		= make_fixed_message("Connect");
constexpr auto TRACKER_MSG_DISCONNECT	= make_fixed_message("Disconnect");
constexpr auto TRACKER_MSG_QR_SCAN		= make_fixed_message("QR_scan");
constexpr auto TRACKER_MSG_ACK			= make_fixed_message("ACK");
constexpr auto TRACKER_MSG_SCAN_S		= make_fixed_message("Scan_s");
constexpr auto TRACKER_MSG_SCAN_F		= make_fixed_message("Scan_f");
constexpr auto TRACKER_MSG_C_ANOMALY_1	= make_fixed_message("C_Anomly_1");
constexpr auto TRACKER_MSG_C_BACK_1		= make_fixed_message("C_Back_1");
constexpr auto TRACKER_MSG_C_ANOMALY_2	= make_fixed_message("C_Anomly_2");
constexpr auto TRACKER_MSG_C_BACK_2		= make_fixed_message("C_Back_2");

/**
 * @brief 工具追跡サーバーからのメッセージ種別
 */
enum class EventType {
	SCAN_SUCCESS,			// スキャン成功
	SCAN_FAIL,				// スキャン失敗
	ANOMALY_UNSCANNED_OUT,	// 未スキャンがフレーム外へ
	BACK_UNSCANNED_IN,		// 未スキャンがフレーム内へ
	ANOMALY_SCANNED_IN,		// スキャン済みがコンテナ内へ
	BACK_SCANNED_OUT,		// スキャン済みがコンテナ外へ
	ACK,					// 接続確認応答
	UNKNOWN,				// 不明なメッセージ
	CONNECTION_LOST,		// 接続切れ
	ERROR					// 受信エラー
};

/**
 * @struct EventNotification
 * @brief 工具追跡サーバーからの通知内容
 */
struct EventNotification {
	EventType type = EventType::UNKNOWN;
	std::array<char, TRACKER_MSG_LENGTH> raw_message{};
	std::string error_details;
};

// --- 録画用クライアント 定数 ---
constexpr const char* RECORDER_BASE_URL		= "http://10.19.106.160:8080";
constexpr const char* RECORDER_CONTEXT_PATH	= "/helloWeb-1.0"; // Tomcatなどのコンテキストパス
constexpr const char* RECORDER_USERNAME		= "yourUsername";
constexpr const char* RECORDER_PASSWORD		= "yourPassword";

// --- インサート個数取得クライアント 定数 ---
constexpr const char* COUNTER_IP	= "127.0.0.1";
constexpr int COUNTER_PORT			= 12345;
constexpr int COUNTER_TIMEOUT_MS	= 5000;

/**
 * @class ToolTrackerClient
 * @brief 工具追跡システムのイベント通知サーバーとのTCP通信を管理するクラス。
 */
class ToolTrackerClient {
public:
	/**
	 * @brief コンストラクタ。
	 * @param[in] callback イベント受信時に呼び出されるコールバック関数。
	 */
	explicit ToolTrackerClient(std::function<void(EventNotification)> callback); // IPとPortを削除
	~ToolTrackerClient();

	// コピー禁止
	ToolTrackerClient(const ToolTrackerClient&) = delete;
	ToolTrackerClient& operator=(const ToolTrackerClient&) = delete;

	/**
	 * @brief サーバーへの接続を試みます。
	 * @return 接続に成功した場合は true。
	 */
	bool connect();

	/**
	 * @brief サーバーから切断します。
	 */
	void disconnect();

	/**
	 * @brief 接続状態を確認します。
	 * @return 接続中の場合は true。
	 */
	bool is_connected();

	/**
	 * @brief QRコードスキャンが行われたことをサーバーに通知します。
	 * @return 送信に成功した場合は true。
	 */
	bool notify_qr_scan(); // qr_data 引数を削除

private:
	/**
	 * @brief サーバーからのメッセージを受信し続けるリスナースレッドの本体。
	 */
	void listener_thread_func();

	sg::Socket socket;
	std::thread listener_thread;
	std::atomic<bool> keep_listening;
	std::function<void(EventNotification)> notification_callback;
};

/**
 * @class RecorderClient
 * @brief 録画サーバーにHTTPリクエストを送信するクラス。
 */
class RecorderClient {
public:
	/**
	 * @brief コンストラクタ。
	 * @details 定義された定数を使用して初期化します。
	 */
	RecorderClient(); // 引数を削除

	/**
	 * @brief 現在時刻に基づいて "recording_YYYYMMDD_HHMMSS.mp4" 形式のファイル名を生成します。
	 * @return 生成されたファイル名文字列。
	 */
	std::string generate_timestamp_filename() const; // ★★★ public に移動 ★★★

	/**
	 * @brief 録画開始コマンドを送信します。
	 * @param[in] filename (オプション) 録画ファイル名を指定します。
	 * 空の場合、タイムスタンプから自動生成されます。
	 * @return サーバーが成功応答 (HTTP 2xx) を返し、
	 * かつファイル名が確定した場合、そのファイル名を std::optional で返します。
	 * 失敗した場合は std::nullopt を返します。
	 */
	std::optional<std::string> start_recording(std::string filename = "");

	/**
	 * @brief 録画停止コマンドを送信します。
	 * @return サーバーが成功応答 (HTTP 2xx) を返した場合は true。
	 */
	bool stop_recording();

private:
	std::string base_url;
	std::string context_path;
	std::string username;
	std::string password;
};

/**
 * @brief インサート個数取得サーバーに接続し、個数を取得して切断します。
 * @param[in] ip (オプション) サーバーIPアドレス。デフォルトは COUNTER_IP 定数。
 * @param[in] port (オプション) サーバーポート番号。デフォルトは COUNTER_PORT 定数。
 * @param[in] timeout_ms (オプション) 受信タイムアウト(ミリ秒)。デフォルトは COUNTER_TIMEOUT_MS 定数。
 * @return 取得した個数。失敗した場合は std::nullopt。
 */
std::optional<int> get_insert_count(
	const char* ip = COUNTER_IP,
	int port = COUNTER_PORT,
	int timeout_ms = COUNTER_TIMEOUT_MS
);

} // namespace tracker
