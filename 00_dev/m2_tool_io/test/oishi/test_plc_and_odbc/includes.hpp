/**
 * @file includes.hpp
 * @brief 複数の実装ファイルで共通してインクルードするヘッダファイルをまとめたヘッダファイル。
 */
#pragma once

// glob_var.hpp以外の全てのヘッダファイルをインクルードする
#include <SimpleGtk>
#include <optional>
#include "./typedef.hpp"
#include "./parameters.hpp"
#include "./develop.hpp"

// 最後にglob_var.hppをインクルード
#include "./glob_var.hpp"
