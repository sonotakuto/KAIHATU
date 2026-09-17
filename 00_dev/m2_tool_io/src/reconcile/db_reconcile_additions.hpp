/**
 * @file db_reconcile_additions.hpp
 * @brief 棚卸し機能のために db.hpp / db.cpp へ追加が必要なインターフェース定義。
 * @details
 * ★★★ 本ファイルは単体では使用しません ★★★
 * 既存の db.hpp (未提供のため本プロジェクトには存在しません) に、以下の宣言を追加し、
 * 対応する実装を db.cpp 側 (DB用筐体PCとのローカル接続処理) に追加してください。
 *
 * 実装にあたっての要件 (呼び出し元 tool_io_gui.cpp の使用方法から逆算したもの):
 *
 * 1. get_StorageAreaInfoList_for_reconcile()
 *    - 棚卸し対象として選択可能な保管領域 (コンテナ) の一覧を取得する。
 *    - 既存の get_StorageAreaInfo_with_max_cases() / _with_min_cases() は
 *      「特定の基本工具IDに対応するコンテナを1件返す」用途のため、
 *      棚卸しの「コンテナ一覧をそのまま見せる」用途には流用できません。
 *    - 取得失敗時 (DB接続断など) は std::nullopt を返してください。
 *      (0件ヒットと接続失敗を区別するため、0件時は空vectorを返してください)
 *
 * 2. get_UniqueToolInfoList_by_StorageArea(storage_area_id)
 *    - 指定した保管領域IDに、現在「保管中(StorageCondition::STORED)」として
 *      登録されている個別工具の一覧を取得する。
 *    - これが棚卸しの「期待リスト (このコンテナにあるはずの工具)」の元データになります。
 *
 * 3. update_last_reconciliation_time(storage_area_id, time)
 *    - 指定した保管領域の最終棚卸し日時 (StorageAreaTable::last_reconciliation_time) を更新する。
 *    - コンテナ選択画面 (RECONCILE_WAITING_FOR_CONTAINER_SELECTION) の一覧に表示される
 *      「最終棚卸し日時」列は、この値を参照します。
 *
 * 既存の db::get_UniqueToolInfo() / db::save_history() は棚卸しでもそのまま再利用しています。
 * (save_history には OperationClass::INVENTORY のレコードを渡しています)
 */
#pragma once
#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>
#include "typedef.hpp"

namespace db {

/**
 * @brief 棚卸し対象として選択可能な保管領域 (コンテナ) の一覧を取得します。
 * @return 保管領域情報のリスト。取得に失敗した場合は std::nullopt。
 */
std::optional<std::vector<StorageAreaInfo>> get_StorageAreaInfoList_for_reconcile();

/**
 * @brief 指定した保管領域に「現在保管されているはず」の個別工具一覧を取得します。
 * @param[in] storage_area_id 対象の保管領域ID。
 * @return 個別工具情報のリスト。取得に失敗した場合は std::nullopt。
 */
std::optional<std::vector<UniqueToolInfo>> get_UniqueToolInfoList_by_StorageArea(std::int32_t storage_area_id);

/**
 * @brief 指定した保管領域の最終棚卸し日時を更新します。
 * @param[in] storage_area_id 対象の保管領域ID。
 * @param[in] time 記録する日時 (通常は sg::now() の結果)。
 * @return 更新に成功した場合は true。
 */
bool update_last_reconciliation_time(std::int32_t storage_area_id, std::chrono::system_clock::time_point time);

} // namespace db
