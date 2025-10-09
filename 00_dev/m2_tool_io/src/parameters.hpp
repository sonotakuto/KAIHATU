/**
 * @file includes.hpp
 * @brief パラメータをまとめて定義するヘッダファイル。
 * @details 現在は変更頻度が高いであろうGUIの見た目に影響する定数のみ残し、
 * 他のパラメータはそれぞれの定数を用いる関数のヘッダファイルで定義している。
 */
#pragma once

//constexpr int	SAMPLE	= 0;	// 説明

constexpr int	DISPLAY_ROWS_PER_COLUMN = 11;	// 表1列あたりの表示行数

// 色定義
// TODO: 色は後で調整する
constexpr GdkRGBA TABLE_BGCOLOR_NORMAL				= {0.914, 0.922, 0.961, 1.0};	// テーブルの通常背景色
constexpr GdkRGBA TABLE_BGCOLOR_HIGHLIGHT			= {0.914, 0.922, 0.961, 1.0};	// テーブルの強調表示背景色
constexpr GdkRGBA TABLE_BGCOLOR_FADED				= {0.914, 0.922, 0.961, 1.0};	// テーブルの薄く表示された背景色
constexpr GdkRGBA TABLE_BGCOLOR_RECONCILE_MATCH		= {0.914, 0.922, 0.961, 1.0};	// テーブルの棚卸し一致行背景色
constexpr GdkRGBA TABLE_BGCOLOR_RECONCILE_MISMATCH	= {0.850, 0.850, 0.850, 1.0};	// テーブルの棚卸し不一致行背景色
constexpr GdkRGBA TABLE_BGCOLOR_RECONCILE_UNREAD	= {0.914, 0.922, 0.961, 1.0};	// テーブルの棚卸し未読行背景色

/**
 * @namespace tracker
 * @brief 工具追跡システムとの連携に関連する関数と定数を格納する名前空間。
 */
namespace tracker {
constexpr const char*	HOST	= "localhost";	// 工具追跡システムのホスト名
constexpr int			PORT	= 10000;		// 工具追跡システムのポート番号
} // namespace tracker
