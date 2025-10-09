/**
 * @file plc_communicator.cpp
 * @brief plc_communicator.hpp で宣言されたPLC通信機能の実装ファイル。
 */
#include "./includes.hpp" // DEV_PRINTとsg::Socketのためにインクルード
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

// --- 以下は、このファイル内部でのみ使用するヘルパー関数 ---

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
		(std::uint8_t)(on ? 0x10 : 0x00) // データ (ON/OFF)
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
		0x01, 0x00  // 点数 (1点)
	};
}

/**
 * @brief コマンドを送信し、PLCからの正常応答（エラーコード0x0000）を待ちます。
 * @param cmd 送信するコマンドのバイト列。
 * @return 応答が正常終了した場合はtrue、エラーが発生した場合はfalse。
 */
static bool send_and_receive_ack(const std::vector<std::uint8_t>& cmd) {
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
 * @brief PLCリスナースレッドからGUIスレッドに通知を送るためのヘルパー関数。
 * @param command 完了したコマンド内容。
 */
void gui_notifier(PlcCommand command) {
	if (gui::mainWindow) {
		gui::mainWindow->on_plc_notified(command);
	}

}

// --- ここからヘッダで宣言された関数の実装 ---

bool retrieve_container(std::uint16_t container_number) {
	DEV_PRINT("PLC: コンテナ({})の搬出要求を開始します。\n", container_number);
	if (!send_and_receive_ack(make_write_M_command(container_number, true))) {
		DEV_PRINT(stderr, "  エラー: M{}のONに失敗しました。\n", container_number);
		return false;
	}
	DEV_PRINT("  - M{} ON 成功\n", container_number);

	if (!send_and_receive_ack(make_write_M_command(M_DEVICE_RETRIEVE_START, true))) {
		DEV_PRINT(stderr, "  エラー: M{} (搬出開始トリガ)のONに失敗しました。\n", M_DEVICE_RETRIEVE_START);
		return false;
	}
	DEV_PRINT("  - M{} ON 成功\n", M_DEVICE_RETRIEVE_START);
	return true;
}

bool store_container() {
	DEV_PRINT("PLC: コンテナの搬入要求を開始します。\n");
	if (!send_and_receive_ack(make_write_M_command(M_DEVICE_STORE_START, true))) {
		DEV_PRINT(stderr, "  エラー: M{} (搬入開始トリガ)のONに失敗しました。\n", M_DEVICE_STORE_START);
		return false;
	}
	DEV_PRINT("  - M{} ON 成功\n", M_DEVICE_STORE_START);
	return true;
}

bool waiting_for_completion(std::uint16_t address) {
	DEV_PRINT("PLC: M{} (完了信号) の監視を開始します...\n", address);
	while (true) {
		bool on = false;
		if (!read_M_status(address, on)) {
			return false;
		}
		if (on) {
			DEV_PRINT("  - M{} ON を検出しました。\n", address);
			break;
		}
		// マクロで定義された時間だけ待機
		std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_INTERVAL_MS));
	}
	return true;
}

void run_plc_listener() {
	DEV_PRINT("PLCリスナースレッドを開始します。\n");
	while (true) {
		try {
			PlcCommand command = g_plc_command_queue.pop();
			command.result = false;

			/*	TODO: 実際に動かすときはコメントアウトを解除
			switch (command.type) {
			case PlcCommandType::RETRIEVE_CONTAINER:
				command.result = plc::retrieve_container(command.data);
				break;
			case PlcCommandType::STORE_CONTAINER:
				command.result = plc::store_container();
				break;
			case PlcCommandType::WAIT_FOR_RETRIEVE_COMPLETION:
				command.result = plc::waiting_for_completion(M_DEVICE_RETRIEVE_COMPLETED);
				break;
			case PlcCommandType::WAIT_FOR_STORE_COMPLETION:
				command.result = plc::waiting_for_completion(M_DEVICE_STORE_COMPLETED);
				break;
			}
			/*/
			std::this_thread::sleep_for(std::chrono::seconds(3));
			command.result = true;	// 常に成功
			//*/

			if (command.result) {
				// 実行したい処理をラムダ式としてヒープに確保
				// ラムダのポインタを渡して、GUIスレッドでの実行を依頼
				g_idle_add(gui::MainWindow::dispatch_to_gui_thread, new std::function<void()>([=]() { gui_notifier(command); }));
			} else { /* TODO: エラー処理 */ }
		} catch (const std::exception& e) {
			DEV_PRINT(stderr, "PLCリスナースレッドで例外が発生: {}\n", e.what());
		}
	}
}

} // namespace plc