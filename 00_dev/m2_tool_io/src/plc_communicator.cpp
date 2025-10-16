/**
 * @file plc_communicator.cpp
 * @brief plc_communicator.hpp で宣言されたPLC通信機能の実装ファイル。
 */
#include "./includes.hpp" // DEV_PRINTとsg::Socketのためにインクルード
#include "./periodic_timer.hpp"
#include "./tool_io_gui.hpp"
#include "./plc_communicator.hpp"

namespace plc {

/// @brief PLCからのエラーコードとメッセージを対応付けるマップ
static const std::map<std::uint16_t, std::string> mc_error_map = {
	{0xC061, "コマンド異常"}	,{0xC062, "サブコマンド異常"},
	{0xC040, "アクセス権異常"}	,{0xC050, "監視タイマ異常"},
	{0x5782, "点数異常"}		,{0x0000, "正常終了"}
};

/// @brief PLCとの通信に使用するソケットオブジェクト。
static sg::Socket socket;
/// @brief 複数のスレッドからソケットへ安全にアクセスするためのミューテックス
static std::recursive_mutex socket_mutex;

// --- クラスのメンバ関数 ---

bool WatchFlags::merge(const WatchFlags& other) {
	bool changed = false;
	if (other.enable_fall_notification.has_value() && this->enable_fall_notification != other.enable_fall_notification) {
		this->enable_fall_notification = other.enable_fall_notification;
		changed = true;
	}
	if (other.enable_rise_notification.has_value() && this->enable_rise_notification != other.enable_rise_notification) {
		this->enable_rise_notification = other.enable_rise_notification;
		changed = true;
	}
	if (other.fall_is_one_shot.has_value() && this->fall_is_one_shot != other.fall_is_one_shot) {
		this->fall_is_one_shot = other.fall_is_one_shot;
		changed = true;
	}
	if (other.rise_is_one_shot.has_value() && this->rise_is_one_shot != other.rise_is_one_shot) {
		this->rise_is_one_shot = other.rise_is_one_shot;
		changed = true;
	}
	return changed;
}

// --- このファイル内部でのみ使用するヘルパー関数 ---

/**
 * @brief PLCリスナースレッドからGUIスレッドに通知を送るためのヘルパー関数。
 * @param notification GUIに送る通知内容。
 */
static void notify_gui_thread(const Notification& notification) {
	auto* func = new std::function<void()>([=]() {
		if (gui::mainWindow) {
			gui::mainWindow->on_plc_notified(notification);
		}
	});
	g_idle_add(gui::MainWindow::dispatch_to_gui_thread, func);
}

/**
 * @brief Mデバイスのビット書き込みコマンド（1点）を生成します。
 * @param addr 書き込み対象のMデバイスアドレス。
 * @param on 書き込む値 (true=ON, false=OFF)。
 * @return 書き込みコマンドのバイト列。
 */
static std::vector<std::uint8_t> make_write_M_command(std::uint16_t addr, bool on) {
	return {
		0x02, 0xFF, 0x00, 0x00, // サブヘッダ (応答要求)
		(std::uint8_t)(addr & 0xFF), (std::uint8_t)((addr >> 8) & 0xFF), 0x00, 0x00, // アドレス
		0x20, 0x4D, // デバイスコード (M)
		0x01, 0x00, // 点数 (1点)
		(std::uint8_t)((on) ? (0x10) : (0x00)) // データ (ON/OFF)
	};
}

/**
 * @brief Mデバイスのビット読み出しコマンド（1点）を生成します。
 * @param addr 読み出し対象のMデバイスアドレス。
 * @return 読み出しコマンドのバイト列。
 */
static std::vector<std::uint8_t> make_read_M_command(std::uint16_t addr) {
	return {
		0x00, 0xFF, 0x00, 0x00, // サブヘッダ (読出し要求)
		(std::uint8_t)(addr & 0xFF), (std::uint8_t)((addr >> 8) & 0xFF), 0x00, 0x00, // アドレス
		0x20, 0x4D, // デバイスコード (M)
		0x01, 0x00	// 点数 (1点)
	};
}

/**
 * @brief コマンドを送信し、PLCからの正常応答（エラーコード0x0000）を待ちます。
 * @param cmd 送信するコマンドのバイト列。
 * @return 応答が正常終了した場合はtrue、エラーが発生した場合はfalse。
 */
static bool send_and_receive_ack(const std::vector<std::uint8_t>& cmd) {
	std::lock_guard<std::recursive_mutex> lock(socket_mutex);
	if (socket.send(cmd.data(), cmd.size()) < 0) {
		DEV_PRINT(stderr, "PLCへのコマンド送信エラー\n");
		return false;
	}
	std::uint8_t resp[1024];
	int n = socket.receive(resp, sizeof(resp));
	if (n <= 0) {
		DEV_PRINT(stderr, "PLCからの応答受信エラー\n");
		return false;
	}

	if ((resp[0] == 0x82 || resp[0] == 0x83 || resp[0] == 0x80 || resp[0] == 0x81) && resp[1] == 0x00) {
		return true;
	}

	std::uint16_t error_code = resp[0] | (resp[1] << 8);
	auto it = mc_error_map.find(error_code);
	if (it != mc_error_map.end()) {
		DEV_PRINT(stderr, "PLCエラー 0x{:04X}: {}\n", error_code, it->second);
	} else {
		DEV_PRINT(stderr, "PLCから未知のエラーコードを受信: 0x{:04X}\n", error_code);
	}
	return false;
}

/**
 * @brief Mデバイスの状態を読み出し、その結果を返します。
 * @param addr 読み出し対象のMデバイスアドレス。
 * @param is_on 読み出されたMデバイスの状態を格納する参照。
 * @return 読み出しが成功した場合はtrue、失敗した場合はfalse。
 */
static bool read_M_status(std::uint16_t addr, bool& is_on) {
	std::lock_guard<std::recursive_mutex> lock(socket_mutex);
	std::vector<std::uint8_t> cmd = make_read_M_command(addr);
	if (socket.send(cmd.data(), cmd.size()) < 0) {
		DEV_PRINT(stderr, "PLCへの読み出しコマンド送信エラー\n");
		return false;
	}
	std::uint8_t resp[1024];
	int n = socket.receive(resp, sizeof(resp));
	if (n <= 0) {
		DEV_PRINT(stderr, "PLCからの読み出し応答受信エラー\n");
		return false;
	}

	if (resp[0] != 0x80 || resp[1] != 0x00 || n < 3) {
		std::uint16_t error_code = resp[0] | (resp[1] << 8);
		auto it = mc_error_map.find(error_code);
		if (it != mc_error_map.end()) {
			DEV_PRINT(stderr, "PLCエラー 0x{:04X}: {}\n", error_code, it->second);
		} else {
			DEV_PRINT(stderr, "PLCから未知のエラーコードを受信: 0x{:04X}\n", error_code);
		}
		return false;
	}
	is_on = (resp[2] == 0x10);
	return true;
}

/**
 * @brief PLCへの接続を試みる内部関数。
 */
static bool connect_to_plc() {
	const char* host = HOST;
	int port = PORT;

	std::lock_guard<std::recursive_mutex> lock(socket_mutex);
	if (socket.is_connected()) return true;

	DEV_PRINT("PLCへの接続を試みています... ({}:{})\n", host, port);
	if (socket.connect(host, port)) {
		DEV_PRINT("PLCへの接続が確立しました。\n");
		notify_gui_thread({.type = NotificationType::CONNECTION_ESTABLISHED});
		return true;
	}
	return false;
}

/**
 * @brief 指定されたMデバイスがONになるのを待機します。
 * @details PLCからの完了信号を待つために使用します。ポーリング間隔は0.1秒です。
 * @param address 監視対象のMデバイスのアドレス。
 * @return 待機が成功（ONを検出）した場合はtrue、通信エラー等で失敗した場合はfalse。
 */
static bool waiting_for_completion(std::uint16_t address) {
	DEV_PRINT("PLC: M{} (完了信号) の監視を開始します...\n", address);
	while (true) {
		bool on = false;
		if (!read_M_status(address, on)) {
			return false;
		}
		if (on) {
			DEV_PRINT("	 - M{} ON を検出しました。\n", address);
			break;
		}
		// マクロで定義された時間だけ待機
		std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_INTERVAL_MS));
	}
	return true;
}

/**
 * @brief PLCモニタースレッドのメインループ。
 * @details この関数は無限ループで動作し、g_watchlistの項目を監視する。
 * 項目の増減要求は g_command_queue で指令を受け取って処理します。
 */
static void run_status_monitor() {
	DEV_PRINT("PLCリスナースレッドを開始します。\n");

	// ポーリングの周期制御用タイマ
	utils::PeriodicTimer timer;

	while (true) {
		try {
			// --- 接続処理 ---
			if (!socket.is_connected()) {
				if (!connect_to_plc()) {
					std::this_thread::sleep_for(std::chrono::seconds(5));
					continue;
				}
				timer.reset();	// 接続が確立したタイミングで、タイマーの基準時刻を「今」にリセットする
			}

			// --- 1. 指令処理 (監視対象の有無でpop/try_popを切り替え) ---
			std::optional<WatchCommand> command_opt;
			command_opt = g_command_queue.try_pop();

			if (command_opt.has_value()) {
				WatchCommand command = command_opt.value();
				bool result = false;

				if (g_watchlist.count(command.addr)) {
					if (command.flags.is_active()) {
						auto& item = g_watchlist.at(command.addr);
						item.flags.merge(command.flags);
						if (item.flags.merge(command.flags)) {
							item.is_first = true;
						}
						DEV_PRINT("PLC監視項目 '{}' を設定しました。\n", command.addr);
					} else {
						g_watchlist.erase(command.addr);
					}
				}
				break;
			}

			// --- 2. 監視処理 (g_watchlist 内のアクティブな項目全ての値を順番にPLCに問い合わせる) ---
			for (auto it = g_watchlist.begin(); it != g_watchlist.end(); /* ここではインクリメントしない */) {
				// auto& [addr, item] の代わりに、it->first と it->second を使う
				auto& addr = it->first;
				auto& item = it->second;

				bool current_state = false;
				if (read_M_status(addr, current_state)) {
					// is_firstフラグが立っている場合の初回チェックと、状態変化（エッジ）の検出
					if (item.is_first || current_state != item.prev_status) {
						if (current_state == true && item.flags.enable_rise_notification.value_or(false)) {
							DEV_PRINT("PLC監視 '{}': 立ち上がり(ON)を検出。\n", addr);
							notify_gui_thread(Notification{ NotificationType::MONITOR, MonitorResult{ addr, current_state } });
							if (item.flags.rise_is_one_shot) item.flags.enable_rise_notification = false;
						} else if (current_state == false && item.flags.enable_fall_notification.value_or(false)) {
							DEV_PRINT("PLC監視 '{}': 立ち下がり(OFF)を検出。\n", addr);
							notify_gui_thread(Notification{ NotificationType::MONITOR, MonitorResult{ addr, current_state } });
							if (item.flags.fall_is_one_shot) item.flags.enable_fall_notification = false;
						}

						// 状態を更新
						if (item.flags.is_active()) {
							item.is_first = false;
							item.prev_status = current_state;
							it++; // 手動で次の要素に進める
						} else {
							it = g_watchlist.erase(it); // eraseが次の有効なイテレータを返すので、それで更新
						}
					} else {
						it++; // 状態変化がなかったので、次の要素へ
					}
				} else {
					socket.close(); // 読み出し失敗＝切断とみなす
					notify_gui_thread({.type = NotificationType::CONNECTION_LOST});
					break;
				}
			}

			// ポーリング間隔で待機 (監視対象がない場合はpopで待機しているので、このsleepは実行されない)
			timer.wait_for_next(POLLING_INTERVAL_MS, true);

		} catch (const std::exception& e) {
			DEV_PRINT(stderr, "PLCリスナースレッドで例外が発生: {}\n", e.what());
		}
	}
	DEV_PRINT("PLC統合リスナースレッドを終了します。\n");
}

// --- 公開関数 ---

void initialize() {
	// 監視リストを初期化
	g_watchlist.insert(std::make_pair(M_DEVICE_MAINTENANCE_MODE, WatchData{
		.flags = WatchFlags{ true, true, false, false},
		.is_first = true
	}));

	// 単一の統合スレッドを起動
	std::thread(run_status_monitor).detach();
}

bool start_session() {
	std::lock_guard<std::recursive_mutex> lock(socket_mutex);
	std::uint16_t device_addr = M_DEVICE_SESSION_START;
	DEV_PRINT("PLC: セッション開始通知を行います。\n");
	if (!send_and_receive_ack(make_write_M_command(device_addr, true))) {
		DEV_PRINT(stderr, "	 エラー: M{} (セッション開始トリガ)のONに失敗しました。\n", device_addr);
		return false;
	}
	DEV_PRINT("	 - M{} ON 成功\n", device_addr);
	return true;
}

bool end_session() {
	std::lock_guard<std::recursive_mutex> lock(socket_mutex);
	std::uint16_t device_addr = M_DEVICE_SESSION_END;
	DEV_PRINT("PLC: セッション終了通知を行います。\n");
	if (!send_and_receive_ack(make_write_M_command(device_addr, true))) {
		DEV_PRINT(stderr, "	 エラー: M{} (セッション終了トリガ)のONに失敗しました。\n", device_addr);
		return false;
	}
	DEV_PRINT("	 - M{} ON 成功\n", device_addr);
	return true;
}

bool retrieve_container(std::uint16_t container_number) {
	std::lock_guard<std::recursive_mutex> lock(socket_mutex);
	std::uint16_t device_addr = container_number;
	DEV_PRINT("PLC: コンテナ({})の搬出要求を開始します。\n", device_addr);
	if (!send_and_receive_ack(make_write_M_command(device_addr, true))) {
		DEV_PRINT(stderr, "	 エラー: M{}のONに失敗しました。\n", device_addr);
		return false;
	}
	DEV_PRINT("	 - M{} ON 成功\n", device_addr);

	device_addr = M_DEVICE_RETRIEVE_START;
	if (!send_and_receive_ack(make_write_M_command(device_addr, true))) {
		DEV_PRINT(stderr, "	 エラー: M{} (搬出開始トリガ)のONに失敗しました。\n", device_addr);
		return false;
	}
	DEV_PRINT("	 - M{} ON 成功\n", device_addr);
	return true;
}

bool store_container() {
	std::lock_guard<std::recursive_mutex> lock(socket_mutex);
	std::uint16_t device_addr = M_DEVICE_STORE_START;
	DEV_PRINT("PLC: コンテナの搬入要求を開始します。\n");
	if (!send_and_receive_ack(make_write_M_command(device_addr, true))) {
		DEV_PRINT(stderr, "	 エラー: M{} (搬入開始トリガ)のONに失敗しました。\n", device_addr);
		return false;
	}
	DEV_PRINT("	 - M{} ON 成功\n", device_addr);
	return true;
}

} // namespace plc