/**
 * @file
 * @brief
 */
#pragma once
#include <SimpleGtk>
#include "./typedef.hpp"
#include "./parameters.hpp"

/**
 * @namespace tracker
 * @brief 工具追跡システムとの連携に関連する関数と定数を格納する名前空間。
**/
namespace tracker {

/*
constexpr const char*	HOST	= "localhost";	// 工具追跡システムのホスト名
constexpr int			PORT	= 10000;		// 工具追跡システムのポート番号
*/

bool connect_to_tracker();
void run_tracking_listener();
} // namespace tracker