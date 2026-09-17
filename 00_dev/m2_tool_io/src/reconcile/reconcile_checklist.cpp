/**
 * @file reconcile_checklist.cpp
 * @brief 棚卸し照合ロジック (reconcile::Checklist) の実装。
 */
#include "reconcile_checklist.hpp"

namespace reconcile {

Checklist::Checklist(std::vector<std::int64_t> expected_unique_tool_ids) {
	entries_.reserve(expected_unique_tool_ids.size());
	index_by_id_.reserve(expected_unique_tool_ids.size());

	for (const std::int64_t id : expected_unique_tool_ids) {
		// 万一、期待リスト内にIDの重複があっても1エントリのみ登録する (防御的処理)
		if (index_by_id_.find(id) != index_by_id_.end()) {
			continue;
		}

		index_by_id_.emplace(id, entries_.size());
		entries_.push_back(ChecklistEntry{
			.unique_tool_id = id,
			.status = MatchStatus::UNREAD,
			.origin = EntryOrigin::EXPECTED
		});
	}

	expected_count_ = entries_.size();
}

MatchStatus Checklist::scan(std::int64_t unique_tool_id) {
	auto it = index_by_id_.find(unique_tool_id);

	if (it != index_by_id_.end()) {
		// 既知のID (期待リスト内、または過去にスキャン済みの想定外項目)
		ChecklistEntry& entry = entries_[it->second];

		if (entry.status == MatchStatus::UNREAD) {
			// 期待リストの未読み取り項目が、初めて読み取られた
			entry.status = MatchStatus::MATCHED;
		}
		// MATCHED や MISMATCH の項目を再スキャンした場合は、ステータスを変更せず冪等に返す (重複スキャン対策)
		return entry.status;
	}

	// 期待リストに存在しない、想定外の個別工具ID
	index_by_id_.emplace(unique_tool_id, entries_.size());
	entries_.push_back(ChecklistEntry{
		.unique_tool_id = unique_tool_id,
		.status = MatchStatus::MISMATCH,
		.origin = EntryOrigin::EXTRA
	});
	return MatchStatus::MISMATCH;
}

bool Checklist::is_fully_read() const {
	for (const ChecklistEntry& entry : entries_) {
		if (entry.origin == EntryOrigin::EXPECTED && entry.status == MatchStatus::UNREAD) {
			return false;
		}
	}
	return true;
}

std::vector<std::int64_t> Checklist::mismatched_ids() const {
	std::vector<std::int64_t> result;
	for (const ChecklistEntry& entry : entries_) {
		if (entry.status == MatchStatus::MISMATCH) {
			result.push_back(entry.unique_tool_id);
		}
	}
	return result;
}

std::vector<std::int64_t> Checklist::unread_ids() const {
	std::vector<std::int64_t> result;
	for (const ChecklistEntry& entry : entries_) {
		if (entry.origin == EntryOrigin::EXPECTED && entry.status == MatchStatus::UNREAD) {
			result.push_back(entry.unique_tool_id);
		}
	}
	return result;
}

} // namespace reconcile
