/**
 * @file includes.hpp
 * @brief パラメータをまとめて定義するヘッダファイル。
 */
#pragma once

//#define SAMPLE 0	// 説明
#define DB_RETRY_LIMIT 3	// DB接続のリトライ回数

// static parameters
//#define STATIC_SAMPLE 0	// 説明
#define ODBC_DSN "odbc_test"	// ODBCのDSN名
#define ODBC_USER "pi"			// ODBCのユーザー名
#define ODBC_PASS "pipasswd"	// ODBCのパスワード

#define DISPLAY_ROWS_PER_COLUMN 11	// 表1列あたりの表示行数