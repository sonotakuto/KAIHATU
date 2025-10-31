/**
 * @file db_client.cpp
 * @brief db_client.hppで宣言された関数の実装ファイル。
 */
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "./any_extension.hpp"
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
		DEV_PRINT("ステートメント生成：接続がありません\n");
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

/**
 * @brief DBから取得した行データ (mapのvector) を ToolingListInfo 構造体に変換します。
 * @param[in] rows OdbcStatement::fetchAll() で取得したDBの行データ。
 * @return 変換に成功した場合は ToolingListInfo を含む optional、失敗した場合は nullopt。
 */
static std::optional<ToolingListInfo> populate_tooling_list_from_rows(
	const std::vector<std::map<std::string, std::any>>& rows)
{
	if (rows.empty()) {
		return std::nullopt; // データがない
	}

	try {
		// 最初の行からリスト全体の情報を作成
		const auto& first_row = rows[0];
		ToolingListInfo list_info{
			.line = MstLineInfo{
				.id		= std::any_cast<std::int32_t>	(first_row.at("line_id")),
				.name	= std::any_cast<std::string>	(first_row.at("line_name"))
			},
			.program = MstProgramInfo{
				.id		= std::any_cast<std::int32_t>	(first_row.at("program_id")),
				.name	= std::any_cast<std::string>	(first_row.at("program_name"))
			}
		};

		// 全ての行をループして、工具のリストを作成
		list_info.tool_assignments.reserve(rows.size());
		for (const auto& row : rows) {
			// tool_assignment_id が NULL でない場合のみ (LEFT JOIN 対策)
			if (row.count("tool_assignment_id") && row.at("tool_assignment_id").has_value()) {
				list_info.tool_assignments.push_back(ToolAssignmentInfo{
					.id			= std::any_cast<std::int32_t>(row.at("tool_assignment_id")),
					.tool_num	= std::any_cast<std::string>(row.at("tool_num")),
					.mst_tool_info = MstToolInfo{
						.id			= std::any_cast<std::int32_t>	(row.at("basic_tool_id")),
						.name		= std::any_cast<std::string>	(row.at("tool_name")),
						.maker		= std::any_cast<std::string>	(row.at("maker")),
						.category	= std::any_cast<std::string>	(row.at("tool_category")),
						.material	= std::any_cast<std::string>	(row.at("tool_material")),
						.stc		= std::any_cast<std::int32_t>	(row.at("stc"))
					}
				});
			}
		}
		return list_info;

	} catch (const std::bad_any_cast& e) {
		DEV_PRINT(stderr, "DB Cast Error in populate_tooling_list_from_rows: {}\n", e.what());
		return std::nullopt;
	}
}

/**
 * @brief program_id と (オプションの) print_time でDBを検索し、ToolingListInfo を構築する共通ヘルパー
 * @param[in] program_id 検索キーとなるプログラムID
 * @param[in] print_time 認証に使うQRコードの印刷時刻。nulloptの場合は program_id のみで検索
 * @return 成功した場合は ToolingListInfo、失敗した場合は nullopt
 */
static std::optional<ToolingListInfo> query_and_populate_tooling_list(
	std::int64_t program_id,
	std::optional<std::chrono::system_clock::time_point> print_time)
{
	auto stmt_opt = create_statement_with_retry();
	if (!stmt_opt) {
		return std::nullopt;
	}
	auto& stmt = stmt_opt.value();

	try {
		// SQLの基本部分
		std::string sql =
			"SELECT "
			"    pal.program_id, pal.program_name, "
			"    ml.line_id, ml.line_name, "
			"    ta.tool_assignment_id, ta.tool_num, "
			"    mt.basic_tool_id, mt.tool_name, mt.maker, mt.tool_category, mt.tool_material, mt.stc "
			"FROM mst_program AS pal "
			"LEFT JOIN mst_line AS ml ON pal.line_id = ml.line_id "
			"LEFT JOIN mst_tool_assignment AS ta ON pal.program_id = ta.program_id "
			"LEFT JOIN mst_tool AS mt ON ta.basic_tool_id = mt.basic_tool_id "
			"WHERE pal.program_id = ? "; // program_id は必須

		// print_time があれば WHERE 句に追加
		if (print_time.has_value()) {
			sql += "AND pal.program_print_time = ? ";
		}
		sql += "ORDER BY ta.tool_assignment_id ASC;";

		stmt.prepare(sql);
		stmt.bind(1, program_id);
		// print_time があればバインド
		if (print_time.has_value()) {
			stmt.bind(2, print_time.value());
		}
		stmt.executePrepared();

		auto rows = stmt.fetchAll();
		return populate_tooling_list_from_rows(rows); // 簡略化されたヘルパーを呼ぶ

	} catch (const odbc::OdbcException& e) {
		DEV_PRINT(stderr, "DB Error in query_and_populate_tooling_list: {}\n", e.what());
		return std::nullopt;
	} catch (const std::bad_any_cast& e) {
		DEV_PRINT(stderr, "DB Cast Error in query_and_populate_tooling_list: {}\n", e.what());
		return std::nullopt;
	}
}

/**
 * @brief get_StrageAreaInfo_with_min_cases と get_StrageAreaInfo_with_max_cases の共通実装部分。
 * @param[in] basic_tool_id 検索対象の基本工具ID。
 * @param[in] order ソート順 ("ASC" または "DESC")。
 * @return 条件に合致する保管領域情報が見つかった場合はその構造体、見つからなければ std::nullopt。
 */
static std::optional<StrageAreaInfo> get_StrageAreaInfo_ordered_by_cases(std::int32_t basic_tool_id, const std::string& order) {
	auto stmt_opt = create_statement_with_retry();
	if (!stmt_opt) {
		DEV_PRINT(stderr, "Error: Failed to create statement in get_StrageAreaInfo_ordered_by_cases after multiple retries.\n");
		return std::nullopt;
	}
	auto& stmt = stmt_opt.value();

	try {
		// storage_area テーブルを mst_address, mst_tool と結合し、
		// 指定された basic_tool_id で絞り込み、toolcase_stc_num でソートして最初の1件を取得するSQL
		const std::string sql = fmt::format(
			"SELECT "
			"    sa.storage_area_id, sa.toolcase_stc_num, "
			"    ma.display_address, ma.control_address, "
			"    mt.basic_tool_id, mt.tool_name, mt.maker, mt.tool_category, mt.tool_material, mt.stc "
			"FROM storage_area AS sa "
			"JOIN mst_address AS ma ON sa.display_address = ma.display_address "
			"JOIN mst_tool AS mt ON sa.basic_tool_id = mt.basic_tool_id "
			"WHERE sa.basic_tool_id = ? "
			"ORDER BY sa.toolcase_stc_num {} " // ソート順をプレースホルダで指定
			"LIMIT 1;",
			order // order ("ASC" or "DESC") を直接埋め込む (SQLインジェクションのリスクはないため安全)
		);

		stmt.prepare(sql);
		stmt.bind(1, basic_tool_id);
		stmt.executePrepared();

		auto rows = stmt.fetchAll();
		if (rows.empty()) {
			return std::nullopt; // 条件に合う領域が見つからない
		}

		const auto& row = rows[0];

		// 取得したデータを StrageAreaInfo 構造体に詰める
		return StrageAreaInfo{
			.id	= std::any_cast<std::int32_t>(row.at("storage_area_id")),

			.item	= MstToolInfo{
				.id			= std::any_cast<std::int32_t>	(row.at("basic_tool_id")),
				.name		= std::any_cast<std::string>	(row.at("tool_name")),
				.maker		= std::any_cast<std::string>	(row.at("maker")),
				.category	= std::any_cast<std::string>	(row.at("tool_category")),
				.material	= std::any_cast<std::string>	(row.at("tool_material")),
				.stc		= std::any_cast<std::int32_t>	(row.at("stc"))
			},

			.toolcase_stc_num	= std::any_cast<std::int32_t>(row.at("toolcase_stc_num")),

			.mst_address_info	= MstAddressInfo{
				.display_address	= std::any_cast<std::string>(row.at("display_address")),
				.control_address	= std::any_cast<std::string>(row.at("control_address"))
			}
		};

	} catch (const odbc::OdbcException& e) {
		DEV_PRINT(stderr, "DB Error in get_StrageAreaInfo_ordered_by_cases: {}\n", e.what());
		return std::nullopt;
	} catch (const std::bad_any_cast& e) {
		DEV_PRINT(stderr, "DB Cast Error in get_StrageAreaInfo_ordered_by_cases: {}\n", e.what());
		return std::nullopt;
	} catch (const fmt::format_error& e) {
		DEV_PRINT(stderr, "Formatting Error in get_StrageAreaInfo_ordered_by_cases: {}\n", e.what());
		return std::nullopt;
	}
}

// --- 公開関数 ---

std::optional<QrCodeData> parse_qr(const std::string& qr_string) {
	if (qr_string.empty()) {
		DEV_PRINT("QR解析失敗\n");
		return std::nullopt; // 空の文字列は無効
	}

	// ハイフンでID部とタイムスタンプ部を分割
	const size_t hyphen_pos = qr_string.find('-');
	if (hyphen_pos == std::string::npos) {
		DEV_PRINT("QR解析失敗\n");
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
		if (id_str.length() != 4 || qr_string.length() != 20) {
			DEV_PRINT("QR解析失敗\n");
			return std::nullopt;
		}
		result.type = QrCodeType::User;
		break;
	case 'T':
		if (id_str.length() != 10 || qr_string.length() != 26) {
			DEV_PRINT("QR解析失敗\n");
			return std::nullopt;
		}
		result.type = QrCodeType::Tool;
		break;
	case 'P':
		if (id_str.length() != 3 || qr_string.length() != 19) {
			DEV_PRINT("QR解析失敗\n");
			return std::nullopt;
		}
		result.type = QrCodeType::Program;
		break;
	default:
		DEV_PRINT("QR解析失敗\n");
		return std::nullopt; // 'U', 'T', 'P' 以外は無効
	}

	// ID文字列を数値に変換
	try {
		if constexpr (sizeof(signed long) == 8){
			result.id = std::stol(id_str);
		} else {
			result.id = std::stoll(id_str);
		}
	} catch (const std::invalid_argument& e) {
		DEV_PRINT("QR解析失敗\n");
		return std::nullopt; // 数値に変換できないIDは無効
	} catch (const std::out_of_range& e) {
		DEV_PRINT("QR解析失敗\n");
		return std::nullopt; // IDが数値型の範囲外
	}

	// タイムスタンプ文字列(YYYYMMDDHHMMSS)をtime_pointに変換
	std::tm tm = {};
	std::stringstream ss(time_str);
	ss >> std::get_time(&tm, "%Y%m%d%H%M%S");
	if (ss.fail()) {
		DEV_PRINT("QR解析失敗\n");
		return std::nullopt; // タイムスタンプのフォーマットが不正
	}
	result.timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));

	DEV_PRINT("QR解析結果 種別：{}, ID：{}, 日時：{:%Y-%m-%d %H:%M:%S}\n", ((result.type == QrCodeType::User) ? ("User") : ((result.type == QrCodeType::User) ? ("Tool") : ("Program"))), result.id, result.timestamp);

	return result;
}

bool connect(void) {
	// リトライ処理
	for (int i = 0; i < RETRY_LIMIT; i++) {
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
}

void disconnect(void) {
//	db_session.reset();
}

bool user_authenticate(const std::string& qr_string) {
	g_logged_in_user_info = get_MstUserInfo(qr_string);
	return g_logged_in_user_info.has_value();
}

bool save_history(const RecOperationTable& record) {
	// 1. リトライ処理付きでステートメントオブジェクトを取得します。
	optional<OdbcStatement> stmt_opt = create_statement_with_retry();
	if (!stmt_opt.has_value()) {
		DEV_PRINT(stderr, "Error: Failed to create statement for saving history after multiple retries.\n");
		// TODO: ユーザーにエラーを通知する処理
		return false;
	}
	auto& stmt = stmt_opt.value();

	try {
		// 2. 実行するSQL文を準備します。
		// operation_id は自動インクリメントなので含めません。
		const std::string sql =
			"INSERT INTO rec_operation "
			"(operation_class, user_id, proc_time, storage_area_id, unique_tool_id, video_path, tool_num) "
			"VALUES (?, ?, ?, ?, ?, ?, ?);";

		stmt.prepare(sql);

		// 3. OperationClass enumをデータベースの文字列に変換します。
		std::string op_class_str;
		switch (record.operation_class) {
			case OperationClass::OUT:		op_class_str = "取出"; break;
			case OperationClass::ADD:		op_class_str = "補充"; break;
			case OperationClass::INVENTORY:	op_class_str = "棚卸"; break;
			default:
				DEV_PRINT(stderr, "Error: Unknown operation class provided to save_history.\n");
				return false;
		}

		// 4. 各プレースホルダにRecOperationTable構造体の値をバインドします。
		stmt.bind(1, op_class_str);
		stmt.bind(2, record.user_id);
		stmt.bind(3, record.proc_time);
		stmt.bind(4, record.storage_area_id);
		stmt.bind(5, record.unique_tool_id);
		stmt.bind(6, record.video_path);
		stmt.bind(7, record.tool_num);

		// 5. 準備されたステートメントを実行します。
		stmt.executePrepared();

		DEV_PRINT("[DB] save_history successful.\n");
		return true;

	} catch (const odbc::OdbcException& e) {
		// データベース処理中にエラーが発生した場合
		DEV_PRINT(stderr, "DB Error in save_history: {}\n", e.what());
		// TODO: ユーザーにエラーを通知する処理
		return false;
	}
}

bool save_history(const std::vector<RecOperationTable>& records) {
	if (records.empty()) {
		return true; // 保存対象がなければ成功とみなす
	}

	// 1. 接続セッションを確認
	if (!db_session || !db_session->connection) {
		DEV_PRINT(stderr, "Error: DB connection not available for saving histories.\n");
		return false;
	}

	// 2. トランザクションを開始
	db_session->connection->beginTransaction();

	try {
		// 3. ステートメントを準備
		optional<OdbcStatement> stmt_opt = create_statement_with_retry();
		if (!stmt_opt.has_value()) {
			DEV_PRINT(stderr, "Error: Failed to create statement for saving history after multiple retries.\n");
			// TODO: ユーザーにエラーを通知する処理
			return false;
		}
		auto& stmt = stmt_opt.value();

		const std::string sql =
			"INSERT INTO rec_operation "
			"(operation_class, user_id, proc_time, storage_area_id, unique_tool_id, video_path, tool_num) "
			"VALUES (?, ?, ?, ?, ?, ?, ?);";
		stmt.prepare(sql);

		// 4. 一括挿入する行数を設定
		stmt.setParamsetSize(records.size());

		// 5. `records` (vector of struct) から各列の `vector` (struct of vector) にデータを変換
		std::vector<std::string> op_class_strs;
		std::vector<std::int32_t> user_ids;
		std::vector<std::chrono::system_clock::time_point> proc_times;
		std::vector<std::int32_t> storage_area_ids;
		std::vector<std::optional<std::int64_t>> unique_tool_ids;
		std::vector<std::optional<std::string>> video_paths;
		std::vector<std::optional<std::int32_t>> tool_nums;

		op_class_strs.reserve(records.size());
		user_ids.reserve(records.size());
		proc_times.reserve(records.size());
		storage_area_ids.reserve(records.size());
		unique_tool_ids.reserve(records.size());
		video_paths.reserve(records.size());
		tool_nums.reserve(records.size());

		for (const auto& rec : records) {
			switch (rec.operation_class) {
				case OperationClass::OUT:		op_class_strs.push_back("取出"); break;
				case OperationClass::ADD:		op_class_strs.push_back("補充"); break;
				case OperationClass::INVENTORY:	op_class_strs.push_back("棚卸"); break;
				default: throw std::runtime_error("Unknown operation class");
			}
			user_ids.push_back(rec.user_id);
			proc_times.push_back(rec.proc_time);
			storage_area_ids.push_back(rec.storage_area_id);
			unique_tool_ids.push_back(rec.unique_tool_id);
			video_paths.push_back(rec.video_path);
			tool_nums.push_back(rec.tool_num);
		}

		// 6. 各列のデータを一括でバインド
		stmt.bindColumn(1, op_class_strs);
		stmt.bindColumn(2, user_ids);
		stmt.bindColumn(3, proc_times);
		stmt.bindColumn(4, storage_area_ids);
		stmt.bindColumn(5, unique_tool_ids);
		stmt.bindColumn(6, video_paths);
		stmt.bindColumn(7, tool_nums);

		// 7. SQLを一括実行
		stmt.executePrepared();

		// 8. トランザクションをコミット
		db_session->connection->commit();

		DEV_PRINT("[DB] save_histories successful for {} records.\n", records.size());
		return true;

	} catch (const std::exception& e) {
		// エラーが発生した場合はロールバック
		DEV_PRINT(stderr, "Error in save_histories, rolling back transaction: {}\n", e.what());
		db_session->connection->rollback();
		return false;
	}
}

std::optional<MstUserInfo> get_MstUserInfo(const std::string& qr_string) {
	auto code_opt = parse_qr(qr_string);
	auto stmt_opt = create_statement_with_retry();
	if (!stmt_opt.has_value() || !code_opt.has_value()) {
		DEV_PRINT("ユーザ情報取得：必要オブジェクト生成失敗\n");
		return std::nullopt;
	}

	auto& code = code_opt.value();
	auto& stmt = stmt_opt.value();

	// QRコードのタイプが間違っていればば失敗
	if (code.type != QrCodeType::User) {
		DEV_PRINT(stderr, "QR Error in get_MstUserInfo: Not a User QR code.\n");
		return std::nullopt;
	}

	try {
		const std::string sql = "SELECT * FROM mst_user WHERE user_id = ? AND user_print_time = ?;";
		stmt.prepare(sql);
		stmt.bind(1, code.id);
		stmt.bind(2, code.timestamp);
		stmt.executePrepared();

		auto rows = stmt.fetchAll();
		if (rows.empty()) {
			DEV_PRINT("ユーザ情報取得：不明なユーザー\n");
			return std::nullopt; // ユーザーが見つからない
		}

		const auto& row = rows[0];
		MstUserInfo user_info;
		user_info.id			= std::any_cast<std::int32_t>	(row.at("user_id"));
		user_info.name			= std::any_cast<std::string>	(row.at("user_name"));
		// TINYINT(1) は int8_t として取得し、boolに変換
		user_info.per_add		= (std::any_cast<std::int8_t>	(row.at("per_add"))			!= 0);
		user_info.per_inventory	= (std::any_cast<std::int8_t>	(row.at("per_inventory"))	!= 0);
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
	auto code_opt = parse_qr(qr_string);
	auto stmt_opt = create_statement_with_retry();
	if (!stmt_opt.has_value() || !code_opt.has_value()) {
		return std::nullopt;
	}

	auto& code = code_opt.value();
	auto& stmt = stmt_opt.value();

	// QRコードのタイプが間違っていればば失敗
	if (code.type != QrCodeType::Tool) {
		DEV_PRINT(stderr, "QR Error in get_UniqueToolInfo: Not a Tool QR code.\n");
		return std::nullopt;
	}

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
			"WHERE ut.unique_tool_id = ? AND ut.tool_print_time = ?;";

		stmt.prepare(sql);
		stmt.bind(1, code.id);
		stmt.bind(2, code.timestamp);
		stmt.executePrepared();

		auto rows = stmt.fetchAll();
		if (rows.empty()) {
			return std::nullopt;
		}

		const auto& row = rows[0];

		// enum を文字列として取得しておく
		std::string condition_str	= std::any_cast<std::string>(row.at("storage_condition"));

		// mst_tool テーブルの情報を詰めて用意しておく
		MstToolInfo mst_tool_info{
			.id			= std::any_cast<std::int32_t>	(row.at("basic_tool_id")),
			.name		= std::any_cast<std::string>	(row.at("tool_name")),
			.maker		= std::any_cast<std::string>	(row.at("maker")),
			.category	= std::any_cast<std::string>	(row.at("tool_category")),
			.material	= std::any_cast<std::string>	(row.at("tool_material")),
			.stc		= std::any_cast<std::int32_t>	(row.at("stc"))
		};

		// unique_tool テーブルの情報を詰めて返す
		return UniqueToolInfo{

			.id = std::any_cast<std::int64_t>(row.at("unique_tool_id")),

			// 用意しておいた mst_tool テーブルの情報を詰める
			.mst_tool_info = mst_tool_info,

			.unique_num		= std::any_cast<std::int32_t>(row.at("unique_num")),
			.case_pack_num	= std::any_cast<std::int32_t>(row.at("case_pack_num")),

			// storage_area と mst_address の情報を詰める (LEFT JOINなのでNULLの可能性がある)
			.storage_area_info = (
				(row.count("storage_area_id") && row.at("storage_area_id").has_value()) ? (
					std::make_optional<StrageAreaInfo>(StrageAreaInfo{
						.id					= std::any_cast<std::int32_t>(row.at("storage_area_id")),
						.item				= mst_tool_info,	// StrageAreaInfo内のitemは、この工具の基本情報と同じ
						.toolcase_stc_num	= std::any_cast<std::int32_t>(row.at("toolcase_stc_num")),

						.mst_address_info = MstAddressInfo{
							.display_address	= std::any_cast<std::string>(row.at("display_address")),
							.control_address	= std::any_cast<std::string>(row.at("control_address"))
						}
					})
				) : (
					std::nullopt
				)
			),

			.storage_condition = (
				(condition_str == "保管中") ? (
					StorageCondition::USABLE
				) : (
					(condition_str == "取出") ? (
						StorageCondition::TAKEN_OUT
					) : (
						StorageCondition::NOT_STORED
					)
				)
			),

			.regrind_count = stdex::any_cast_optional<std::int32_t>(row.at("regrind_count")),
		};

	} catch (const odbc::OdbcException& e) {
		DEV_PRINT(stderr, "DB Error in get_UniqueToolInfo: {}\n", e.what());
		return std::nullopt;
	} catch (const std::bad_any_cast& e) {
		DEV_PRINT(stderr, "DB Cast Error in get_UniqueToolInfo: {}\n", e.what());
		return std::nullopt;
	}
}

std::optional<ToolingListInfo> get_ToolingListInfo(const std::string& qr_string) {
	auto code_opt = parse_qr(qr_string);
	if (!code_opt.has_value()) {
		return std::nullopt;
	}

	auto& code = code_opt.value();
	if (code.type != QrCodeType::Program) {
		DEV_PRINT(stderr, "QR Error in get_ToolingListInfo: Not a Program QR code.\n");
		return std::nullopt;
	}

	// 共通ヘルパーを呼び出す (ID と print_time の両方を渡す)
	return query_and_populate_tooling_list(code.id, code.timestamp);
}

bool update_ToolingListInfo(ToolingListInfo& list_info) {
	// 共通ヘルパーを呼び出す (ID のみ渡し、print_time は nullopt)
	auto new_list_info_opt = query_and_populate_tooling_list(list_info.program.id, std::nullopt);

	if (new_list_info_opt.has_value()) {
		// 更新に成功した場合、中身をムーブで入れ替える
		list_info = std::move(new_list_info_opt.value());
		return true;
	} else {
		// DBからデータが取得できなかった場合
		return false;
	}
}

std::optional<StrageAreaInfo> get_StrageAreaInfo_with_min_cases(int basic_tool_id) {
	// 共通関数を呼び出し、ソート順に "ASC" (昇順) を指定
	return get_StrageAreaInfo_ordered_by_cases(basic_tool_id, "ASC");
}

std::optional<StrageAreaInfo> get_StrageAreaInfo_with_max_cases(int basic_tool_id) {
	// 共通関数を呼び出し、ソート順に "DESC" (降順) を指定
	return get_StrageAreaInfo_ordered_by_cases(basic_tool_id, "DESC");
}

} // namespace db
