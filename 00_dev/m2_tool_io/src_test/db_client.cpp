#include "db_client.hpp"
#include <sstream>

// (コンストラクタなどは変更なし)
// ...

/**
 * @brief QRコードに対応するツーリングリストを取得する
 * @param qr_code プログラム割当一覧表のQRコード情報 (形式: "ライン番号;プログラム番号")
 * @return ToolingList ツーリングリストの情報。見つからない場合はnullopt
 */
optional<ToolingList> DbClient::get_tooling_list(const string &qr_code)
{
    // ■■■■ 課題 (ダミー動作) ここから修正 ■■■■

    // QRコードを';'で分割して、ライン番号とプログラム番号を取得
    stringstream ss(qr_code);
    string line, prog;
    getline(ss, line, ';');
    getline(ss, prog, ';');

    // 指定された条件の場合のみダミーデータを返す
    if (line == "aaaaa" && prog == "aaaaa")
    {
        ToolingList dummy_list;
        dummy_list.line_name = line;
        dummy_list.program_number = prog;

        // 工具のダミーデータを追加
        dummy_list.tools.push_back({"T1", "ドリル", "OSG", "ADF-2D-9", "超硬", 7});
        dummy_list.tools.push_back({"T2", "インサート", "タンガロイ", "TPMT16T308-CM", "T515", 3});
        dummy_list.tools.push_back({"T3", "エンドミル", "YAMATO", "M-CEC2-EM10-20-R0.2", "超硬", 1});
        // (必要に応じてダミーデータを追加)
        
        return dummy_list;
    }

    // 条件に一致しない場合は、データが見つからなかったとしてnulloptを返す
    return nullopt;
    // ■■■■ 課題 (ダミー動作) ここまで修正 ■■■■
}