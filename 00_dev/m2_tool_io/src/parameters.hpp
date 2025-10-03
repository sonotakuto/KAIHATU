/**
 * @file includes.hpp
 * @brief パラメータをまとめて定義するヘッダファイル。
 */
#pragma once

//#define SAMPLE 0	// 説明
#define DB_RETRY_LIMIT 3	// DB接続のリトライ回数

// 色定義
// TODO: 色は後で調整する
constexpr GdkRGBA TABLE_BGCOLOR_NORMAL				= {0.914, 0.922, 0.961, 1.0};	// テーブルの通常背景色
constexpr GdkRGBA TABLE_BGCOLOR_HIGHLIGHT			= {0.914, 0.922, 0.961, 1.0};	// テーブルの強調表示背景色
constexpr GdkRGBA TABLE_BGCOLOR_FADED				= {0.914, 0.922, 0.961, 1.0};	// テーブルの薄く表示された背景色
constexpr GdkRGBA TABLE_BGCOLOR_RECONCILE_MATCH		= {0.914, 0.922, 0.961, 1.0};	// テーブルの棚卸し一致行背景色
constexpr GdkRGBA TABLE_BGCOLOR_RECONCILE_MISMATCH	= {0.914, 0.922, 0.961, 1.0};	// テーブルの棚卸し不一致行背景色
constexpr GdkRGBA TABLE_BGCOLOR_RECONCILE_UNREAD	= {0.914, 0.922, 0.961, 1.0};	// テーブルの棚卸し未読行背景色

// static parameters
//#define STATIC_SAMPLE 0	// 説明
#define ODBC_DSN	"odbc_test"	// ODBCのDSN名
#define ODBC_USER	"pi"		// ODBCのユーザー名
#define ODBC_PASS	"pipasswd"	// ODBCのパスワード

#define TRACKER_HOST	"localhost"	// 工具追跡システムのホスト名
#define TRACKER_PORT	10000		// 工具追跡システムのポート番号

#define DISPLAY_ROWS_PER_COLUMN 11	// 表1列あたりの表示行数