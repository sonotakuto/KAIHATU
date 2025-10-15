/**
 * @file db_client.cpp
 * @brief db_client.hppで宣言された関数の実装ファイル。
 */
#include "./odbc.hpp"
#include "./includes.hpp"
#include "./db_client.hpp"

using namespace odbc;

namespace db {

// データベースへの接続を管理するクラスをまとめた構造体へのポインタ
static std::unique_ptr<OdbcSession> db_session;

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
			string message = "ステートメント作成失敗 (試行 " + to_string(i + 1) + "): " + e.what();
			// cerr << "[LOG] " << message << endl;

			// 5秒待機してからリトライ
			this_thread::sleep_for(chrono::seconds(5));
		}
	}

	// 試行が全て失敗した場合
	return nullopt;
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
			string message = "接続失敗 (試行 " + to_string(i + 1) + "): " + e.what();
//			cerr << "[LOG] " << message << endl;

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

bool user_authenticate(int user_id) {
/*
	optional<OdbcStatement> stmt_opt = create_statement_with_retry();

	if (!stmt_opt) {
		// 試行が全て失敗した場合
		// TODO:エラーメッセージを表示するコードをここに書く？
	}
*/
	DEV_PRINT("[DB Stub] user_authenticate called with user_id = {}\n", user_id);

	g_logged_in_user_info = MstUserInfo{
		.user_id = user_id,
		.user_name = "山田太郎",
		.per_out = true,
		.per_add = true,
		.per_inventory = true
	};

	return true; // TODO: 一時的に常に成功させる
}

bool save_history(void) {
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

std::optional<MstUserTable> get_user_data(int user_id) {
/*
	optional<OdbcStatement> stmt_opt = create_statement_with_retry();

	if (!stmt_opt) {
		// 試行が全て失敗した場合
		// TODO:エラーメッセージを表示するコードをここに書く？
	}
*/
	DEV_PRINT("[DB Stub] user_authenticate called with user_id = {}\n", user_id);

	return MstUserTable{
		.user_id = user_id,
		.user_name = "山田太郎",
		.per_out = true,
		.per_add = true,
		.per_inventory = true
	};
}

std::optional<MstToolTable> get_mst_tool_data(int basic_tool_id) {
/*
	optional<OdbcStatement> stmt_opt = create_statement_with_retry();
	if (!stmt_opt) {
		// 試行が全て失敗した場合
		// TODO:エラーメッセージを表示するコードをここに書く？
	}
*/
	DEV_PRINT("[DB Stub] get_mst_tool_data called with basic_tool_id = {}\n", basic_tool_id);

	return MstToolTable{
		.basic_tool_id = basic_tool_id,
		.tool_name = "D-1234",
		.maker = "工具メーカー",
		.tool_category = "ドリル",
		.tool_material = "超硬",
		.stc = 10
	};
}

std::optional<UniqueToolTable> get_unique_tool_data(int unique_tool_id) {
/*
	optional<OdbcStatement> stmt_opt = create_statement_with_retry();

	if (!stmt_opt) {
		// 試行が全て失敗した場合
		// TODO:エラーメッセージを表示するコードをここに書く？
	}
*/
	DEV_PRINT("[DB Stub] get_unique_tool_data called with id = {}\n", unique_tool_id);

	return UniqueToolTable{
		.unique_tool_id = unique_tool_id,
		.basic_tool_id = unique_tool_id,
		.unique_num = unique_tool_id,
		.case_pack_num = 5,
		.storage_area_id = 7,
		.storage_condition = StorageCondition::USABLE,
		.regrind_count = 2
	};
}

std::optional<ToolingListInfo> get_tooling_list(int line_id) {
/*
	optional<OdbcStatement> stmt_opt = create_statement_with_retry();

	if (!stmt_opt) {
		// 試行が全て失敗した場合
		// TODO:エラーメッセージを表示するコードをここに書く？
	}
*/
	DEV_PRINT("[DB Stub] get_tooling_list called with line_id = {}\n", line_id);

	return ToolingListInfo{
		.line_id = line_id,
		.line_name = "ラインA",
		.prog_name = "プログラムX",
		.items = std::vector<ToolAssignmentInfo>{
			{
				.tool_number = "T-0001",
				.mst_tool = get_basic_tool_data(1).value()
			}, {
				.tool_number = "T-0002",
				.mst_tool = get_basic_tool_data(2).value()
			}, {
				.tool_number = "T-0003",
				.mst_tool = get_basic_tool_data(3).value()
			}, {
				.tool_number = "T-0004",
				.mst_tool = get_basic_tool_data(4).value()
			}, {
				.tool_number = "T-0005",
				.mst_tool = get_basic_tool_data(5).value()
			}, {
				.tool_number = "T-0006",
				.mst_tool = get_basic_tool_data(6).value()
			}, {
				.tool_number = "T-0007",
				.mst_tool = get_basic_tool_data(7).value()
			}, {
				.tool_number = "T-0008",
				.mst_tool = get_basic_tool_data(8).value()
			}, {
				.tool_number = "T-0009",
				.mst_tool = get_basic_tool_data(9).value()
			}, {
				.tool_number = "T-0010",
				.mst_tool = get_basic_tool_data(10).value()
			}, {
				.tool_number = "T-0011",
				.mst_tool = get_basic_tool_data(11).value()
			}, {
				.tool_number = "T-0012",
				.mst_tool = get_basic_tool_data(12).value()
			}, {
				.tool_number = "T-0013",
				.mst_tool = get_basic_tool_data(13).value()
			}, {
				.tool_number = "T-0014",
				.mst_tool = get_basic_tool_data(14).value()
			}, {
				.tool_number = "T-0015",
				.mst_tool = get_basic_tool_data(15).value()
			}, {
				.tool_number = "T-0016",
				.mst_tool = get_basic_tool_data(16).value()
			}, {
				.tool_number = "T-0017",
				.mst_tool = get_basic_tool_data(17).value()
			}, {
				.tool_number = "T-0018",
				.mst_tool = get_basic_tool_data(18).value()
			}, {
				.tool_number = "T-0019",
				.mst_tool = get_basic_tool_data(19).value()
			}, {
				.tool_number = "T-0020",
				.mst_tool = get_basic_tool_data(20).value()
			}, {
				.tool_number = "T-0021",
				.mst_tool = get_basic_tool_data(21).value()
			}, {
				.tool_number = "T-0022",
				.mst_tool = get_basic_tool_data(22).value()
			}, {
				.tool_number = "T-0023",
				.mst_tool = get_basic_tool_data(23).value()
			}, {
				.tool_number = "T-0024",
				.mst_tool = get_basic_tool_data(24).value()
			}, {
				.tool_number = "T-0025",
				.mst_tool = get_basic_tool_data(25).value()
			}, {
				.tool_number = "T-0026",
				.mst_tool = get_basic_tool_data(26).value()
			}, {
				.tool_number = "T-0027",
				.mst_tool = get_basic_tool_data(27).value()
			}, {
				.tool_number = "T-0028",
				.mst_tool = get_basic_tool_data(28).value()
			}, {
				.tool_number = "T-0029",
				.mst_tool = get_basic_tool_data(29).value()
			}, {
				.tool_number = "T-0030",
				.mst_tool = get_basic_tool_data(30).value()
			}
		}
	};
}

} // namespace db