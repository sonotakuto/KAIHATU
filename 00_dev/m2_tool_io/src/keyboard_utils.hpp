/**
 * @file keyboard_utils.hpp
 * @brief onboardキーボードの起動・表示・非表示を制御するユーティリティ関数群。
 */
#pragma once

/**
 * @namespace onboard
 * @brief onboardキーボードの起動・表示・非表示を制御する関数群を格納する名前空間。
 */
namespace onboard {

/**
 * @brief onboardが起動していない場合、バックグラウンドで起動します。
 * 既に起動している場合は何もしません。
 */
void ensure_running();

/**
 * @brief onboardを表示します(可視化)。
 */
void show();

/**
 * @brief onboardを非表示にします(隠す)。
 */
void hide();

}
