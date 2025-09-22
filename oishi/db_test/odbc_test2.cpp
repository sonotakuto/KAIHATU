#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <fstream>
#include <iomanip>
#include <limits>
#include "../../00_dev/m2_tool_io/src/odbc.hpp"

// --- ヘルパー関数群 (変更なし) ---

void press_any_key_to_continue() {
	std::cout << "\n... 何かキーを押すと次に進みます ...\n" << std::endl;
	std::cin.get();
	// std::system("clear"); // Linux/macOSの場合。Windowsなら "cls"
}

std::string format_timepoint(const std::chrono::system_clock::time_point& tp) {
	std::time_t timet = std::chrono::system_clock::to_time_t(tp);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&timet), "%Y-%m-%d %H:%M:%S");
	return ss.str();
}

std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		throw std::runtime_error("ファイルを開けませんでした: " + filename);
	}
	return std::vector<char>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void writeFile(const std::string& filename, const std::vector<char>& data) {
	std::ofstream file(filename, std::ios::binary);
	file.write(data.data(), data.size());
}

// --- メイン処理 ---
int main() {
	const std::string dsn = "odbc_test";
	const std::string user = "pi";
	const std::string pass = "pipasswd";

	try {
		std::cout << "[[ 総合テストプログラム ]]" << std::endl;
		OdbcConnection db(dsn, user, pass);
		OdbcStatement stmt(db);
		int first_id = 0;
		int second_id = 0;

		// --- 1. 初期化 ---
		std::cout << "\n## 1. 初期化: テーブルを空にします ##" << std::endl;
		stmt.execute("DELETE FROM comprehensive_test");
		std::cout << "-> テーブルが空になりました。HeidiSQLで確認してください。" << std::endl;
		press_any_key_to_continue();

		// --- 2. INSERTテスト (2件) ---
		std::cout << "## 2. INSERT: データを2件挿入します ##" << std::endl;
		stmt.prepare("INSERT INTO comprehensive_test (description, integer_val, double_val, timestamp_val, blob_val) VALUES (?, ?, ?, ?, ?)");

		std::cout << "-> 1件目のデータを挿入中..." << std::endl;
		auto time1 = std::chrono::system_clock::now();
		std::vector<char> blob1 = readFile("test_blob_1.txt");
		stmt.bind(1, std::string("高性能ノートPC（日本語）"));
		stmt.bind(2, 158000);
		stmt.bind(3, 1.25);
		stmt.bind(4, time1);
		stmt.bind(5, blob1);
		stmt.executePrepared();

		std::cout << "-> 2件目のデータを挿入中..." << std::endl;
		auto time2 = std::chrono::system_clock::now();
		std::vector<char> blob2 = readFile("test_blob_2.txt");
		stmt.bind(1, std::string("Wireless Mouse"));
		stmt.bind(2, 4500);
		stmt.bind(3, 0.11);
		stmt.bind(4, time2);
		stmt.bind(5, blob2);
		stmt.executePrepared();

		std::cout << "-> 2件のデータを挿入しました。HeidiSQLで確認してください。" << std::endl;
		press_any_key_to_continue();

		stmt.execute("SELECT id FROM comprehensive_test ORDER BY id");
		if(stmt.fetch()) first_id = stmt.getData<int>(1).value_or(0);
		if(stmt.fetch()) second_id = stmt.getData<int>(1).value_or(0);
		stmt.closeCursor();

		// --- 3. UPDATEテスト ---
		std::cout << "## 3. UPDATE: 1件目のデータを更新します (ID: " << first_id << ") ##" << std::endl;
		stmt.prepare("UPDATE comprehensive_test SET description = ?, integer_val = ? WHERE id = ?");
		stmt.bind(1, std::string("【更新済み】高性能ノートPC"));
		stmt.bind(2, 160000);
		stmt.bind(3, first_id);
		stmt.executePrepared();
		std::cout << "-> 1件のデータを更新しました。HeidiSQLで確認してください。" << std::endl;
		press_any_key_to_continue();

		// --- 4. 単一行読み取りテスト (prepare, bind, fetch, getData) ---
		std::cout << "## 4. 単一行読み取りテスト: 更新後のデータを1件読み取ります ##" << std::endl;
		stmt.prepare("SELECT * FROM comprehensive_test WHERE id = ?");
		stmt.bind(1, first_id);
		stmt.executePrepared();

		if (stmt.fetch()) {
			auto id = stmt.getData<int>(1).value_or(0);
			auto desc = stmt.getData<std::string>(2).value_or("");
			auto int_val = stmt.getData<int>(3).value_or(0);
			auto double_val = stmt.getData<int>(4).value_or(0.0);
			auto time_val = stmt.getData<std::chrono::system_clock::time_point>(5);
			auto blob_val = stmt.getData<std::vector<char>>(6);
			std::string blob_file_name = std::string("retrieved_blob_from_getData_id") + std::to_string(id) + std::string(".txt");

			std::cout << "-> ID: " << id << " のデータを読み取りました:" << std::endl;
			std::cout << "   - Description: " << desc << std::endl;
			std::cout << "   - Integer    : " << int_val << std::endl;
			std::cout << "   - Double     : " << std::fixed << std::setprecision(3) << double_val << std::endl;
			std::cout << "   - Timestamp  : " << format_timepoint(time_val.value()) << std::endl;

			writeFile(blob_file_name, blob_val.value());
			std::cout << "   - BLOB       : " << blob_val->size() << " bytes (" << blob_file_name << " に保存)" << std::endl;
		} else {
			std::cerr << "-> ID: " << first_id << " のデータが見つかりませんでした。" << std::endl;
		}

		// fetchAllと違って結果を最後まで読んでいないため、カーソルを手動で閉じる必要がある
		stmt.closeCursor();
		press_any_key_to_continue();

		// --- 5. 全行読み取りテスト (fetchAll) ---
		std::cout << "## 5. SELECT (fetchAll): 全データを一括で読み取ります ##" << std::endl;
		stmt.execute("SELECT * FROM comprehensive_test ORDER BY id");
		auto results = stmt.fetchAll();

		std::cout << "-> " << results.size() << "件のデータを取得しました。" << std::endl;
		for (const auto& row : results) {
			try {
				int id = std::any_cast<int>(row.at("id"));
				std::string desc = std::any_cast<std::string>(row.at("description"));
				int int_val = std::any_cast<int>(row.at("integer_val"));
				double double_val = std::any_cast<double>(row.at("double_val"));
				auto time_val = std::any_cast<std::chrono::system_clock::time_point>(row.at("timestamp_val"));
				auto blob_val = std::any_cast<std::vector<char>>(row.at("blob_val"));
				std::string blob_file_name = std::string("retrieved_blob_from_fetchAll_id") + std::to_string(id) + std::string(".txt");

				std::cout << "\n   --- Row ID: " << id << " ---" << std::endl;
				std::cout << "   - Description: " << desc << std::endl;
				std::cout << "   - Integer    : " << int_val << std::endl;
				std::cout << "   - Double     : " << std::fixed << std::setprecision(3) << double_val << std::endl;
				std::cout << "   - Timestamp  : " << format_timepoint(time_val) << std::endl;

				writeFile(blob_file_name, blob_val);
				std::cout << "   - BLOB       : " << blob_val.size() << " bytes (" << blob_file_name << " に保存)" << std::endl;
			} catch (const std::exception& e) {
				std::cerr << "   [Error] データ処理中にエラー: " << e.what() << std::endl;
			}
		}
		press_any_key_to_continue();

		// --- 6. DELETEテスト ---
		std::cout << "## 6. DELETE: 全てのテストデータを削除します ##" << std::endl;
		int deleted_rows = stmt.execute("DELETE FROM comprehensive_test");
		std::cout << "-> " << deleted_rows << "件のデータを削除しました。HeidiSQLで確認してください。" << std::endl;
		press_any_key_to_continue();

		std::cout << "[[ 全てのテストが完了しました ]]" << std::endl;

	} catch (const std::exception& e) {
		std::cerr << "エラーが発生しました: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}