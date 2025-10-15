/**
 * @file plc_communicator.hpp
 * @brief PLCとの通信を通じて、コンテナの搬出・搬入を制御する機能を提供するモジュール。
 */
#pragma once
#include <any>
#include <cstdint>
#include "safe_queue.hpp"

/**
 * @namespace plc
 * @brief PLCとの連携に関連する関数と定数を格納する名前空間。
 */
namespace plc {

// --- 型定義 ---

/**
 * @enum PlcCommandType
 * @brief GUIからPLCへの指令の種類を定義するenum
 */
enum class CommandType {
	NONE,
	SET_WATCH_ITEM_ACTIVE,	// 監視項目の有効/無効を切り替える
	START_SESSION,			// M82 ON: GUIセッション開始
	END_SESSION,			// M83 ON: GUIセッション終了
	RETRIEVE_CONTAINER,		// コンテナ搬出
	STORE_CONTAINER			// コンテナ搬入
};

/**
 * @struct SetWatchActiveCmd
 * @brief SET_WATCH_ITEM_ACTIVE 指令のデータを格納する構造体
 */
struct SetWatchActiveCmd {
	std::string watch_item_name; // どの監視項目を操作するか
	bool		is_active;		 // 有効にするか無効にするか
};

/**
 * @struct Command
 * @brief PLCへの指令内容を格納する構造体
 */
struct Command {
	CommandType	type{CommandType::NONE};	// PLCへの指令の種類
	std::any	data{};						// SetWatchActiveCmd や コンテナ番号(uint16_t) などを格納
};

/**
 * @enum NotificationType
 * @brief PLCからGUIへの通知の種類
 */
enum class NotificationType {
	NONE,
	TASK_SUCCESS,			// 指令が成功した
	TASK_FAILURE,			// 指令が失敗した
	MAINTENANCE_MODE_ON,	// メンテナンスモードが開始された (M93 ON)
	MAINTENANCE_MODE_OFF,	// メンテナンスモードが終了した (M93 OFF)
	CONNECTION_LOST,		// PLCとの接続が切れた
	CONNECTION_ESTABLISHED	// PLCとの接続が確立した
};

/**
 * @struct Notification
 * @brief PLCからGUIへの通知内容
 */
struct Notification {
	NotificationType	type{NotificationType::NONE};
	std::any			data; // 完了した指令の種類(PlcCommandType)などを格納
};

/**
 * @struct WatchItem
 * @brief PLCの状態監視リストの各項目を定義する構造体。
 * @details 立ち上がり/立ち下がりエッジを検出し、個別の通知を送信できる。
 */
struct WatchItem {
	std::uint16_t				address;			// 監視対象のMデバイスアドレス
	std::optional<Notification>	notification_fall;	// ONからOFFになった時にGUIに送る通知内容。nulloptなら通知しない
	std::optional<Notification>	notification_rise;	// OFFからONになった時にGUIに送る通知内容。nulloptなら通知しない
	bool						prev_status{false};	// 前回の状態。通知トリガの検知用。
	bool						is_first{false};	// is_activeをtrueにする際trueにする。これがtrueのとき、現在の状態に応じて必ず通知を試行し、試行後このフラグをfalseにする。
	bool						is_active{false};	// この監視項目が現在有効かどうかのフラグ
	bool						is_one_shot{false};	// trueの場合、一度条件が成立したら is_active をfalseにする
};

// --- 定数定義 ---

constexpr const char*	HOST	= "10.19.133.181";	// PLCのホスト名
constexpr int			PORT	= 10000;			// PLCのポート番号

constexpr int	POLLING_INTERVAL_MS = 100;	// PLCのポーリング間隔 (ミリ秒)

// Mデバイスのアドレス定義
constexpr std::uint16_t M_DEVICE_RETRIEVE_COMPLETED	= 80;	// 搬出完了信号
constexpr std::uint16_t M_DEVICE_STORE_COMPLETED	= 81;	// 搬入完了信号
constexpr std::uint16_t M_DEVICE_SESSION_START		= 82;	// GUIセッション開始信号
constexpr std::uint16_t M_DEVICE_SESSION_END		= 83;	// GUIセッション終了信号
constexpr std::uint16_t M_DEVICE_MAINTENANCE_MODE	= 93;	// メンテナンスモード信号
constexpr std::uint16_t M_DEVICE_RETRIEVE_START		= 91;	// 搬出開始トリガ
constexpr std::uint16_t M_DEVICE_STORE_START		= 92;	// 搬入開始トリガ

// --- PLC通信機能の使用に必要なグローバル変数を定義 ---

/// @brief GUIスレッドからPLCリスナースレッドへの指令を格納するスレッドセーフなキュー。
inline utils::SafeQueue<Command> g_command_queue;

/// @brief PLCの状態監視リスト。キーは監視内容のユニークな名前。
inline std::map<std::string, WatchItem> g_watchlist;

// --- 関数宣言 ---

/**
 * @brief PLC通信モジュールを初期化し、単一の統合リスナースレッドを開始する。
 * @details アプリケーション起動時に一度だけ呼び出す。
 */
void initialize();

} // namespace plc