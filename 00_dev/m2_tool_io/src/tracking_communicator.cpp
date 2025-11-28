/**
 * @file tracking_communicator.cpp
 * @brief 工具追跡システムおよび録画サーバーと通信するためのクライアントクラス群の実装。
 */
#include "./includes.hpp"
#include "./url_transfer.hpp"
#include "./tracking_communicator.hpp"

namespace tracker {

// =================================
// ToolTrackerClient クラス実装
// =================================

ToolTrackerClient::ToolTrackerClient(std::function<void(const Notification&)> callback)
	: keep_listening(false), notification_callback(callback) {}

ToolTrackerClient::~ToolTrackerClient() {
	disconnect();
}

bool ToolTrackerClient::connect() {
	if (socket.is_connected()) {
		return true;
	}

	try {
		if (!socket.connect(TRACKER_IP, TRACKER_PORT)) {
			DEV_PRINT(stderr, "[ToolTrackerClient] Connection Failed to {}:{}\n", TRACKER_IP, TRACKER_PORT);
			return false;
		}

		if (socket.send(TRACKER_PADDED_MSG_CONNECT.data(), TRACKER_PADDED_MSG_CONNECT.size()) < 0) {
			DEV_PRINT(stderr, "[ToolTrackerClient] Error sending 'Connect' message.\n");
			socket.close();
			return false;
		}

		socket.set_timeout(5000);
		std::array<char, TRACKER_MSG_LENGTH> buffer{};
		ssize_t bytes_read = socket.receive(buffer.data(), buffer.size());
		socket.set_timeout(0); // ブロッキングモードに戻す
		std::string raw_message(buffer.data(), bytes_read);

		if (bytes_read == TRACKER_MSG_LENGTH && sg::rtrim(raw_message) == TRACKER_MSG_CONNECT_ACK) {
			keep_listening = true;
			listener_thread = std::thread(&ToolTrackerClient::listener_thread_func, this);
			return true;
		} else {
			DEV_PRINT(stderr, "[ToolTrackerClient] Error: Failed to get ACK. Received: '{}'\n",
					((bytes_read > 0) ? (raw_message) : ("(No data or error)")));
			socket.close();
			return false;
		}
	} catch (const sg::SocketTimeoutException& e) {
		DEV_PRINT(stderr, "[ToolTrackerClient] Timeout waiting for ACK: {}\n", e.what());
		socket.close();
		return false;
	} catch (const std::exception& e) {
		DEV_PRINT(stderr, "[ToolTrackerClient] Connection error: {}\n", e.what());
		socket.close();
		return false;
	}
}

void ToolTrackerClient::disconnect() {
	if (!keep_listening && !socket.is_connected()) {
		return;
	}
	keep_listening = false;
	if (socket.is_connected()) {
		try {
			socket.send(TRACKER_PADDED_MSG_DISCONNECT.data(), TRACKER_PADDED_MSG_DISCONNECT.size());
			if (socket.send(TRACKER_PADDED_MSG_DISCONNECT.data(), TRACKER_PADDED_MSG_DISCONNECT.size()) >= 0) {
				socket.set_timeout(5000);
				std::array<char, TRACKER_MSG_LENGTH> buffer{};
				ssize_t bytes_read = socket.receive(buffer.data(), buffer.size());
				socket.set_timeout(0); // ブロッキングモードに戻す
				std::string raw_message(buffer.data(), bytes_read);

				if (bytes_read == TRACKER_MSG_LENGTH && sg::rtrim(raw_message) != TRACKER_MSG_DISCONNECT_ACK) {
					DEV_PRINT(stderr, "[ToolTrackerClient] Error: Failed to get ACK. Received: '{}'\n",
							((bytes_read > 0) ? (raw_message) : ("(No data or error)")));
				}
			} else {
				DEV_PRINT(stderr, "[ToolTrackerClient] Error sending 'Disconnect' message.\n");
			}
		} catch (...) {}
		socket.close();
	}
	if (listener_thread.joinable()) {
		listener_thread.join();
	}
}

bool ToolTrackerClient::is_connected() {
	return keep_listening && socket.is_connected();
}

bool ToolTrackerClient::notify_qr_scan() {
	if (!is_connected()) {
		DEV_PRINT(stderr, "[ToolTrackerClient] Error: Not connected to send QR scan notification.\n");
		// notification_callback({NotificationType::ERROR, {}, "Not connected"}); // 必要ならコールバックで通知
		return false;
	}
	try {
		if (socket.send(TRACKER_PADDED_MSG_QR_SCAN.data(), TRACKER_PADDED_MSG_QR_SCAN.size()) < 0) {
			DEV_PRINT(stderr, "[ToolTrackerClient] Error sending QR scan notification (send returned < 0).\n");
			disconnect();
			notification_callback({NotificationType::ERROR, {}, "Send failed"});
			return false;
		}
		return true;
	} catch (const std::exception& e) {
		DEV_PRINT(stderr, "[ToolTrackerClient] Error sending QR scan notification: {}\n", e.what());
		disconnect();
		notification_callback({NotificationType::ERROR, {}, std::string("Send error: ") + e.what()});
		return false;
	}
}

void ToolTrackerClient::listener_thread_func() {
	std::array<char, TRACKER_MSG_LENGTH> buffer{};

	while (keep_listening) {
		try {
			// 受信のためにバッファをクリア
			buffer.fill('\0');
			ssize_t bytes_read = socket.receive(buffer.data(), buffer.size());
			std::string raw_message(buffer.data(), bytes_read);
			DEV_PRINT(stderr, "[ToolTrackerClient] Received {} bytes\n", bytes_read);
			DEV_PRINT(stderr, "[ToolTrackerClient] Received data: '{}'\n", raw_message);

			if (bytes_read == TRACKER_MSG_LENGTH) {
				Notification notification;
				std::string trimmed_message = sg::rtrim(raw_message);

				// 固定長メッセージの比較を行い、通知タイプを判別、設定
				if		(trimmed_message == TRACKER_MSG_SCAN_S					) { notification.type = NotificationType::SCAN_SUCCESS;				}
				else if	(trimmed_message == TRACKER_MSG_SCAN_F					) { notification.type = NotificationType::SCAN_FAILURE;				}
				else if	(trimmed_message == TRACKER_MSG_SCANNED_IN_CONTAINER	) { notification.type = NotificationType::SCANNED_IN_CONTAINER;		}
				else if	(trimmed_message == TRACKER_MSG_SCANNED_OUT_CONTAINER	) { notification.type = NotificationType::SCANNED_OUT_CONTAINER;	}
				else if	(trimmed_message == TRACKER_MSG_OTHER_IN_CONTAINER		) { notification.type = NotificationType::OTHER_IN_CONTAINER;		}
				else if	(trimmed_message == TRACKER_MSG_OTHER_OUT_CONTAINER		) { notification.type = NotificationType::OTHER_OUT_CONTAINER;		}
				else if	(trimmed_message == TRACKER_MSG_UNSCANNED_IN_CONTAINER	) { notification.type = NotificationType::UNSCANNED_IN_CONTAINER;	}
				else if	(trimmed_message == TRACKER_MSG_UNSCANNED_OUT_CONTAINER	) { notification.type = NotificationType::UNSCANNED_OUT_CONTAINER;	}
				else if	(trimmed_message == TRACKER_MSG_UNSCANNED_OUT_FRAME		) { notification.type = NotificationType::UNSCANNED_OUT_FRAME;		}
				else { notification.type = NotificationType::UNKNOWN; }

				notification.raw_message = std::move(buffer);	// 固定長配列をムーブ
				notification.trimmed_message = std::move(trimmed_message);	// トリムしたメッセージをムーブ
				notification_callback(notification);

			} else if (bytes_read <= 0) {
				// receive が 0 (正常切断) または負数 (エラー) を返した場合
				if (keep_listening) {	// disconnect()以外での切断
					notification_callback({NotificationType::CONNECTION_LOST, {}, (bytes_read == 0 ? "Connection closed by peer (EOF)" : "Receive error")});
				}
				break;

			} else {
				Notification notification;
				notification.type = NotificationType::ERROR;
				notification.error_details = fmt::format("Received unexpected data length: {}", bytes_read);
				notification_callback(notification);
				// break; // エラーなら終了する場合
			}

		} catch (const sg::SocketTimeoutException& e) {
			// タイムアウト例外は無視
		} catch (const std::exception& e) {
			if (keep_listening) {
				notification_callback({NotificationType::ERROR, {}, std::string("Listener error: ") + e.what()});
				DEV_PRINT(stderr, "[ToolTrackerClient] Listener error: {}\n", e.what());
			}
			break;
		}
	} // while
	keep_listening = false;
}

// =================================
// RecorderClient クラス実装
// =================================

// コンストラクタ: 定数を使ってメンバ変数を初期化
RecorderClient::RecorderClient() :
	base_url(RECORDER_BASE_URL),
	context_path(RECORDER_CONTEXT_PATH),
	username(RECORDER_USERNAME),
	password(RECORDER_PASSWORD),
	m_current_filename(std::nullopt)
{}

std::string RecorderClient::generate_timestamp_filename() const {
	return fmt::format("recording_{:%Y%m%d_%H%M%S}.mp4", sg::now());
}

std::optional<std::string> RecorderClient::start_recording(std::string filename /* = "" */) {
	// ファイル名が指定されなかった場合は生成
	if (filename.empty()) {
		filename = generate_timestamp_filename();
	}
	std::string post_data = "filename=" + filename;

	try {
		url_transfer::CurlRequest request;
		std::string url = base_url + context_path + "/record/start";

		request.set_url(url);
		request.set_post_data(post_data);
		request.set_basic_auth(username, password);
		request.set_timeout(5L);
		request.perform();

		long http_code = request.get_response_code();

		// 成功した場合 (HTTP 2xx) はファイル名を返す
		if (http_code >= 200 && http_code < 300) {
			// 成功時にファイル名を保存して返す
			{
				std::lock_guard<std::mutex> lock(m_filename_mutex);
				m_current_filename = filename;
			}
			return filename;
		} else {
			// 失敗した場合はエラーログを出力して nullopt を返す
			DEV_PRINT(stderr, "[RecorderClient] Start recording failed with HTTP status code: {}\nResponse: {}\n", http_code, request.get_response_body());
			return std::nullopt; // ★★★ 失敗時は nullopt ★★★
		}

	} catch (const std::exception& e) {
		DEV_PRINT(stderr, "[RecorderClient] Error starting recording: {}\n", e.what());
		return std::nullopt; // ★★★ 例外発生時も nullopt ★★★
	}
}

bool RecorderClient::stop_recording() {
	bool success = false;
	try {
		url_transfer::CurlRequest request;
		std::string url = base_url + context_path + "/record/stop";

		request.set_url(url);
		request.set_post_data("");
		request.set_basic_auth(username, password);
		request.set_timeout(5L);
		request.perform();

		long http_code = request.get_response_code();
		success = (http_code >= 200 && http_code < 300);

	} catch (const std::exception& e) {
		DEV_PRINT(stderr, "[RecorderClient] Error stopping recording: {}\n", e.what());
		success = false;
	}

	// 成功時にファイル名をクリア
	if (success) {
		std::lock_guard<std::mutex> lock(m_filename_mutex);
		m_current_filename = std::nullopt;
	}
	return success;
}

std::optional<std::string> RecorderClient::get_current_filename() const {
	std::lock_guard<std::mutex> lock(m_filename_mutex);
	return m_current_filename; // 現在のファイル名 (または nullopt) を返す
}

// =================================
// インサート個数取得クライアント 実装
// =================================

std::optional<int> get_insert_count(const char* ip, int port, int timeout_ms) {
	sg::Socket local_socket; // ローカル変数としてソケットを作成 (RAII)
	std::optional<int> result = std::nullopt;
	DEV_PRINT("インサート個数取得サーバーと通信します\n");

	try {
		// 1. 接続
		if (!local_socket.connect(ip, port)) {
			DEV_PRINT(stderr, "[InsertCounter] Connection Failed to {}:{}\n", ip, port);
			return std::nullopt; // RAIIによりソケットは自動で閉じられる
		}

		// 2. 受信 (タイムアウト付き)
		local_socket.set_timeout(timeout_ms);
		std::string data_str = local_socket.receive(1024); // 最大1024バイト受信

		if (!data_str.empty()) {
			// 3. 文字列を整数に変換
			try {
				result = std::stoi(data_str);
			} catch (const std::invalid_argument& e) {
				DEV_PRINT(stderr, "[InsertCounter] Error: Received data is not a valid integer: '{}'\n", data_str);
			} catch (const std::out_of_range& e) {
				DEV_PRINT(stderr, "[InsertCounter] Error: Received integer is out of range: '{}'\n", data_str);
			}
		} else {
			DEV_PRINT(stderr, "[InsertCounter] Received empty data or connection closed.\n");
		}

	} catch (const sg::SocketTimeoutException& e) {
		DEV_PRINT(stderr, "[InsertCounter] Timeout receiving data: {}\n", e.what());
	} catch (const std::exception& e) {
		DEV_PRINT(stderr, "[InsertCounter] Error during communication: {}\n", e.what());
	}

	// 4. 切断 (sg::Socket のデストラクタが自動で close() を呼ぶので明示的な処理は不要)
	// local_socket がスコープを抜けるときに自動で close される

	return result;
}

} // namespace tracker
