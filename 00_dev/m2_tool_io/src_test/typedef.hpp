#pragma once
#include "includes.hpp"

// ■■■■ 課題 (ToolingList未定義エラー対応) ここから追加 ■■■■

/**
 * @brief 個々の工具情報を格納する構造体
 */
struct Tool
{
    string tool_id;        // T番号 (例: "T1")
    string tool_name;      // 分類 (例: "ドリル")
    string maker;          // メーカー
    string model_number;   // 型番
    string material;       // 材質
    int stock_quantity;    // 在庫数
};

/**
 * @brief ツーリングリスト全体（紐づく工具リストを含む）を格納する構造体
 */
struct ToolingList
{
    string line_name;              // ライン番号 (例: "MC8")
    string program_number;         // プログラム番号 (例: "O0201")
    vector<Tool> tools;            // 上記のプログラムに紐づく工具のリスト
};

// ■■■■ 課題 (ToolingList未定義エラー対応) ここまで追加 ■■■■

struct ToolData
{
    /* data */
};

struct ToolHistory
{
    /* data */
};