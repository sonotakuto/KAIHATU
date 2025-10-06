/**
 * @file plc_communicator.hpp
 * @brief PLCとの通信を通じて、コンテナの搬出・搬入を制御する機能を提供するモジュール。
 */
#pragma once
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
 * @brief PLCへの指令の種類を定義するenum
 */
enum class PlcCommandType {
	NONE,
	WAIT_FOR_COMPLETION,
	RETRIEVE_CONTAINER,
	STORE_CONTAINER
	// 将来的に他の指令も追加可能
};

/**
 * @struct PlcCommand
 * @brief PLCへの指令内容を格納する構造体
 */
struct PlcCommand {
	PlcCommandType	type{PlcCommandType::NONE};
	std::uint16_t	data{}; // uint16_t のアドレスやコンテナ番号などを格納
};

// --- 定数定義 ---

constexpr int	POLLING_INTERVAL_MS = 100;	// PLCのポーリング間隔 (ミリ秒)

// Mデバイスのアドレス定義
constexpr std::uint16_t M_DEVICE_RETRIEVE_COMPLETED	= 80; // 搬出完了信号
constexpr std::uint16_t M_DEVICE_STORE_COMPLETED	= 81; // 搬入完了信号
constexpr std::uint16_t M_DEVICE_RETRIEVE_START		= 91; // 搬出開始トリガ
constexpr std::uint16_t M_DEVICE_STORE_START		= 92; // 搬入開始トリガ

// --- PLC通信機能の使用に必要なグローバル変数を定義 ---

/// @brief GUIスレッドからPLCリスナースレッドへの指令を格納するスレッドセーフなキュー。
inline utils::SafeQueue<PlcCommand> g_plc_command_queue;

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

/**
 * @brief 指定されたMデバイスがONになるのを待機します。
 * @details PLCからの完了信号を待つために使用します。ポーリング間隔は0.1秒です。
 * @param address 監視対象のMデバイスのアドレス。
 * @return 待機が成功（ONを検出）した場合はtrue、通信エラー等で失敗した場合はfalse。
 */
bool waiting_for_completion(std::uint16_t address);

} // namespace plc