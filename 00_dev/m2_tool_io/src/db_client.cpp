/**
 * @file db_client.cpp
 * @brief db_client.hppで宣言された関数の実装ファイル。
 */
#include "./odbc.hpp"
#include "./includes.hpp"
using namespace odbc;

// データベースへの接続を管理するクラスをまとめた構造体へのポインタ
static std::unique_ptr<OdbcSession> db_session;

/**
 * @brief OdbcStatementをリトライ処理付きで安全に作成するヘルパー関数
 * @return OdbcStatementを保持するoptional。失敗した場合はnullopt。
 */
static optional<OdbcStatement> create_statement_with_retry() {
	// グローバルなDBセッションまたは接続が存在しない場合は即座に失敗させる
	if (!db_session || !db_session->connection) {
		// TODO: 接続がない旨のエラーメッセージを表示する
		return nullopt;
	}

	// リトライ処理
	for (int i = 0; i < DB_RETRY_LIMIT; i++) {
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

bool db_connect(void) {
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
}

void db_disconnect(void) {
	db_session.reset();
}

void user_authenticate(void) {
	optional<OdbcStatement> stmt_opt = create_statement_with_retry();

	if (!stmt_opt) {
		// 試行が全て失敗した場合
		// TODO:エラーメッセージを表示するコードをここに書く？
	}
}

void save_history(void) {
	optional<OdbcStatement> stmt_opt = create_statement_with_retry();

	if (!stmt_opt) {
		// 試行が全て失敗した場合
		// TODO:エラーメッセージを表示するコードをここに書く？
	}
}

void get_tool_data(void) {
	optional<OdbcStatement> stmt_opt = create_statement_with_retry();

	if (!stmt_opt) {
		// 試行が全て失敗した場合
		// TODO:エラーメッセージを表示するコードをここに書く？
	}
}

void get_tooling_list(void) {
	optional<OdbcStatement> stmt_opt = create_statement_with_retry();

	if (!stmt_opt) {
		// 試行が全て失敗した場合
		// TODO:エラーメッセージを表示するコードをここに書く？
	}
}