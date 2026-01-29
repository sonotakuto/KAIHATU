/**
 * @file tracking_communicator.cpp
 * @brief 工具追跡システムおよび録画サーバーと通信するためのクライアントクラス群の実装。
 */
#include "utils/string_utils.hpp"
#include "includes.hpp"
#include "tracking_communicator.hpp"

namespace tracker {

// =================================
// ToolTrackerClient クラス実装
// =================================

// --- 公開メンバ関数 ---

ToolTrackerClient::ToolTrackerClient(std::function<void(const Notification&)> callback) :
	keep_listening(false),
	notification_callback(callback)
{}

ToolTrackerClient::~ToolTrackerClient() {
	stop();
}

bool ToolTrackerClient::start() {
	if (keep_listening) {
		return true;
	}

	stop();

	try {
		std::lock_guard<std::mutex> lock(socket_mutex);
		if (connect()) {
			keep_listening = true;
			listener_thread = std::thread(&ToolTrackerClient::listener_thread_func, this);
			return true;
		} else {
			return false;
		}
	} catch (const std::exception& e) {
		DEV_PRINT(stderr, "[ToolTrackerClient] Start error: {}\n", e.what());
		stop();
		return false;
	}
}

void ToolTrackerClient::stop() {
	keep_listening = false;

	{
		std::lock_guard<std::mutex> lock(socket_mutex);
		disconnect();
	}

	if (listener_thread.joinable()) {
		listener_thread.join();
	}
}


bool ToolTrackerClient::notify_qr_scan() {
	bool result{false};
	{
		std::lock_guard<std::mutex> lock(socket_mutex);
		if (!is_connected_internal()) {
			DEV_PRINT(stderr, "[ToolTrackerClient] Error: Not connected to send QR scan notification.\n");
			notification_callback(Notification{.type = NotificationType::ERROR, .error_details = "Not connected"});
			result = false;
		}
		try {
			if (socket->send(PADDED_MSG_QR_SCAN.data(), PADDED_MSG_QR_SCAN.size()) >= 0) {
				result = true;
			} else {
				DEV_PRINT(stderr, "[ToolTrackerClient] Error sending QR scan notification (send returned < 0).\n");
				notification_callback(Notification{.type = NotificationType::ERROR, .error_details = "Send failed"});
				result = false;
			}
		} catch (const std::exception& e) {
			DEV_PRINT(stderr, "[ToolTrackerClient] Error sending QR scan notification: {}\n", e.what());
			notification_callback(Notification{.type = NotificationType::ERROR, .error_details = fmt::format("Send error: {}", e.what())});
			result = false;
		}
	}

	if (!result) {
		stop();
	}

	return result;
}

// --- 非公開メンバ関数 ---

bool ToolTrackerClient::connect() {
	if (is_connected_internal()) {
		return true;
	}

	disconnect();

	try {
		socket = std::make_unique<sg::Socket>();

		if (!socket->connect(HOST, PORT)) {
			DEV_PRINT(stderr, "[ToolTrackerClient] Connection Failed to {}:{}\n", HOST, PORT);
			return false;
		}

		if (socket->send(PADDED_MSG_CONNECT.data(), PADDED_MSG_CONNECT.size()) < 0) {
			DEV_PRINT(stderr, "[ToolTrackerClient] Error sending 'Connect' message.\n");
			socket->close();
			return false;
		}

		socket->set_timeout(TIMEOUT_SECONDS);
		std::array<char, MSG_LENGTH> buffer{};
		ssize_t bytes_read = socket->receive(buffer.data(), buffer.size());
		socket->set_timeout(0);	// ブロッキングモードに戻す
		std::string_view raw_view = utils::make_strn_view(buffer.data(), ((bytes_read > 0) ? (static_cast<size_t>(bytes_read)) : (0)));
		DEV_PRINT(stderr, "[ToolTrackerClient] Received {} bytes\n", bytes_read);
		DEV_PRINT(stderr, "[ToolTrackerClient] Received data: '{}'\n", raw_view);

		std::string_view trimmed_view = utils::rstrip(raw_view);

		if (bytes_read == MSG_LENGTH && trimmed_view == MSG_CONNECT_ACK) {
			notification_callback(Notification{
				.type = NotificationType::CONNECT_ACK,
				.raw_message = buffer,
				.trimmed_message = std::string(trimmed_view)
			});
			return true;
		} else {
			DEV_PRINT(stderr, "[ToolTrackerClient] Error: Failed to get ACK. Received: '{}'\n",
				((bytes_read > 0) ? (raw_view) : ("(No data or error)"))
			);
			socket->close();
			return false;
		}
	} catch (const sg::SocketTimeoutException& e) {
		DEV_PRINT(stderr, "[ToolTrackerClient] Timeout waiting for ACK: {}\n", e.what());
		socket->close();
		return false;
	} catch (const std::exception& e) {
		DEV_PRINT(stderr, "[ToolTrackerClient] Connection error: {}\n", e.what());
		socket->close();
		return false;
	}
}

void ToolTrackerClient::disconnect() {
	if (is_connected_internal()) {
		try {
			if (socket->send(PADDED_MSG_DISCONNECT.data(), PADDED_MSG_DISCONNECT.size()) < 0) {
				DEV_PRINT(stderr, "[ToolTrackerClient] Error sending 'Disconnect' message.\n");
			}
		} catch (...) {}
		socket->close();
	}
}

void ToolTrackerClient::listener_thread_func() {
	std::array<char, MSG_LENGTH> buffer{};

	while (keep_listening) {
		try {
			// 受信のためにバッファをクリア
			buffer.fill('\0');
			ssize_t bytes_read = socket->receive(buffer.data(), buffer.size());
			std::string_view raw_view = utils::make_strn_view(buffer.data(), ((bytes_read > 0) ? (static_cast<size_t>(bytes_read)) : (0)));
			DEV_PRINT(stderr, "[ToolTrackerClient] Received {} bytes\n", bytes_read);
			DEV_PRINT(stderr, "[ToolTrackerClient] Received data: '{}'\n", raw_view);

			if (bytes_read == MSG_LENGTH) {
				Notification notification;
				std::string_view trimmed_view = utils::rstrip(raw_view);

				// 固定長メッセージの比較を行い、通知タイプを判別、設定
				if		(trimmed_view == MSG_CONNECT_ACK				) { notification.type = NotificationType::CONNECT_ACK;				}
				else if	(trimmed_view == MSG_DISCONNECT_ACK				) { notification.type = NotificationType::DISCONNECT_ACK;			}
				else if	(trimmed_view == MSG_SCAN_S						) { notification.type = NotificationType::SCAN_SUCCESS;				}
				else if	(trimmed_view == MSG_SCAN_F						) { notification.type = NotificationType::SCAN_FAILURE;				}
				else if	(trimmed_view == MSG_SCANNED_IN_CONTAINER		) { notification.type = NotificationType::SCANNED_IN_CONTAINER;		}
				else if	(trimmed_view == MSG_SCANNED_OUT_CONTAINER		) { notification.type = NotificationType::SCANNED_OUT_CONTAINER;	}
				else if	(trimmed_view == MSG_OTHER_IN_CONTAINER			) { notification.type = NotificationType::OTHER_IN_CONTAINER;		}
				else if	(trimmed_view == MSG_OTHER_OUT_CONTAINER		) { notification.type = NotificationType::OTHER_OUT_CONTAINER;		}
				else if	(trimmed_view == MSG_UNSCANNED_IN_CONTAINER		) { notification.type = NotificationType::UNSCANNED_IN_CONTAINER;	}
				else if	(trimmed_view == MSG_UNSCANNED_OUT_CONTAINER	) { notification.type = NotificationType::UNSCANNED_OUT_CONTAINER;	}
				else if	(trimmed_view == MSG_UNSCANNED_OUT_FRAME		) { notification.type = NotificationType::UNSCANNED_OUT_FRAME;		}
				else { notification.type = NotificationType::UNKNOWN; }

				notification.raw_message = buffer;	// 固定長配列をコピー
				notification.trimmed_message = std::string(trimmed_view);	// トリムしたメッセージをコピー
				notification_callback(notification);

			} else if (bytes_read <= 0) {
				// receive が 0 (正常切断) または負数 (エラー) を返した場合
				if (keep_listening) {	// disconnect()以外での切断
					notification_callback(Notification{.type = NotificationType::CONNECTION_LOST, .error_details = ((bytes_read == 0) ? ("Connection closed by peer (EOF)") : ("Receive error"))});
				}
				break;

			} else {
				notification_callback(Notification{.type = NotificationType::ERROR, .error_details = fmt::format("Received unexpected data length: {}", bytes_read)});
				// break; // エラーなら終了する場合
			}

		} catch (const sg::SocketTimeoutException& e) {
			// タイムアウト例外は無視
		} catch (const std::exception& e) {
			if (keep_listening) {
				notification_callback(Notification{.type = NotificationType::ERROR, .error_details = fmt::format("Listener error: {}", e.what())});
				DEV_PRINT(stderr, "[ToolTrackerClient] Listener error: {}\n", e.what());
			}
			break;
		}
	} // while
	keep_listening = false;

	std::lock_guard<std::mutex> lock(socket_mutex);
	disconnect();
}

// =================================
// RecorderClient クラス実装
// =================================

// コンストラクタ: 定数を使ってメンバ変数を初期化
RecorderClient::RecorderClient() :
	base_url(BASE_URL),
	context_path(CONTEXT_PATH),
	username(USERNAME),
	password(PASSWORD),
	m_current_filename(std::nullopt)
{}

std::string RecorderClient::generate_timestamp_filename() const {
	return fmt::format("recording_{:%Y%m%d_%H%M%S}.mp4", sg::now());
}

std::optional<std::string> RecorderClient::start_recording(std::string filename) {
	std::lock_guard<std::mutex> lock(m_filename_mutex);

	if (m_current_filename.has_value()) {	// TODO: 条件これで良いか考える
		return std::nullopt;
	}

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
		request.set_timeout(TIMEOUT_SECONDS);
		request.perform();

		long http_code = request.get_response_code();

		// 成功した場合 (HTTP 2xx) はファイル名を返す
		if (http_code >= 200 && http_code < 300) {
			// 成功時にファイル名を保存して返す
			m_current_filename = filename;
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
	std::lock_guard<std::mutex> lock(m_filename_mutex);

	if (!m_current_filename.has_value()) {
		return true;
	}

	bool success = false;
	try {
		url_transfer::CurlRequest request;
		std::string url = base_url + context_path + "/record/stop";

		request.set_url(url);
		request.set_post_data("");
		request.set_basic_auth(username, password);
		request.set_timeout(TIMEOUT_SECONDS);
		request.perform();

		long http_code = request.get_response_code();
		success = (http_code >= 200 && http_code < 300);

	} catch (const std::exception& e) {
		DEV_PRINT(stderr, "[RecorderClient] Error stopping recording: {}\n", e.what());
		success = false;
	}

	// 成功時にファイル名をクリア
	if (success) {
		m_current_filename = std::nullopt;
	}
	return success;
}

std::optional<std::string> RecorderClient::get_current_filename() const {
	std::lock_guard<std::mutex> lock(m_filename_mutex);
	return m_current_filename;	// 現在のファイル名 (または nullopt) を返す
}

bool RecorderClient::is_recording() const {
	std::lock_guard<std::mutex> lock(m_filename_mutex);
	return m_current_filename.has_value();
}

// =================================
// インサート個数取得クライアント 実装
// =================================

std::optional<int> get_insert_count(const char* host, int port, std::uint32_t timeout_sec) {
	sg::Socket local_socket; // ローカル変数としてソケットを作成 (RAII)
	std::optional<int> result = std::nullopt;
	DEV_PRINT("インサート個数取得サーバーと通信します\n");

	try {
		// 1. 接続
		if (!local_socket.connect(host, port)) {
			DEV_PRINT(stderr, "[InsertCounter] Connection Failed to {}:{}\n", host, port);
			return std::nullopt; // RAIIによりソケットは自動で閉じられる
		}

		// 2. 受信 (タイムアウト付き)
		local_socket.set_timeout(timeout_sec);
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
