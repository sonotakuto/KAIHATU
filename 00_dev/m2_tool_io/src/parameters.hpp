/**
 * @file includes.hpp
 * @brief パラメータをまとめて定義するヘッダファイル。
 * @details 現在は変更頻度が高いであろうGUIの見た目に影響する定数のみ残し、
 * 他のパラメータはそれぞれの定数を用いる関数のヘッダファイルで定義している。
 */
#pragma once

//inline constexpr int	SAMPLE	= 0;	// 説明

inline constexpr int	MONITOR_WIDTH = 1920;	// モニターの幅（ピクセル）
inline constexpr int	MONITOR_HEIGHT = 1080;	// モニターの高さ（ピクセル）

inline constexpr const char* TOOL_CATEGORY_INSERT = "インサート";	// インサート専用の処理を行う必要がある、データベース上の工具種別名

inline constexpr int	DISPLAY_ROWS_PER_COLUMN = 11;	// 表1列あたりの表示行数
inline constexpr int	NUM_OF_VIEW_COLUMNS = 6;		// メインテーブルの表示列数

// 色定義
inline constexpr const char* CSS_BGCOLOR_WARNING = "background-color_warning";

inline constexpr GdkRGBA TABLE_BGCOLOR_NORMAL				= {0.97, 0.97, 1.00, 1.0};	// テーブルの通常背景色
inline constexpr GdkRGBA TABLE_BGCOLOR_HIGHLIGHT			= {1.00, 0.56, 0.00, 1.0};	// テーブルの強調表示背景色
inline constexpr GdkRGBA TABLE_BGCOLOR_FADED				= {0.69, 0.77, 0.87, 1.0};	// テーブルの薄く表示された背景色
inline constexpr GdkRGBA TABLE_BGCOLOR_RECONCILE_MATCH		= {0.00, 0.88, 0.00, 1.0};	// テーブルの棚卸し一致行背景色
inline constexpr GdkRGBA TABLE_BGCOLOR_RECONCILE_MISMATCH	= {1.00, 0.65, 0.00, 1.0};	// テーブルの棚卸し不一致行背景色
inline constexpr GdkRGBA TABLE_BGCOLOR_RECONCILE_UNREAD	= {0.69, 0.77, 0.87, 1.0};	// テーブルの棚卸し未読行背景色

inline constexpr int	INSERT_COUNT_MAX_RECONCILE = 99;	// 棚卸しでのインサート個数入力の上限
