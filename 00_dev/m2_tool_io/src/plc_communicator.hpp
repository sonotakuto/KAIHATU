/**
 * @file plc_communicator.hpp
 * @brief PLCとの通信を通じて、コンテナの搬出・搬入を制御する機能を提供するモジュール。
 */
#pragma once
#include <any>
#include <cstdint>
#include "safe_queue.hpp"

// PLCを使わずにテストしたいときに定義する。
#ifndef STUB_PLC
#define STUB_PLC	// TODO: リリース時はコメントアウト
#endif

/**
 * @namespace plc
 * @brief PLCとの連携に関連する関数と定数を格納する名前空間。
 */
namespace plc {

// --- 型定義 ---

struct WatchFlags {
	std::optional<bool>	enable_fall_notification{};	// ONからOFFになった時にGUIに通知するか
	std::optional<bool>	enable_rise_notification{};	// OFFからONになった時にGUIに通知するか
	std::optional<bool>	fall_is_one_shot{};			// trueの場合、一度条件が成立したら enable_fall_notification をfalseにする
	std::optional<bool>	rise_is_one_shot{};			// trueの場合、一度条件が成立したら rise_is_one_shot をfalseにする

	bool is_active() const {
		return enable_fall_notification.value_or(false) && enable_rise_notification.value_or(false);
	}

	/**
	 * @brief 他の WatchFlags の値でこのインスタンスを更新する。
	 * @param other 更新に使用する WatchFlags インスタンス。
	 * @return 変更があったかどうか (true: 変更あり, false: 変更なし)
	 */
	bool merge(const WatchFlags& other);
};

/**
 * @struct WatchItem
 * @brief PLCの状態監視リストの各項目を定義する構造体。
 * @details 立ち上がり/立ち下がりエッジを検出し、個別の通知を送信できる。
 */
struct WatchData {
	WatchFlags	flags{};		// 監視対象のMデバイスアドレス
	bool		prev_status{};	// 前回の状態。通知トリガの検知用。
	bool		is_first{};		// flags を外部から更新する際 true にする。これがtrueのとき、現在の状態に応じて必ず通知を試行し、試行後このフラグをfalseにする。
};

/**
 * @struct Command
 * @brief g_watchlist をどう操作するか格納する構造体
 */
struct WatchCommand {
	std::uint16_t	addr{};		// 監視対象
	WatchFlags		flags{};	// 有効にするか無効にするか
};

/**
 * @enum NotificationType
 * @brief PLCからGUIへの通知の種類
 */
enum class NotificationType {
	NONE,
	CONNECTION_LOST,		// PLCとの接続が切れた
	CONNECTION_ESTABLISHED,	// PLCとの接続が確立した
	MONITOR					// 監視項目で通知が発生した
};

struct MonitorResult {
	std::uint16_t	addr{};		// 監視対象
	bool			status{};	// 監視対象の状態
};

/**
 * @struct Notification
 * @brief PLCからGUIへの通知内容
 */
struct Notification {
	NotificationType				type{NotificationType::NONE};
	std::optional<MonitorResult>	monitor_result{}; // 監視結果の通知の場合に結果を格納
};

// --- 定数定義 ---

constexpr const char*	HOST	= "10.19.133.181";	// PLCのホスト名
constexpr int			PORT	= 10000;			// PLCのポート番号

constexpr std::chrono::milliseconds	POLLING_INTERVAL_MS(100);	// PLCのポーリング間隔 (ミリ秒)

// Mデバイスのアドレス定義
constexpr std::uint16_t M_DEVICE_RETRIEVE_COMPLETED	= 80;	// 搬出完了信号
constexpr std::uint16_t M_DEVICE_STORE_COMPLETED	= 81;	// 搬入完了信号
constexpr std::uint16_t M_DEVICE_SESSION_START		= 82;	// GUIセッション開始信号
constexpr std::uint16_t M_DEVICE_SESSION_END		= 83;	// GUIセッション終了信号
constexpr std::uint16_t M_DEVICE_RETRIEVE_START		= 91;	// 搬出開始トリガ
constexpr std::uint16_t M_DEVICE_STORE_START		= 92;	// 搬入開始トリガ
constexpr std::uint16_t M_DEVICE_MAINTENANCE_MODE	= 93;	// メンテナンスモード信号

#ifdef STUB_PLC
constexpr std::uint16_t M_DEVICE_DOOR_OPEND		= 0;	// ドアの状態 // TODO: 決まったらアドレスを設定する
#else
//constexpr std::uint16_t M_DEVICE_DOOR_OPEND		= xx;	// ドアの状態 // TODO: 決まったらアドレスを設定する
#endif

// --- PLC通信機能の使用に必要なグローバル変数を定義 ---

/// @brief GUIスレッドからPLCリスナースレッドへの指令を格納するスレッドセーフなキュー。
inline utils::SafeQueue<WatchCommand> g_command_queue;

/// @brief PLCの状態監視リスト。キーは監視内容のユニークな名前。
inline std::map<std::uint16_t, WatchData> g_watchlist;

// --- 関数宣言 ---

/**
 * @brief PLC通信モジュールを初期化し、単一の統合リスナースレッドを開始する。
 * @details アプリケーション起動時に一度だけ呼び出す。
 */
void initialize();

/**
 * @brief PLCにセッションの開始を通知します。
 * @details セッション開始を意味するM82をONにします。
 * @return 処理が成功した場合はtrue、失敗した場合はfalse。
 */
bool start_session();

/**
 * @brief PLCにセッションの終了を通知します。
 * @details セッション終了を意味するM83をONにします。
 * @return 処理が成功した場合はtrue、失敗した場合はfalse。
 */
bool end_session();

/**
 * @brief PLCにコンテナの搬出を要求します。
 * @details 指定されたコンテナ番号に対応するMデバイスと、搬出開始を指示するM91をONにします。
 * @param container_number 搬出するコンテナ番号。
 * @return 処理が成功した場合はtrue、失敗した場合はfalse。
 */
bool retrieve_container(std::uint16_t container_number);

/**
 * @brief PLCにコンテナの搬入を要求します。
 * @details 搬入開始を指示するM92をONにします。
 * @return 処理が成功した場合はtrue、失敗した場合はfalse。
 */
bool store_container();

} // namespace plc