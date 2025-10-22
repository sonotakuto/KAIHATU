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

// --- 以下は、このファイル内部でのみ使用するヘルパー関数 ---

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
 * @brief PLCにセッションの開始を通知します。
 * @details セッション開始を意味するM82をONにします。
 * @return 処理が成功した場合はtrue、失敗した場合はfalse。
 */
static bool start_session() {
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

/**
 * @brief PLCにセッションの終了を通知します。
 * @details セッション終了を意味するM83をONにします。
 * @return 処理が成功した場合はtrue、失敗した場合はfalse。
 */
static bool end_session() {
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

/**
 * @brief PLCにコンテナの搬出を要求します。
 * @details 指定されたコンテナ番号に対応するMデバイスと、搬出開始を指示するM91をONにします。
 * @param container_number 搬出するコンテナ番号。
 * @return 処理が成功した場合はtrue、失敗した場合はfalse。
 */
static bool retrieve_container(std::uint16_t container_number) {
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

/**
 * @brief PLCにコンテナの搬入を要求します。
 * @details 搬入開始を指示するM92をONにします。
 * @return 処理が成功した場合はtrue、失敗した場合はfalse。
 */
static bool store_container() {
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
 * @brief PLCリスナースレッドのメインループ。
 * @details この関数は無限ループで動作し、g_command_queueから指令を受け取って処理します。
 * GUIスレッドとは別スレッドで実行されることを想定しています。
 */
static void run_status_monitor() {
	DEV_PRINT("PLCリスナースレッドを開始します。\n");

	// 監視リストがアクティブかどうかをチェックするラムダ式
	auto is_any_watch_active = []() {
		for (const auto& [name, item] : g_watchlist) {
			if (item.is_active) return true;
		}
		return false;
	};

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
			std::optional<Command> command_opt;
			if (!is_any_watch_active()) {
				command_opt = g_command_queue.pop();	// 監視対象がないので、指令が来るまでブロックして待機 (CPUリソースを解放)
				timer.reset();							// 待機から復帰したので、タイマーの基準時刻を「今」にリセットする
			} else {
				// 監視対象があるので、ポーリングの合間に指令をチェック (ブロックしない)
				command_opt = g_command_queue.try_pop();
			}

			if (command_opt.has_value()) {
				Command command = command_opt.value();
				bool result = false;

				switch (command.type) {
				case CommandType::SET_WATCH_ITEM_ACTIVE: {
					auto cmd_data = std::any_cast<SetWatchActiveCmd>(command.data);
					if (g_watchlist.count(cmd_data.watch_item_name)) {
						auto& item = g_watchlist.at(cmd_data.watch_item_name);
						item.is_active = cmd_data.is_active;
						if (item.is_active) {
							item.is_first = true;
						}
						DEV_PRINT("PLC監視項目 '{}' を {} に設定しました。\n", cmd_data.watch_item_name, ((cmd_data.is_active) ? ("有効") : ("無効")));
					}
					break;
				}
				case CommandType::START_SESSION:
					send_and_receive_ack(make_write_M_command(M_DEVICE_SESSION_START, true));
					break;
				case CommandType::END_SESSION:
					send_and_receive_ack(make_write_M_command(M_DEVICE_SESSION_END, true));
					break;
				case CommandType::RETRIEVE_CONTAINER:
					result = retrieve_container(std::any_cast<uint16_t>(command.data));
					notify_gui_thread({((result) ? (NotificationType::TASK_SUCCESS) : (NotificationType::TASK_FAILURE)), command});
					break;
				case CommandType::STORE_CONTAINER:
					result = store_container();
					notify_gui_thread({((result) ? (NotificationType::TASK_SUCCESS) : (NotificationType::TASK_FAILURE)), command});
					break;
				}
			}

			// --- 2. 監視処理 (g_watchlist 内のアクティブな項目全ての値を順番にPLCに問い合わせる) ---
			for (auto& [name, item] : g_watchlist) {
				if (!item.is_active) continue;	// 無効な項目はスキップ

				bool current_state = false;
				if (read_M_status(item.address, current_state)) {
					// is_firstフラグが立っている場合の初回チェックと、状態変化（エッジ）の検出
					if (item.is_first || current_state != item.prev_status) {
						if (current_state == true && item.notification_rise.has_value()) {
							DEV_PRINT("PLC監視 '{}': 立ち上がり(ON)を検出。\n", name);
							notify_gui_thread(item.notification_rise.value());
							if (item.is_one_shot) item.is_active = false;
						} else if (current_state == false && item.notification_fall.has_value()) {
							DEV_PRINT("PLC監視 '{}': 立ち下がり(OFF)を検出。\n", name);
							notify_gui_thread(item.notification_fall.value());
							if (item.is_one_shot) item.is_active = false;
						}
						// 状態を更新
						item.is_first = false;
						item.prev_status = current_state;
					}
				} else {
					socket.close();	// 読み出し失敗＝切断とみなす
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
	// 監視リストの項目を定義
	g_watchlist.insert(std::make_pair(WatchItemName::WAIT_RETRIEVE, WatchItem{
		.address = M_DEVICE_RETRIEVE_COMPLETED,
		.notification_rise = Notification{NotificationType::TASK_SUCCESS, CommandType::RETRIEVE_CONTAINER},
		.is_one_shot = true
		// 他のメンバはデフォルト初期化
	}));
	g_watchlist.insert(std::make_pair(WatchItemName::WAIT_STORE, WatchItem{
		.address = M_DEVICE_STORE_COMPLETED,
		.notification_rise = Notification{NotificationType::TASK_SUCCESS, CommandType::STORE_CONTAINER},
		.is_one_shot = true
	}));
	g_watchlist.insert(std::make_pair(WatchItemName::MONITOR_MAINTENANCE, WatchItem{
		.address = M_DEVICE_RETRIEVE_COMPLETED,
		.notification_fall = Notification{NotificationType::MAINTENANCE_MODE_OFF},
		.notification_rise = Notification{NotificationType::MAINTENANCE_MODE_ON}
	}));
	// ... 他の監視項目の初期化 ...

	// 単一の統合スレッドを起動
	std::thread(run_plc_listener).detach();
}

} // namespace plc