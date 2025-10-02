#pragma once
#include "includes.hpp"
#include "typedef.hpp"

class ToolHandling
{
public:
    // ■■■■ 課題 (ステップ1-2) ここから追加 ■■■■

    /**
     * @brief 工具取り出しプロセスのメイン関数
     * @param qr_code ユーザーがスキャンしたプログラム割当一覧表のQRコード
     */
    void retrieve_tool(const string &qr_code);

private:
    /**
     * @brief ユーザーに工具を選択させるUI処理とロジック
     * @param qr_code プログラム割当一覧表のQRコード
     * @return optional<vector<Tool>> ユーザーが選択した工具のリスト
     */
    optional<vector<Tool>> select_retrieval_tool(const string &qr_code);

    /**
     * @brief 物理的な工具の取り出しを確認・記録する
     * @param tool_to_verify 取り出し対象の工具情報
     * @return bool ユーザーが「続ける」を選択した場合はtrue、「終了」の場合はfalse
     */
    bool verify_tool_checkout(const Tool &tool_to_verify);

    // ■■■■ 課題 (ステップ1-2) ここまで追加 ■■■■
};