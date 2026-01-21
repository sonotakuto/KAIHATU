/**
 * @file plc_communicator.cpp
 * @brief plc_communicator.hpp で宣言されたPLC通信機能の実装ファイル。
 */
#include "utils/periodic_timer.hpp"
#include "utils/signal.hpp"
#include "includes.hpp" // DEV_PRINTとsg::Socketのためにインクルード
#include "tool_io_gui.hpp"
#include "plc_communicator.hpp"

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
static std::mutex socket_mutex;
/// @brief PLCの状態監視リスト。キーは監視内容のユニークな名前。
static std::map<std::uint16_t, WatchData> g_watchlist;
/// @brief 書き込みタスクを管理するキュー
static utils::SafeQueue<Task> g_task_queue;
/// @brief スレッド統合待機用のシグナル
static utils::Signal g_signal;
/// @brief 現在のセッション世代を管理するアトミック変数
static std::atomic<std::uint32_t> g_session_generation{0};
/// @brief 緊急停止時などに、現在実行中のタスクと待機列をすべて破棄するためのフラグ
static std::atomic<bool> g_cancel_requested{false};

// --- このファイル内部でのみ使用するヘルパー関数 ---

/**
 * @brief PLCモニタースレッドからGUIスレッドに通知を送るためのヘルパー関数。
 * @param[in] notification GUIに送る通知内容。
 */
static void notify_gui_thread(const Notification& notification) {
	auto* func = new std::function<void()>([=]() {
		if (gui::mainWindow) {
			gui::mainWindow->plc_on_notified(notification);
		}
	});
	g_idle_add(gui::MainWindow::dispatch_to_gui_thread, func);
}

/**
 * @brief Mデバイスのビット書き込みコマンド（1点）を生成します。
 * @param[in] addr 書き込み対象のMデバイスアドレス。
 * @param[in] on 書き込む値 (true=ON, false=OFF)。
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
 * @param[in] addr 読み出し対象のMデバイスアドレス。
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
 * @brief Dレジスタ（ワード）書き込みコマンド（1点）を生成します。
 * @param[in] addr 書き込み対象のDレジスタアドレス。
 * @param[in] data 書き込む値 (16bit整数)。
 * @return 書き込みコマンドのバイト列。
 */
static std::vector<std::uint8_t> make_write_D_command(std::uint16_t addr, std::uint16_t data) {
	return {
		0x03, 0xFF, 0x00, 0x00, // サブヘッダ (D書込み要求)
		(std::uint8_t)(addr & 0xFF), (std::uint8_t)((addr >> 8) & 0xFF), 0x00, 0x00, // アドレス
		0x20, 0x44, // デバイスコード (D)
		0x01, 0x00, // 点数 (1点)
		(std::uint8_t)(data & 0xFF), (std::uint8_t)((data >> 8) & 0xFF) // データ (リトルエンディアン)
	};
}

/**
 * @brief Dレジスタ（ワード）読み出しコマンド（1点）を生成します。
 * @param[in] addr 読み出し対象のDレジスタアドレス。
 * @return 読み出しコマンドのバイト列。
 */
static std::vector<std::uint8_t> make_read_D_command(std::uint16_t addr) {
	return {
		0x01, 0xFF, 0x00, 0x00, // サブヘッダ (D読出し要求)
		(std::uint8_t)(addr & 0xFF), (std::uint8_t)((addr >> 8) & 0xFF), 0x00, 0x00, // アドレス
		0x20, 0x44, // デバイスコード (D)
		0x01, 0x00	// 点数 (1点)
	};
}

/**
 * @brief コマンドを送信し、PLCからの正常応答（エラーコード0x0000）を待ちます。
 * @param[in] cmd 送信するコマンドのバイト列。
 * @return 応答が正常終了した場合はtrue、エラーが発生した場合はfalse。
 */
static bool send_and_receive_ack(const std::vector<std::uint8_t>& cmd) {
#ifdef STUB_PLC
	DEV_PRINT("PLCにコマンドを送信します。(STUBモード)\n");
	return true;
#else
	std::lock_guard<std::mutex> lock(socket_mutex);
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
		DEV_PRINT("PLCから正常応答を受信しました。\n");
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
#endif
}

/**
 * @brief Mデバイスに値を書き込みます。
 * @details 内部で `send_and_receive_ack` を使用するため、成功時はエラーコード0x0000を確認済みです。
 * @param[in] addr 書き込み対象のMデバイスアドレス。
 * @param[in] on 書き込む値 (true=ON, false=OFF)。
 * @return 書き込みが成功した場合はtrue、失敗した場合はfalse。
 */
static bool write_M_status(std::uint16_t addr, bool on) {
	// send_and_receive_ack は内部で mutex ロックを行うため、ここでのロックは不要
	DEV_PRINT("PLCにM{}を書き込みます: {}\n", addr, on ? "ON" : "OFF");
	return send_and_receive_ack(make_write_M_command(addr, on));
}

/**
 * @brief Dレジスタ（ワード）に値を書き込みます。
 * @details 内部で `send_and_receive_ack` を使用するため、成功時はエラーコード0x0000を確認済みです。
 * @param[in] addr 書き込み対象のDレジスタアドレス。
 * @param[in] data 書き込む値。
 * @return 書き込みが成功した場合はtrue、失敗した場合はfalse。
 */
static bool write_D_word(std::uint16_t addr, std::uint16_t data) {
	// send_and_receive_ack は内部で mutex ロックを行うため、ここでのロックは不要
	// また、D書き込みの正常応答(0x83 0x00)も既存の send_and_receive_ack で判定可能
	return send_and_receive_ack(make_write_D_command(addr, data));
}

/**
 * @brief PLCへの接続を試みる内部関数。
 */
static bool connect_to_plc() {
	const char* host = HOST;
	int port = PORT;

	std::lock_guard<std::mutex> lock(socket_mutex);
	if (socket.is_connected()) { return true; }

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
 * @param[in] address 監視対象のMデバイスのアドレス。
 * @return 待機が成功（ONを検出）した場合はtrue、通信エラー等で失敗した場合はfalse。
 */
/* --- 使わなくなったのでコメントアウト ---
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
*/

/**
 * @brief PLC通信スレッドのメインループ。
 * @details この関数は無限ループで動作し、PLCとの通信を管理します。
 * 通信要求は、キューから受け取り、PLCの状態監視も行います。
 * STUB_PLCが定義されている場合、PLCに接続せず、ワンショット通知を時間経過でシミュレートします。
 * 定義されていない場合、実際のPLCに接続してポーリングを行います。
 */
static void run_communicator() {
#ifndef STUB_PLC

	// =========================================================================
	// 本来の実装 (PLCとの通信)
	// =========================================================================
	DEV_PRINT("PLCリスナースレッドを開始します。\n");

	// ポーリングの周期制御用タイマ
	utils::PeriodicTimer timer;
	// 現在実行中のタスクを保持する変数
	std::optional<Task> current_task;
	// 保留中のタスクを保持する変数
	std::optional<Task> pending_task;

	notify_gui_thread({.type = NotificationType::CONNECTION_LOST});

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

			// =========================================================
			// 1. 入力処理フェーズ (キューからの取り出し)
			//    待機はせず、あるもの全てを処理する (Non-blocking)
			// =========================================================

			// キャンセルリクエストの処理
			// フラグが立っていたら、実行中・保留中・待機列のすべてを破棄する
			if (g_cancel_requested.exchange(false)) { // 読み取ると同時に false に戻す
				DEV_PRINT("PLC: キャンセル要求を受信しました。全タスクを破棄します。\n");
				current_task.reset();	// 完了待ちの破棄 (これで無限待ちから脱出)
				pending_task.reset();	// 書き込み直前のタスクを破棄
				g_task_queue.clear();	// 待機列をすべて空にする
				notify_gui_thread({.type = NotificationType::TASK_CLEARED});
			}

			// --- A. 新しいタスクの取り出し ---
			// タスク実行中でなければ、新しいタスクをキューから取得
			if (!current_task.has_value() && socket.is_connected()) {
				if (!pending_task.has_value()) {
					pending_task = g_task_queue.try_pop();
				}
				while (pending_task.has_value() && socket.is_connected()) {
					DEV_PRINT("PLC: 新しいタスクを開始します。\n");
					// 書き込みアクションを実行
					if (pending_task->action()) {
						DEV_PRINT("PLC: タスクの書き込み処理が成功しました。\n");
						// 書き込み成功
						if (pending_task->wait_completion_addr.has_value()) {
							// 完了待ちが必要なら current_task に設定してループ継続
							current_task = pending_task;
							DEV_PRINT("PLC: 完了信号 M{} の監視に入ります。\n", pending_task->wait_completion_addr.value());
						} else {
							// 完了待ち不要なら即終了
							DEV_PRINT("PLC: タスク完了(待機なし)\n");
						}
						pending_task.reset();
					} else {
						DEV_PRINT(stderr, "PLC: タスクの書き込み処理に失敗しました。\n");
						// 通信エラー等
						socket.close();
						notify_gui_thread({.type = NotificationType::CONNECTION_LOST});
						// pending_task にタスクを残しておくことで、再接続後にリトライする
					}
					if (!current_task.has_value() && !pending_task.has_value() && socket.is_connected()) {
						// 完了待ちタスクがセットされなかった場合、次のタスクを試行的に取得
						pending_task = g_task_queue.try_pop();
					}
				}
			}

			// --- B. 監視指令の取り出し ---
			// try_pop_all を使って、現在溜まっている指令を「一括」で取り出します。
			// これにより、ロックの取得回数が1回で済み、他のスレッド（GUIなど）の書き込みを阻害しません。
			auto command_queue_local = g_command_queue.try_pop_all();

			while (!command_queue_local.empty()) {
				// 先頭の要素を参照
				WatchCommand& command = command_queue_local.front();
				if (g_watchlist.count(command.addr)) {
					if (command.flags.is_active()) {
						// 既存の監視項目を更新
						auto& item = g_watchlist.at(command.addr);
						item.flags.merge(command.flags);
						item.is_first = true;
						DEV_PRINT("PLC監視項目 '{}' を更新しました。\n", command.addr);
					} else {
						// 監視項目を削除
						g_watchlist.erase(command.addr);
						DEV_PRINT("PLC監視項目 '{}' を削除しました。\n", command.addr);
					}
				} else {
					if (command.flags.is_active()) {
						// 新しい監視項目を追加
						g_watchlist.try_emplace(command.addr, plc::WatchData{.flags = command.flags, .is_first = true});
						DEV_PRINT("PLC監視項目 '{}' を追加しました。\n", command.addr);
					}
				}
				// 処理が終わったらローカルキューから削除
				command_queue_local.pop();
			}

			// =========================================================
			// 2. PLC 読み取りフェーズ
			// =========================================================

			// --- A. タスクの完了待ちチェック ---
			if (current_task.has_value() && socket.is_connected()) {
				if (current_task->wait_completion_addr.has_value()) {
					bool state = false;
					// 完了信号の状態を読み出し
					if (read_M_status(current_task->wait_completion_addr.value(), state)) {
						// 条件一致（例：完了信号がONになった）
						if (state == current_task->wait_target_state) {
							DEV_PRINT("PLC: 完了信号 M{} を検出しました。タスク完了。\n", current_task->wait_completion_addr.value());
							if (current_task->notify_completion) {
								notify_gui_thread({
									.type = NotificationType::MONITOR,
									.monitor_result = MonitorResult{
										.addr = current_task->wait_completion_addr.value(),
										.status = state
									}
								});
							}
							current_task.reset(); // タスククリア -> 次のタスクへ
						}
					} else {
						// 通信エラー等
						socket.close();
						notify_gui_thread({.type = NotificationType::CONNECTION_LOST});
						// 通信切断時は、再接続後にリトライする
					}
				} else{
					// 安全策: 完了待ちアドレスが無い場合は即座にタスク完了とする
					DEV_PRINT("PLC: タスクの完了待ちアドレスが不明なため、タスクを完了します。\n");
					current_task.reset();
					continue;
				}
			}

			// --- B. 監視処理 (g_watchlist 内のアクティブな項目全ての値を順番にPLCに問い合わせる) ---
			for (auto it = g_watchlist.begin(); (it != g_watchlist.end() && socket.is_connected()); /* ここではインクリメントしない */) {
				auto& [addr, item] = *it;

				bool current_state = false;
				if (read_M_status(addr, current_state)) {
					// is_firstフラグが立っている場合の初回チェックと、状態変化（エッジ）の検出
					if (item.is_first || current_state != item.prev_status) {
						if (current_state == true && item.flags.enable_rise_notification.value_or(false)) {
							DEV_PRINT("PLC監視 '{}': 立ち上がり(ON)を検出。\n", addr);
							notify_gui_thread(Notification{ NotificationType::MONITOR, MonitorResult{ addr, current_state } });
							if (item.flags.rise_is_one_shot.value_or(false)) item.flags.enable_rise_notification = false;
						} else if (current_state == false && item.flags.enable_fall_notification.value_or(false)) {
							DEV_PRINT("PLC監視 '{}': 立ち下がり(OFF)を検出。\n", addr);
							notify_gui_thread(Notification{ NotificationType::MONITOR, MonitorResult{ addr, current_state } });
							if (item.flags.fall_is_one_shot.value_or(false)) item.flags.enable_fall_notification = false;
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
				}
			}

			// =========================================================
			// 3. 待機フェーズ (Signalによる統合待機)
			// =========================================================

			// 現在「やるべきこと(ポーリング)」があるか判定
			bool is_active = !g_watchlist.empty() || current_task.has_value() || pending_task.has_value();

			if (is_active) {
				// 【稼働中】
				// 基本はポーリング間隔(100ms)待つ。
				// ただし、待機中に新しいタスクや指令が来たら即座に起きる。
				if (g_signal.wait_until(timer.update_target_time(POLLING_INTERVAL_MS, true))) {
					// シグナルで起床
					timer.reset();	// タイマーの基準時刻を現在にリセット
				}
			} else {
				// 【アイドル中】
				// 監視対象もなくタスクもない。
				// 次に何か指令が来るまで、CPUを使わず無限に眠る。
				g_signal.wait();
			}
		} catch (const std::exception& e) {
			DEV_PRINT(stderr, "PLCリスナースレッドで例外が発生: {}\n", e.what());
		}
	}
	DEV_PRINT("PLC統合リスナースレッドを終了します。\n");

#else

	// =========================================================================
	// スタブ実装 (PLC不要のテスト用)
	// =========================================================================
	DEV_PRINT("PLCリスナースレッドを開始します。(STUBモード)\n");
	utils::chrono::PeriodicTimer timer;

	// ワンショット監視要求(Watch)の受付時刻を記録するためのマップ
	std::map<std::uint16_t, std::chrono::steady_clock::time_point> one_shot_timers;

	// タスク管理用変数
	std::optional<Task> current_task;
	std::optional<Task> pending_task;
	// タスクの完了待ち開始時刻（完了信号がONになるまでの遅延シミュレーション用）
	std::chrono::steady_clock::time_point task_wait_start_time;

	notify_gui_thread({.type = NotificationType::CONNECTION_LOST});
	notify_gui_thread({.type = NotificationType::CONNECTION_ESTABLISHED});

	while (true) {
		const auto now = std::chrono::steady_clock::now();

		// =========================================================
		// 1. 入力処理フェーズ (タスク & 監視指令)
		// =========================================================

		// キャンセルリクエストの処理
		// フラグが立っていたら、実行中・保留中・待機列のすべてを破棄する
		if (g_cancel_requested.exchange(false)) { // 読み取ると同時に false に戻す
			DEV_PRINT("PLC: キャンセル要求を受信しました。全タスクを破棄します。\n");
			current_task.reset();	// 完了待ちの破棄 (これで無限待ちから脱出)
			pending_task.reset();	// 書き込み直前のタスクを破棄
			g_task_queue.clear();	// 待機列をすべて空にする
			notify_gui_thread({.type = NotificationType::TASK_CLEARED});
		}

		// --- A. タスクの取り出しと実行 ---
		if (!current_task.has_value()) {
			if (!pending_task.has_value()) {
				pending_task = g_task_queue.try_pop();
			}

			// ペンディング中のタスクがあれば実行試行
			while (pending_task.has_value()) {
				DEV_PRINT("PLC(STUB): 新しいタスクを開始します。\n");

				// アクション実行 (STUBモードなので send_and_receive_ack は常に成功する)
				if (pending_task->action()) {
					DEV_PRINT("PLC(STUB): タスクのアクションを実行しました。\n");

					if (pending_task->wait_completion_addr.has_value()) {
						// 完了待ちが必要な場合、Currentに昇格させて待機時間を記録
						current_task = pending_task;
						task_wait_start_time = now;
						DEV_PRINT("PLC(STUB): 完了信号 M{} の待機シミュレーションに入ります(2秒後に完了)。\n", pending_task->wait_completion_addr.value());
					} else {
						DEV_PRINT("PLC(STUB): タスク完了(待機なし)\n");
					}
					pending_task.reset();
				} else {
					// STUBでここに来ることは稀だが、失敗時は破棄して次へ
					DEV_PRINT(stderr, "PLC(STUB): タスクのアクションに失敗しました。\n");
					pending_task.reset();
					// 次のタスクがあれば即取得
					pending_task = g_task_queue.try_pop();
				}
			}
		}

		// --- B. 監視指令の取り出し ---
		auto command_queue_local = g_command_queue.try_pop_all();
		while (!command_queue_local.empty()) {
			WatchCommand& command = command_queue_local.front();
			bool is_one_shot = command.flags.rise_is_one_shot.value_or(false) || command.flags.fall_is_one_shot.value_or(false);

			if (g_watchlist.count(command.addr)) {
				if (command.flags.is_active()) {
					auto& item = g_watchlist.at(command.addr);
					item.flags.merge(command.flags);
					item.is_first = true;
					DEV_PRINT("PLC(STUB): 監視項目 '{}' を更新しました。\n", command.addr);
				} else {
					g_watchlist.erase(command.addr);
					DEV_PRINT("PLC(STUB): 監視項目 '{}' を削除しました。\n", command.addr);
				}
			} else {
				if (command.flags.is_active()) {
					g_watchlist.try_emplace(command.addr, plc::WatchData{.flags = command.flags, .is_first = true});
					DEV_PRINT("PLC(STUB): 監視項目 '{}' を追加しました。\n", command.addr);
				}
			}

			// ワンショット監視ならタイマーセット(3秒後に発火シミュレーション)
			if (is_one_shot) {
				one_shot_timers[command.addr] = now;
				DEV_PRINT("PLC(STUB): M{} の one-shot 監視シミュレーション開始。\n", command.addr);
			}
			command_queue_local.pop();
		}

		// =========================================================
		// 2. シミュレーション処理フェーズ
		// =========================================================

		// --- A. タスクの完了待ちシミュレーション ---
		if (current_task.has_value()) {
			// 4秒経過したら完了とみなす
			if (now - task_wait_start_time > std::chrono::seconds(4)) {
				DEV_PRINT("PLC(STUB): 完了信号 M{} を検出(Simulated)。タスク完了。\n", current_task->wait_completion_addr.value());

				if (current_task->notify_completion) {
					notify_gui_thread({
						.type = NotificationType::MONITOR,
						.monitor_result = MonitorResult{
							.addr = current_task->wait_completion_addr.value(),
							.status = current_task->wait_target_state
						}
					});
				}
				current_task.reset();
			}
		}

		// --- B. 監視(Watchlist)のシミュレーション ---
		for (auto it = g_watchlist.begin(); it != g_watchlist.end(); /* ループ内でインクリメント */) {
			auto& [addr, item] = *it;

			// ワンショットタイマーがセットされているか確認
			if (one_shot_timers.count(addr)) {
				// 1秒経過したら通知
				if (now - one_shot_timers.at(addr) > std::chrono::seconds(1)) {
					// 立ち上がり(rise)通知のシミュレート
					if (item.flags.rise_is_one_shot.value_or(false) && item.flags.enable_rise_notification.value_or(false)) {
						DEV_PRINT("PLC(STUB): M{} の one-shot (rise) を通知します。\n", addr);
						notify_gui_thread(Notification{ NotificationType::MONITOR, MonitorResult{ addr, true } });
						item.flags.enable_rise_notification = false; // 一度通知したら無効化
					}
					// 立ち下がり(fall)通知のシミュレート
					if (item.flags.fall_is_one_shot.value_or(false) && item.flags.enable_fall_notification.value_or(false)) {
						DEV_PRINT("PLC(STUB): M{} の one-shot (fall) を通知します。\n", addr);
						notify_gui_thread(Notification{ NotificationType::MONITOR, MonitorResult{ addr, false } });
						item.flags.enable_fall_notification = false; // 一度通知したら無効化
					}
					// シミュレートが完了したのでタイマーから削除
					one_shot_timers.erase(addr);
				}
			}

			// 監視フラグが全て無効になった項目はリストから削除
			if (!item.flags.is_active()) {
				it = g_watchlist.erase(it);
			} else {
				++it;
			}
		}

		// =========================================================
		// 3. 待機フェーズ (Signalによる統合待機)
		// =========================================================

		// 何か処理中(タスク実行中、ペンディングあり、監視中)ならポーリング待機、そうでなければ無限待機
		bool is_active = !g_watchlist.empty() || current_task.has_value() || pending_task.has_value();

		if (is_active) {
			// ポーリング周期待機 (タスク追加などでシグナルが来たら即起きる)
			if (g_signal.wait_until(timer.update_target_time(POLLING_INTERVAL_MS, true))) {
				timer.reset();
			}
		} else {
			// アイドル待機
			g_signal.wait();
			timer.reset();
		}
	}

#endif
}

// --- 公開関数 ---

void initialize() {
	// どちらのキューに追加があっても、同じシグナルに通知を送る
	auto notifier = []() { g_signal.notify_one(); };
	g_command_queue.set_notifier(notifier);
	g_task_queue.set_notifier(notifier);

	// 監視リストを初期化
	g_watchlist.insert(std::make_pair(M_DEVICE_MAINTENANCE_MODE, WatchData{
		.flags = WatchFlags{ true, true, false, false},
		.is_first = true
	}));
	g_watchlist.insert(std::make_pair(M_DEVICE_ERROR, WatchData{
		.flags = WatchFlags{ true, true, false, false},
		.is_first = true
	}));

	// 単一の統合スレッドを起動
	std::thread(run_communicator).detach();
}

bool read_M_status(std::uint16_t addr, bool& is_on) {
	std::lock_guard<std::mutex> lock(socket_mutex);
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

bool read_D_word(std::uint16_t addr, std::uint16_t& out_data) {
	std::lock_guard<std::mutex> lock(socket_mutex);
	std::vector<std::uint8_t> cmd = make_read_D_command(addr);

	if (socket.send(cmd.data(), cmd.size()) < 0) {
		DEV_PRINT(stderr, "PLCへのD読み出しコマンド送信エラー\n");
		return false;
	}

	std::uint8_t resp[1024];
	int n = socket.receive(resp, sizeof(resp));
	if (n <= 0) {
		DEV_PRINT(stderr, "PLCからのD読み出し応答受信エラー\n");
		return false;
	}

	if (resp[0] != 0x81 || resp[1] != 0x00 || n < 4) {
		std::uint16_t error_code = resp[0] | (resp[1] << 8);
		auto it = mc_error_map.find(error_code);
		if (it != mc_error_map.end()) {
			DEV_PRINT(stderr, "PLCエラー(D読出) 0x{:04X}: {}\n", error_code, it->second);
		} else {
			DEV_PRINT(stderr, "PLCから未知のエラーコードを受信(D読出): 0x{:04X}\n", error_code);
		}
		return false;
	}

	// データ部 (リトルエンディアン) の復元
	out_data = resp[2] | (resp[3] << 8);
	return true;
}

void start_session() {
	// 1. 新しいセッションを開始するので、世代を進める
	// これにより、過去に積まれた end_session タスクは「世代不一致」で無効化される
	g_session_generation++;

	g_task_queue.push(Task{
		.action = [](){
			DEV_PRINT("PLC: セッション開始通知を行います。\n");
			return write_M_status(M_DEVICE_SESSION_START, true);
		}
	});
}

void end_session() {
	// この終了要求が発行された時点の世代を記録する
	std::uint32_t target_gen = g_session_generation.load();

	g_task_queue.push(Task{
		.action = [target_gen](){
			// 【重要】実行直前に有効性をチェック
			// もし start_session が後から呼ばれていれば、g_session_generation は増えているはず
			if (target_gen != g_session_generation.load()) {
				DEV_PRINT("PLC: キャンセルされたセッション終了要求を破棄します (ReqGen:{} != CurrGen:{})\n", target_gen, g_session_generation.load());
				return true; // 処理済みとしてtrueを返し、何も書き込まない
			}
			DEV_PRINT("PLC: セッション終了実行 (Gen:{})\n", target_gen);
			return write_M_status(M_DEVICE_SESSION_END, true);
		}
	});
}

void retrieve_container(std::uint16_t container_number, bool notify_completion) {
	g_task_queue.push(Task{
		.action = [container_number](){
			DEV_PRINT("PLC: コンテナ({})の搬出要求を開始します。\n", container_number);
			if (!write_D_word(D_DEVICE_CONTAINER_ADDRESS, container_number)) {
				DEV_PRINT(stderr, "	 エラー: D{} コンテナアドレス{}の書き込みに失敗しました。\n", D_DEVICE_CONTAINER_ADDRESS, container_number);
				return false;
			}
			DEV_PRINT("	 - D{} アドレス{} 書き込み成功\n", D_DEVICE_CONTAINER_ADDRESS, container_number);
			if (!write_M_status(M_DEVICE_RETRIEVE_START, true)) {
				DEV_PRINT(stderr, "	 エラー: M{} (搬出開始トリガ)のONに失敗しました。\n", M_DEVICE_RETRIEVE_START);
				return false;
			}
			DEV_PRINT("	 - M{} ON 成功\n", M_DEVICE_RETRIEVE_START);
			return true;
		},
		.wait_completion_addr = M_DEVICE_RETRIEVE_COMPLETED,
		.wait_target_state = true,
		.notify_completion = notify_completion
	});
}

void store_container(bool notify_completion) {
	g_task_queue.push(Task{
		.action = [](){
			DEV_PRINT("PLC: コンテナの搬入要求を開始します。\n");
			return write_M_status(M_DEVICE_STORE_START, true);
		},
		.wait_completion_addr = M_DEVICE_STORE_COMPLETED,
		.wait_target_state = true,
		.notify_completion = notify_completion
	});
}

void clear_task_queue() {
	// 1. 待機列を即座にクリア (スレッドが起きるのを待たずに消す)
	g_task_queue.clear();

	// 2. スレッドに対して「現在持っているタスクも捨てろ」と指示
	g_cancel_requested = true;

	// 3. スレッドが寝ている(wait中)場合に備えて叩き起こす
	g_signal.notify_all();

	DEV_PRINT("PLC: 全タスクの中止を要求しました。\n");}

} // namespace plc
