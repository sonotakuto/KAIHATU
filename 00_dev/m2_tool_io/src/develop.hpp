/**
 * @file develop.hpp
 * @brief 開発用のデバッグマクロを定義するヘッダファイル。
 */
#pragma once

#ifndef DEV
#define DEV
#endif

#ifdef DEV
	/**
	 * @brief デバッグ用の標準出力マクロ。
	 * @details DEVが定義されている場合にのみ有効。fmtライブラリを使用してフォーマットされた文字列を出力します。
	 * @param ... fmt::printに渡す引数 (フォーマット文字列と可変引数)
	 */
	#define DEV_PRINT(...) fmt::print(__VA_ARGS__)
#else
	/**
	 * @brief デバッグ用の標準出力マクロ。
	 * @details DEVが定義されている場合にのみ有効。fmtライブラリを使用してフォーマットされた文字列を出力します。
	 * @param ... fmt::printに渡す引数 (フォーマット文字列と可変引数)
	 */
	#define DEV_PRINT(...) (void)0
#endif