/**
 * @file tracker_clients.hpp
 * @brief 工具追跡システムおよび録画サーバーと通信するためのクライアントクラス群。
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <SimpleGtk>

#include "./url_transfer.hpp"

namespace tracker {

// --- クラスの前方宣言 ---

class ToolTrackerClient;
class RecorderClient;

// 固定長メッセージを生成するヘルパー関数
template<std::size_t length>
constexpr std::array<char, length> make_fixed_message(std::string_view sv) {
	std::array<char, length> arr{}; // ゼロ初期化
	// コピーする文字数 = svの長さ と バッファサイズ-1 (ヌル終端用) の小さい方
	size_t len_to_copy = std::min(sv.length(), length - 1);

	// 文字列をコピー
	for (size_t i = 0; i < len_to_copy; i++) {
		arr[i] = sv[i];
	}

	// 残りをスペースでパディング (ヌル終端の1つ手前まで)
	for (size_t i = len_to_copy; i < length - 1; i++) {
		arr[i] = ' ';
	}
	// arr[length - 1] はゼロ初期化されているので、ヌル終端は保証される

	return arr;
}

// --- 共通定数 ---
//inline constexpr int COMMON_TIMEOUT_SECONDS = 5;
inline constexpr int COMMON_TIMEOUT_SECONDS = 1;	// TODO: テスト用

// --- インサート個数取得クライアント 定数 ---
//inline constexpr const char*	COUNTER_HOST			= "10.19.133.3";
inline constexpr const char*	COUNTER_HOST			= "localhost";	// dummy
inline constexpr int			COUNTER_PORT			= 12345;
inline constexpr int			COUNTER_TIMEOUT_SECONDS	= COMMON_TIMEOUT_SECONDS;

inline std::unique_ptr<tracker::ToolTrackerClient> g_tool_tracker;	// 工具追跡クライアントのグローバルインスタンス
inline std::unique_ptr<tracker::RecorderClient> g_recorder_client;	// 録画クライアントのグローバルインスタンス

/**
 * @class ToolTrackerClient
 * @brief 工具追跡システムのイベント通知サーバーとのTCP通信を管理するクラス。
 */
class ToolTrackerClient {
public:
	// --- 定数 ---
	//static constexpr const char*	HOST			= "10.19.133.122";	// D201
	//static constexpr const char*	HOST			= "10.19.106.160";	// C103
	static constexpr const char*	HOST			= "localhost";	// dummy
	static constexpr int			PORT			= 5000;
	static constexpr size_t			MSG_LENGTH		= 24;	// メッセージ長
	static constexpr std::uint32_t	TIMEOUT_SECONDS	= COMMON_TIMEOUT_SECONDS;

	// --- プロトコルメッセージ定数 (パディングなし) ---
	// 送信メッセージ
	static constexpr std::string_view	MSG_CONNECT					= "Connect";					// 接続要求
	static constexpr std::string_view	MSG_DISCONNECT				= "Disconnect";					// 切断要求
	static constexpr std::string_view	MSG_QR_SCAN					= "QR_scan";					// QRコードスキャン通知
	// 受信メッセージ
	static constexpr std::string_view	MSG_CONNECT_ACK				= "Connect_ACK";				// 接続完了応答
	static constexpr std::string_view	MSG_DISCONNECT_ACK			= "Disconnect_ACK";				// 切断完了応答
	static constexpr std::string_view	MSG_SCAN_S					= "Scan_S";						// スキャン成功通知
	static constexpr std::string_view	MSG_SCAN_F					= "Scan_F";						// スキャン失敗通知
	static constexpr std::string_view	MSG_SCANNED_IN_CONTAINER	= "Scanned_in_container";		// スキャン済工具がコンテナ内に入った通知
	static constexpr std::string_view	MSG_SCANNED_OUT_CONTAINER	= "Scanned_out_container";		// スキャン済工具がコンテナ外に出た通知
	static constexpr std::string_view	MSG_OTHER_IN_CONTAINER		= "Other_in_container";			// その他工具がコンテナ内に入った通知
	static constexpr std::string_view	MSG_OTHER_OUT_CONTAINER		= "Other_out_container";		// その他工具がコンテナ外に出た通知
	static constexpr std::string_view	MSG_UNSCANNED_IN_CONTAINER	= "Unscanned_in_container";		// 未スキャン工具がコンテナ内に入った通知
	static constexpr std::string_view	MSG_UNSCANNED_OUT_CONTAINER	= "Unscanned_out_container";	// 未スキャン工具がコンテナ外に出た通知
	static constexpr std::string_view	MSG_UNSCANNED_OUT_FRAME		= "Unscanned_out_frame";		// 未スキャン工具がフレーム外に出た通知

	// --- プロトコルメッセージ定数 (パディングあり) ---
	// 送信メッセージ
	static constexpr auto	PADDED_MSG_CONNECT					= make_fixed_message<MSG_LENGTH>(MSG_CONNECT);					// 接続要求
	static constexpr auto	PADDED_MSG_DISCONNECT				= make_fixed_message<MSG_LENGTH>(MSG_DISCONNECT);				// 切断要求
	static constexpr auto	PADDED_MSG_QR_SCAN					= make_fixed_message<MSG_LENGTH>(MSG_QR_SCAN);					// QRコードスキャン通知
	// 受信メッセージ
	static constexpr auto	PADDED_MSG_CONNECT_ACK				= make_fixed_message<MSG_LENGTH>(MSG_CONNECT_ACK);				// 接続完了応答
	static constexpr auto	PADDED_MSG_DISCONNECT_ACK			= make_fixed_message<MSG_LENGTH>(MSG_DISCONNECT_ACK);			// 切断完了応答
	static constexpr auto	PADDED_MSG_SCAN_S					= make_fixed_message<MSG_LENGTH>(MSG_SCAN_S);					// スキャン成功通知
	static constexpr auto	PADDED_MSG_SCAN_F					= make_fixed_message<MSG_LENGTH>(MSG_SCAN_F);					// スキャン失敗通知
	static constexpr auto	PADDED_MSG_SCANNED_IN_CONTAINER		= make_fixed_message<MSG_LENGTH>(MSG_SCANNED_IN_CONTAINER);		// スキャン済工具がコンテナ内に入った通知
	static constexpr auto	PADDED_MSG_SCANNED_OUT_CONTAINER	= make_fixed_message<MSG_LENGTH>(MSG_SCANNED_OUT_CONTAINER);	// スキャン済工具がコンテナ外に出た通知
	static constexpr auto	PADDED_MSG_OTHER_IN_CONTAINER		= make_fixed_message<MSG_LENGTH>(MSG_OTHER_IN_CONTAINER);		// その他工具がコンテナ内に入った通知
	static constexpr auto	PADDED_MSG_OTHER_OUT_CONTAINER		= make_fixed_message<MSG_LENGTH>(MSG_OTHER_OUT_CONTAINER);		// その他工具がコンテナ外に出た通知
	static constexpr auto	PADDED_MSG_UNSCANNED_IN_CONTAINER	= make_fixed_message<MSG_LENGTH>(MSG_UNSCANNED_IN_CONTAINER);	// 未スキャン工具がコンテナ内に入った通知
	static constexpr auto	PADDED_MSG_UNSCANNED_OUT_CONTAINER	= make_fixed_message<MSG_LENGTH>(MSG_UNSCANNED_OUT_CONTAINER);	// 未スキャン工具がコンテナ外に出た通知
	static constexpr auto	PADDED_MSG_UNSCANNED_OUT_FRAME		= make_fixed_message<MSG_LENGTH>(MSG_UNSCANNED_OUT_FRAME);		// 未スキャン工具がフレーム外に出た通知

	/**
	 * @brief 工具追跡サーバーからのメッセージ種別
	 */
	enum class NotificationType {
		CONNECT_ACK,				// 接続完了
		DISCONNECT_ACK,				// 切断完了
		SCAN_SUCCESS,				// スキャン成功
		SCAN_FAILURE,				// スキャン失敗
		SCANNED_IN_CONTAINER,		// スキャン済工具がコンテナ内に入った
		SCANNED_OUT_CONTAINER,		// スキャン済工具がコンテナ外に出た
		OTHER_IN_CONTAINER,			// その他工具がコンテナ内に入った
		OTHER_OUT_CONTAINER,		// その他工具がコンテナ外に出た
		UNSCANNED_IN_CONTAINER,		// 未スキャン工具がコンテナ内に入った
		UNSCANNED_OUT_CONTAINER,	// 未スキャン工具がコンテナ外に出た
		UNSCANNED_OUT_FRAME,		// 未スキャン工具がフレーム外に出た
		UNKNOWN,					// 不明なメッセージ
		CONNECTION_LOST,			// 接続切れ
		ERROR						// 受信エラー
	};

	/**
	 * @struct Notification
	 * @brief 工具追跡サーバーからの通知内容
	 */
	struct Notification {
		NotificationType type = NotificationType::UNKNOWN;
		std::array<char, MSG_LENGTH> raw_message{};
		std::string trimmed_message{};
		std::string error_details{};
	};

	/**
	 * @brief コンストラクタ。
	 * @param[in] callback イベント受信時に呼び出されるコールバック関数。
	 */
	explicit ToolTrackerClient(std::function<void(const Notification&)> callback);
	~ToolTrackerClient();

	// コピー・ムーブを完全に禁止 (this キャプチャが存在するため)
	ToolTrackerClient(const ToolTrackerClient&) = delete;
	ToolTrackerClient& operator=(const ToolTrackerClient&) = delete;
	ToolTrackerClient(ToolTrackerClient&&) = delete;
	ToolTrackerClient& operator=(ToolTrackerClient&&) = delete;

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
	std::function<void(const tracker::ToolTrackerClient::Notification&)> notification_callback;
};

/**
 * @class RecorderClient
 * @brief 録画サーバーにHTTPリクエストを送信するクラス。
 */
class RecorderClient {
public:
	// --- 定数 ---
	static constexpr const char*	BASE_URL		= "http://10.19.106.160:8080";
	static constexpr const char*	CONTEXT_PATH	= "/helloWeb-1.0"; // Tomcatなどのコンテキストパス
	static constexpr const char*	USERNAME		= "yourUsername";
	static constexpr const char*	PASSWORD		= "yourPassword";
	static constexpr long			TIMEOUT_SECONDS	= COMMON_TIMEOUT_SECONDS;

	/**
	 * @brief コンストラクタ。
	 * @details 定義された定数を使用して初期化します。
	 */
	RecorderClient(); // 引数を削除

	// コピー・ムーブを完全に禁止 (ミューテックスが存在するため)
	RecorderClient(const RecorderClient&) = delete;
	RecorderClient& operator=(const RecorderClient&) = delete;
	RecorderClient(RecorderClient&&) = delete;
	RecorderClient& operator=(RecorderClient&&) = delete;

	/**
	 * @brief 現在時刻に基づいて "recording_YYYYMMDD_HHMMSS.mp4" 形式のファイル名を生成します。
	 * @return 生成されたファイル名文字列。
	 */
	std::string generate_timestamp_filename() const;

	/**
	 * @brief 録画開始コマンドを送信します。
	 * @param[in] filename (オプション・デフォルト "") 録画ファイル名を指定します。
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

	/**
	 * @brief 現在録画中のファイル名を取得します。(スレッドセーフ)
	 * @return 録画中の場合はファイル名、録画中でない場合は std::nullopt。
	 */
	std::optional<std::string> get_current_filename() const;

private:
	std::string base_url;
	std::string context_path;
	std::string username;
	std::string password;

	/// @brief 現在録画中のファイル名を保持 (録画中でなければ nullopt)
	std::optional<std::string> m_current_filename;
	/// @brief m_current_filename へのスレッドセーフなアクセスを保証するミューテックス
	mutable std::mutex m_filename_mutex;
};

/**
 * @brief インサート個数取得サーバーに接続し、個数を取得して切断します。
 * @param[in] host (オプション・デフォルト COUNTER_HOST) サーバーホスト名 (またはIPアドレス)。
 * @param[in] port (オプション・デフォルト COUNTER_PORT) サーバーポート番号。
 * @param[in] timeout_sec (オプション・デフォルト COUNTER_TIMEOUT_SECONDS) 受信タイムアウト(秒)。
 * @return 取得した個数。失敗した場合は std::nullopt。
 */
std::optional<int> get_insert_count(
	const char* host = COUNTER_HOST,
	int port = COUNTER_PORT,
	std::uint32_t timeout_sec = COUNTER_TIMEOUT_SECONDS
);

} // namespace tracker
