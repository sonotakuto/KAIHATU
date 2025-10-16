/**
 * @file db_client.cpp
 * @brief db_client.hppで宣言された関数の実装ファイル。
 */
#include <iostream>
#include <sstream>
#include <iomanip>
#include "./odbc.hpp"
#include "./includes.hpp"
#include "./db_client.hpp"

using namespace odbc;

namespace db {

// データベースへの接続を管理するクラスをまとめた構造体へのポインタ
static std::unique_ptr<OdbcSession> db_session;

// --- このファイル内部でのみ使用するヘルパー関数 ---

/**
 * @brief OdbcStatementをリトライ処理付きで安全に作成するヘルパー関数。
 * @return OdbcStatementを保持するoptional。失敗した場合はnullopt。
 */
static optional<OdbcStatement> create_statement_with_retry() {
	// グローバルなDBセッションまたは接続が存在しない場合は即座に失敗させる
	if (!db_session || !db_session->connection) {
		// TODO: 接続がない旨のエラーメッセージを表示する
		return nullopt;
	}

	// リトライ処理
	for (int i = 0; i < RETRY_LIMIT; i++) {
		try {
			// ステートメントを作成し、optionalに格納して返す
			return optional<OdbcStatement>(in_place, *db_session->connection);
		} catch (const odbc::OdbcException& e) {
			// TODO: ログを残すか表示する
			DEV_PRINT(stderr, "[LOG] ステートメント作成失敗 (試行 {}): {}", i + 1, e.what());
			// cerr << "[LOG] " << message << endl;

			// 5秒待機してからリトライ
			this_thread::sleep_for(chrono::seconds(5));
		}
	}

	// 試行が全て失敗した場合
	return nullopt;
}

// --- 公開関数 ---

std::optional<QrCodeData> parse_qr(const std::string& qr_string) {
	if (qr_string.empty()) {
		return std::nullopt; // 空の文字列は無効
	}

	// ハイフンでID部とタイムスタンプ部を分割
	const size_t hyphen_pos = qr_string.find('-');
	if (hyphen_pos == std::string::npos) {
		return std::nullopt; // ハイフンがない形式は無効
	}

	// 各パーツを抽出
	const char type_char = qr_string[0];
	const std::string id_str = qr_string.substr(1, hyphen_pos - 1);
	const std::string time_str = qr_string.substr(hyphen_pos + 1);

	QrCodeData result;
	result.type = QrCodeType::Unknown;

	// 最初の文字でタイプを判別し、各IDの桁数と文字列長を検証
	switch (type_char) {
		case 'U':
			if (id_str.length() != 4 || qr_string.length() != 20) { return std::nullopt; }
			result.type = QrCodeType::User;
			break;
		case 'T':
			if (id_str.length() != 10 || qr_string.length() != 26) { return std::nullopt; }
			result.type = QrCodeType::Tool;
			break;
		case 'P':
			if (id_str.length() != 3 || qr_string.length() != 19) { return std::nullopt; }
			result.type = QrCodeType::Program;
			break;
		default:
			return std::nullopt; // 'U', 'T', 'P' 以外は無効
	}

	// ID文字列をlong longに変換
	try {
		result.id = std::stoll(id_str);
	} catch (const std::invalid_argument& e) {
		return std::nullopt; // 数値に変換できないIDは無効
	} catch (const std::out_of_range& e) {
		return std::nullopt; // IDがlong longの範囲外
	}

	// タイムスタンプ文字列(YYYYMMDDHHMMSS)をtime_pointに変換
	std::tm tm = {};
	std::stringstream ss(time_str);
	ss >> std::get_time(&tm, "%Y%m%d%H%M%S");
	if (ss.fail()) {
		return std::nullopt; // タイムスタンプのフォーマットが不正
	}
	result.timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));

	return result;
}

bool connect(void) {
/*
	// リトライ処理
	for (int i = 0; i < DB_RETRY_LIMIT; i++) {
		try {
			// DBに接続
			auto conn = make_unique<OdbcConnection>(ODBC_DSN, ODBC_USER, ODBC_PASS);
			db_session = make_unique<OdbcSession>(move(conn));

			// 成功したらループを抜ける
			return true;
		} catch (const odbc::OdbcException& e) {
			// 接続に失敗した場合
			// TODO:ログを残すか表示する？
			DEV_PRINT(stderr, "[LOG] 接続失敗 (試行 {}): {}", i + 1, e.what());

			// 5秒待機
			this_thread::sleep_for(chrono::seconds(5));
		}
	}

	// 試行が全て失敗した場合
	// TODO:エラーメッセージを表示するコードをここに書く？

	return false;
*/
	return true; // TODO: 一時的に常に成功させる
}

void disconnect(void) {
//	db_session.reset();
}

bool user_authenticate(const std::string& qr_string) {
	g_logged_in_user_info = get_MstUserInfo(qr_string);
	return g_logged_in_user_info.has_value();
}

bool save_history(const RecOperationTable& rec) {
/*
	optional<OdbcStatement> stmt_opt = create_statement_with_retry();

	if (!stmt_opt) {
		// 試行が全て失敗した場合
		// TODO:エラーメッセージを表示するコードをここに書く？
	}
*/
	DEV_PRINT("[DB Stub] save_history called.\n");
	return true; // TODO: 一時的に常に成功させる
}

std::optional<MstUserInfo> get_MstUserInfo(const std::string& qr_string) {
	auto code_opt = parse_qr(qr_string);
	auto stmt_opt = create_statement_with_retry();
	if (!stmt_opt || !code_opt) { return std::nullopt; }

	auto& code = code_opt.value();
	auto& stmt = stmt_opt.value();

	try {
		const std::string sql = "SELECT * FROM mst_user WHERE user_id = ? AND user_print_time = ?;";
		stmt.prepare(sql);
		stmt.bind(1, code.id);
		stmt.bind(2, code.timestamp);
		stmt.executePrepared();

		auto rows = stmt.fetchAll();
		if (rows.empty()) {
			return std::nullopt; // ユーザーが見つからない
		}

		const auto& row = rows[0];
		MstUserInfo user_info;
		user_info.user_id		= std::any_cast<int>		(row.at("user_id"));
		user_info.user_name		= std::any_cast<std::string>(row.at("user_name"));
		// TINYINT(1) は int として取得し、boolに変換
		user_info.per_add		= (std::any_cast<int>		(row.at("per_add"))			!= 0);
		user_info.per_inventory	= (std::any_cast<int>		(row.at("per_inventory"))	!= 0);
		// per_outはtypedef.hppでデフォルトtrueのため、DBスキーマに合わせて他の権限も追加

		return user_info;

	} catch (const odbc::OdbcException& e) {
		DEV_PRINT(stderr, "DB Error in get_MstUserInfo: {}\n", e.what());
		return std::nullopt;
	} catch (const std::bad_any_cast& e) {
		DEV_PRINT(stderr, "DB Cast Error in get_MstUserInfo: {}\n", e.what());
		return std::nullopt;
	}
}

std::optional<UniqueToolInfo> get_UniqueToolInfo(const std::string& qr_string) {
	auto stmt_opt = create_statement_with_retry();
	if (!stmt_opt) { return std::nullopt; }

	auto& stmt = stmt_opt.value();

	try {
		const std::string sql =
			"SELECT "
			"    ut.unique_tool_id, ut.unique_num, ut.case_pack_num, ut.storage_condition, ut.regrind_count, "
			"    mt.basic_tool_id, mt.tool_name, mt.maker, mt.tool_category, mt.tool_material, mt.stc, "
			"    sa.storage_area_id, sa.toolcase_stc_num, "
			"    ma.display_address, ma.control_address "
			"FROM unique_tool AS ut "
			"LEFT JOIN mst_tool AS mt ON ut.basic_tool_id = mt.basic_tool_id "
			"LEFT JOIN storage_area AS sa ON ut.storage_area_id = sa.storage_area_id "
			"LEFT JOIN mst_address AS ma ON sa.display_address = ma.display_address "
			"WHERE ut.unique_tool_id = ?;";

		stmt.prepare(sql);
		stmt.bind(1, unique_tool_id);
		stmt.executePrepared();

		auto rows = stmt.fetchAll();
		if (rows.empty()) {
			return std::nullopt;
		}

		const auto& row = rows[0];
		UniqueToolInfo tool_info;

		// unique_tool テーブルの情報を詰める
		tool_info.unique_tool_id	= std::any_cast<int>(row.at("unique_tool_id"));
		tool_info.unique_num		= std::any_cast<int>(row.at("unique_num"));
		tool_info.case_pack_num		= std::any_cast<int>(row.at("case_pack_num"));

		// enumは文字列として取得
		std::string condition_str	= std::any_cast<std::string>(row.at("storage_condition"));
		if		(condition_str == "保管中")	{tool_info.storage_condition = StorageCondition::USABLE;}
		else if	(condition_str == "取出")	{tool_info.storage_condition = StorageCondition::TAKEN_OUT;}
		else								{tool_info.storage_condition = StorageCondition::NOT_STORED;}

		if (row.count("regrind_count") && row.at("regrind_count").has_value()) {
			tool_info.regrind_count = std::any_cast<int>(row.at("regrind_count"));
		}

		// mst_tool テーブルの情報を詰める
		tool_info.mst_tool.basic_tool_id	= std::any_cast<int>		(row.at("basic_tool_id"));
		tool_info.mst_tool.tool_name		= std::any_cast<std::string>(row.at("tool_name"));
		tool_info.mst_tool.maker			= std::any_cast<std::string>(row.at("maker"));
		tool_info.mst_tool.tool_category	= std::any_cast<std::string>(row.at("tool_category"));
		tool_info.mst_tool.tool_material	= std::any_cast<std::string>(row.at("tool_material"));
		tool_info.mst_tool.stc				= std::any_cast<int>		(row.at("stc"));

		// storage_area と mst_address の情報を詰める (LEFT JOINなのでNULLの可能性がある)
		if (row.count("storage_area_id") && row.at("storage_area_id").has_value()) {
			StrageAreaInfo area_info;
			area_info.storage_area_id				= std::any_cast<int>		(row.at("storage_area_id"));
			area_info.toolcase_stc_num				= std::any_cast<int>		(row.at("toolcase_stc_num"));
			area_info.mst_address.display_address	= std::any_cast<std::string>(row.at("display_address"));
			area_info.mst_address.control_address	= std::any_cast<int>		(row.at("control_address"));
			area_info.item = tool_info.mst_tool;	// StrageAreaInfo内のitemは、この工具の基本情報と同じ
			tool_info.storage_area = area_info;
		}

		return tool_info;

	} catch (const odbc::OdbcException& e) {
		DEV_PRINT(stderr, "DB Error in get_UniqueToolInfo: {}\n", e.what());
		return std::nullopt;
	} catch (const std::bad_any_cast& e) {
		DEV_PRINT(stderr, "DB Cast Error in get_UniqueToolInfo: {}\n", e.what());
		return std::nullopt;
	}
}

std::optional<ToolingListInfo> get_ToolingListInfo(const std::string& qr_string) {
	auto stmt_opt = create_statement_with_retry();
	if (!stmt_opt) { return std::nullopt; }

	auto& stmt = stmt_opt.value();

	try {
		const std::string sql =
			"SELECT "
			"    pal.program_assignment_id, pal.line_name, pal.program_num, "
			"    ta.tool_assignment_id, ta.tool_num, "
			"    mt.basic_tool_id, mt.tool_name, mt.maker, mt.tool_category, mt.tool_material, mt.stc "
			"FROM program_assignment_list AS pal "
			"JOIN tool_assignment AS ta ON pal.program_assignment_id = ta.program_assignment_id "
			"JOIN mst_tool AS mt ON ta.basic_tool_id = mt.basic_tool_id "
			"WHERE pal.program_assignment_id = ? "
			"ORDER BY ta.tool_assignment_id ASC;";

		stmt.prepare(sql);
		stmt.bind(1, program_assignment_id);
		stmt.executePrepared();

		auto rows = stmt.fetchAll();
		if (rows.empty()) {
			return std::nullopt; // リストが見つからない
		}

		// 最初の行からリスト全体の情報を作成
		ToolingListInfo list_info;
		list_info.program_assignment_id	= std::any_cast<int>		(rows[0].at("program_assignment_id"));
		list_info.line_name				= std::any_cast<std::string>(rows[0].at("line_name"));
		list_info.program_num			= std::any_cast<std::string>(rows[0].at("program_num"));

		// 全ての行をループして、工具のリストを作成
		for (const auto& row : rows) {
			ToolAssignmentInfo item;
			item.tool_assignment_id	= std::any_cast<int>		(row.at("tool_assignment_id"));
			item.tool_number		= std::any_cast<std::string>(row.at("tool_num"));

			item.mst_tool.basic_tool_id	= std::any_cast<int>		(row.at("basic_tool_id"));
			item.mst_tool.tool_name		= std::any_cast<std::string>(row.at("tool_name"));
			item.mst_tool.maker			= std::any_cast<std::string>(row.at("maker"));
			item.mst_tool.tool_category	= std::any_cast<std::string>(row.at("tool_category"));
			item.mst_tool.tool_material	= std::any_cast<std::string>(row.at("tool_material"));
			item.mst_tool.stc			= std::any_cast<int>		(row.at("stc"));

			list_info.tool_assignments.push_back(item);
		}

		return list_info;

	} catch (const odbc::OdbcException& e) {
		DEV_PRINT(stderr, "DB Error in get_ToolingListInfo: {}\n", e.what());
		return std::nullopt;
	} catch (const std::bad_any_cast& e) {
		DEV_PRINT(stderr, "DB Cast Error in get_ToolingListInfo: {}\n", e.what());
		return std::nullopt;
	}
}

} // namespace db