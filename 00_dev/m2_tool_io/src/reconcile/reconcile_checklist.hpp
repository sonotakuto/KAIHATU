/**
 * @file reconcile_checklist.hpp
 * @brief 棚卸し（コンテナ内の期待工具リストと実読み取り結果の突合）ロジックを提供するヘッダファイル。
 * @details
 * 本クラスは GUI (SimpleGtk) にも DB (db::) にも一切依存しません。
 * 「DBから取得した期待工具ID一覧」と「QR読み取りによる実際のスキャンID」を
 * 突き合わせるだけの単一責任クラスであり、単体テストが容易な形に分離しています (SRP)。
 *
 * 呼び出し側 (tool_io_gui.cpp) の責務:
 *  - DBから期待工具一覧を取得し、本クラスを構築する
 *  - QRコード読み取りイベントごとに scan() を呼び、戻り値に応じてテーブルの色 (TableCellState) を更新する
 *  - is_fully_read() で「未読み取りが残っていないか」を確認し、画面遷移や警告表示を制御する
 */
#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace reconcile {

/**
 * @enum MatchStatus
 * @brief 棚卸し対象1件ごとの照合結果ステータス。
 * @details tool_io_gui 側の TableCellState::RECONCILE_MATCH / MISMATCH / UNREAD と1対1で対応させる想定です。
 */
enum class MatchStatus {
	UNREAD,		///< まだ読み取られていない (期待リストに存在するが未スキャン)
	MATCHED,	///< 期待通りの個別工具がスキャンされ、一致が確認できた
	MISMATCH	///< 想定外 (期待リストに無い個別工具がスキャンされた、あるいは既に不一致登録済み)
};

/**
 * @enum EntryOrigin
 * @brief チェックリストの1エントリがどのように登録されたかを示す区分。
 */
enum class EntryOrigin {
	EXPECTED,	///< DBの保管情報から「このコンテナにあるはず」として登録された
	EXTRA		///< スキャン時に想定外として動的に追加された (期待リストに無かった)
};

/**
 * @struct ChecklistEntry
 * @brief 棚卸しチェックリストの1行分のデータ。
 */
struct ChecklistEntry {
	std::int64_t	unique_tool_id{};				///< 個別工具ID
	MatchStatus		status{MatchStatus::UNREAD};	///< 現在の照合ステータス
	EntryOrigin		origin{EntryOrigin::EXPECTED};	///< 登録経緯
};

/**
 * @class Checklist
 * @brief 棚卸しの「期待工具リスト」と「実際のスキャン結果」の突合のみを担当するクラス。
 */
class Checklist {
public:
	/**
	 * @brief コンストラクタ。
	 * @param[in] expected_unique_tool_ids DBから取得した「このコンテナに保管されているはず」の個別工具IDの一覧。
	 */
	explicit Checklist(std::vector<std::int64_t> expected_unique_tool_ids);

	/**
	 * @brief QRコード等で読み取った個別工具IDを照合します。
	 * @details
	 * - 期待リストに存在し未読み取りの場合: MATCHED に更新して返す。
	 * - 期待リストに存在するが既に読み取り済み(重複スキャン)の場合: 現在のステータスをそのまま返す (冪等)。
	 * - 期待リストに存在しない場合: MISMATCH の新規エントリ (origin=EXTRA) として追加して返す。
	 * @param[in] unique_tool_id スキャンされた個別工具ID。
	 * @return 照合の結果ステータス。
	 */
	MatchStatus scan(std::int64_t unique_tool_id);

	/**
	 * @brief 期待リストに含まれる全項目の読み取りが完了しているか判定します。
	 * @details origin=EXTRA (想定外項目) は判定対象に含めません。
	 * @return 未読み取りの期待項目が1件も無ければ true。
	 */
	bool is_fully_read() const;

	/**
	 * @brief 現在のチェックリスト全体を取得します (テーブル描画・再構築用)。
	 */
	const std::vector<ChecklistEntry>& entries() const {
		return entries_;
	}

	/**
	 * @brief 不一致 (MISMATCH) 状態にある個別工具IDの一覧を取得します。
	 */
	std::vector<std::int64_t> mismatched_ids() const;

	/**
	 * @brief 未読み取り (UNREAD) のまま残っている個別工具IDの一覧を取得します。
	 */
	std::vector<std::int64_t> unread_ids() const;

	/**
	 * @brief 期待リストの総数 (origin=EXPECTED の件数) を取得します。
	 */
	std::size_t expected_count() const {
		return expected_count_;
	}

private:
	std::vector<ChecklistEntry> entries_;						///< 全エントリ (期待 + 想定外)
	std::unordered_map<std::int64_t, std::size_t> index_by_id_;	///< unique_tool_id -> entries_ の index
	std::size_t expected_count_{};								///< origin=EXPECTED の件数 (構築時に確定)
};

} // namespace reconcile
