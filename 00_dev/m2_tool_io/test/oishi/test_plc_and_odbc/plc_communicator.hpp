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
 * @struct WatchItemName
 * @brief 監視リストの項目名を定義するenum
 */
enum class WatchItemName {
	WAIT_RETRIEVE,
	WAIT_STORE,
	MONITOR_MAINTENANCE
};

/**
 * @struct WatchItem
 * @brief PLCの状態監視リストの各項目を定義する構造体。
 * @details 立ち上がり/立ち下がりエッジを検出し、個別の通知を送信できる。
 */
struct WatchItem {
	std::uint16_t			address;			// 監視対象のMデバイスアドレス
	std::function<void()>	on_fall{nullptr};	// ONからOFFになった時の実行内容。nullptr なら何も実行しない
	std::function<void()>	on_rise{nullptr};	// OFFからONになった時の実行内容。nullptr なら何も実行しない
	bool					prev_status{false};	// 前回の状態。通知トリガの検知用。
	bool					is_first{false};	// is_activeをtrueにする際trueにする。これがtrueのとき、現在の状態に応じて必ず通知を試行し、試行後このフラグをfalseにする。
	bool					is_active{false};	// この監視項目が現在有効かどうかのフラグ
	bool					is_one_shot{false};	// trueの場合、一度条件が成立したら is_active をfalseにする
};

/**
 * @struct Command
 * @brief g_watchlist をどう操作するか格納する構造体
 */
struct Command {
	WatchItemName				watch_item_name;	// 監視項目のキー
	std::optional<WatchItem>	watch_item;			// 監視内容 (nulloptなら解除)
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

// --- PLC通信機能の使用に必要なグローバル変数を定義 ---

/// @brief GUIスレッドからPLCリスナースレッドへの指令を格納するスレッドセーフなキュー。
inline utils::SafeQueue<Command> g_command_queue;

/// @brief PLCの状態監視リスト。キーは監視内容のユニークな名前。
inline std::map<WatchItemName, WatchItem> g_watchlist;

// --- 関数宣言 ---

/**
 * @brief PLC通信モジュールを初期化し、単一の統合リスナースレッドを開始する。
 * @details アプリケーション起動時に一度だけ呼び出す。
 */
void initialize();

} // namespace plc