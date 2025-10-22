#include "odbc.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>

// ヘルパー関数群 (変更なし、内容は省略)
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

// format_timepoint (optional版)
std::string format_timepoint(const std::optional<std::chrono::system_clock::time_point>& tp_opt) {
	if (tp_opt) return format_timepoint(tp_opt.value());
	return "NULL";
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
		std::cout << "[[ プリペアドステートメント 総合テスト ]]" << std::endl;
		odbc::OdbcConnection db(dsn, user, pass);
		odbc::OdbcStatement stmt(db);
		int last_id = 0;

		// --- 1. 初期化 ---
		std::cout << "\n## 1. 初期化: テーブルを空にします ##" << std::endl;
		stmt.execute("DELETE FROM prepared_stmt_test");
		std::cout << "-> テーブルが空になりました。HeidiSQLで確認してください。" << std::endl;
		press_any_key_to_continue();

		// --- 2. 単一行 INSERT (値あり) ---
		std::cout << "## 2. 単一行 INSERT (値あり) ##" << std::endl;
		stmt.prepare("INSERT INTO prepared_stmt_test (int_val, uint_val, bigint_val, ubigint_val, double_val, string_val, blob_val, timestamp_val) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

		auto time_now = std::chrono::system_clock::now();
		std::vector<char> blob_data1 = readFile("test_blob_1.txt");

		stmt.bind(1, 123);                                  // int
		stmt.bind(2, (unsigned int)456);                    // unsigned int
		stmt.bind(3, -789LL);                               // long long
		stmt.bind(4, 1234567890123456789ULL);             // unsigned long long
		stmt.bind(5, 12.345);                               // double
		stmt.bind(6, std::string("テスト文字列1"));          // string
		stmt.bind(7, blob_data1);                           // vector<char>
		stmt.bind(8, time_now);                             // time_point

		stmt.executePrepared();
		std::cout << "-> 1行挿入しました。HeidiSQLで確認してください。" << std::endl;
		press_any_key_to_continue();

		// --- 3. 単一行 INSERT (NULLあり) ---
		std::cout << "## 3. 単一行 INSERT (NULLあり) ##" << std::endl;
		// prepareは同じSQLなので不要

		std::optional<int> int_null = std::nullopt;
		std::optional<unsigned int> uint_null = std::nullopt;
		std::optional<long long> bigint_null = std::nullopt;
		std::optional<unsigned long long> ubigint_null = std::nullopt;
		std::optional<double> double_null = std::nullopt;
		std::optional<std::string> string_null = std::nullopt;
		std::optional<std::vector<char>> blob_null = std::nullopt;
		std::optional<std::chrono::system_clock::time_point> time_null = std::nullopt;

		stmt.bind(1, int_null);
		stmt.bind(2, uint_null);
		stmt.bind(3, bigint_null);
		stmt.bind(4, ubigint_null);
		stmt.bind(5, double_null);
		stmt.bind(6, string_null);
		stmt.bind(7, blob_null);
		stmt.bind(8, time_null);

		stmt.executePrepared();
		std::cout << "-> NULL値を含む1行を挿入しました。HeidiSQLで確認してください。" << std::endl;
		press_any_key_to_continue();

		// --- 4. 単一行 UPDATE ---
		std::cout << "## 4. 単一行 UPDATE (最初の行の文字列と整数を更新) ##" << std::endl;
		stmt.execute("SELECT MIN(id) FROM prepared_stmt_test"); // 最初の行のIDを取得
		if(stmt.fetch()) last_id = stmt.getData<int>(1).value_or(0);
		stmt.closeCursor();

		stmt.prepare("UPDATE prepared_stmt_test SET string_val = ?, int_val = ? WHERE id = ?");
		stmt.bind(1, std::string("更新された文字列"));
		stmt.bind(2, 999);
		stmt.bind(3, last_id);
		stmt.executePrepared();
		std::cout << "-> ID=" << last_id << " の行を更新しました。HeidiSQLで確認してください。" << std::endl;
		press_any_key_to_continue();

		// --- 5. 単一行 SELECT (更新確認) ---
		std::cout << "## 5. 単一行 SELECT (更新確認 ID=" << last_id << ") ##" << std::endl;
		stmt.prepare("SELECT int_val, string_val FROM prepared_stmt_test WHERE id = ?");
		stmt.bind(1, last_id);
		stmt.executePrepared();
		if(stmt.fetch()){
			auto int_val = stmt.getData<int>(1);
			auto str_val = stmt.getData<std::string>(2);
			std::cout << "-> 取得データ: int=" << (int_val ? std::to_string(*int_val) : "NULL")
					  << ", string=" << (str_val ? *str_val : "NULL") << std::endl;
		}
		stmt.closeCursor();
		press_any_key_to_continue();

		// --- 6. 複数行 INSERT (NULL非対応版) ---
		std::cout << "## 6. 複数行 INSERT (NULL非対応版, 2行) ##" << std::endl;
		stmt.prepare("INSERT INTO prepared_stmt_test (int_val, uint_val, bigint_val, ubigint_val, double_val, string_val, blob_val, timestamp_val) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
		stmt.setParamsetSize(2);

		std::vector<int> ints = {10, 20};
		std::vector<unsigned int> uints = {11, 21};
		std::vector<long long> bigints = {100LL, 200LL};
		std::vector<unsigned long long> ubigints = {101ULL, 201ULL};
		std::vector<double> doubles = {1.1, 2.2};
		std::vector<std::string> strings = {"Bulk String 1", "バルク文字列 2"};
		std::vector<std::vector<char>> blobs = {readFile("test_blob_1.txt"), readFile("test_blob_2.txt")};
		std::vector<std::chrono::system_clock::time_point> times = {std::chrono::system_clock::now(), std::chrono::system_clock::now() + std::chrono::seconds(1)};

		stmt.bindColumn(1, ints);
		stmt.bindColumn(2, uints);
		stmt.bindColumn(3, bigints);
		stmt.bindColumn(4, ubigints);
		stmt.bindColumn(5, doubles);
		stmt.bindColumn(6, strings);
		stmt.bindColumn(7, blobs);
		stmt.bindColumn(8, times);

		stmt.executePrepared();
		std::cout << "-> NULL非対応データ2行を一括挿入しました。HeidiSQLで確認してください。" << std::endl;
		press_any_key_to_continue();

		// --- 7. 複数行 INSERT (NULL対応版) ---
		std::cout << "## 7. 複数行 INSERT (NULL対応版, 3行) ##" << std::endl;
		// prepareは同じSQLなので不要
		stmt.setParamsetSize(3);

		std::vector<std::optional<int>> opt_ints = {30, std::nullopt, 50};
		std::vector<std::optional<unsigned int>> opt_uints = {std::nullopt, 41, 61};
		std::vector<std::optional<long long>> opt_bigints = {300LL, 500LL, std::nullopt};
		std::vector<std::optional<unsigned long long>> opt_ubigints = {std::nullopt, 401ULL, std::nullopt};
		std::vector<std::optional<double>> opt_doubles = {3.3, std::nullopt, 5.5};
		std::vector<std::optional<std::string>> opt_strings = {"Bulk Opt 1", std::nullopt, "バルク Opt 3"};
		std::vector<std::optional<std::vector<char>>> opt_blobs = {std::make_optional(readFile("test_blob_1.txt")), std::nullopt, std::make_optional(std::vector<char>{'a', 'b', 'c'})};
		std::vector<std::optional<std::chrono::system_clock::time_point>> opt_times = {std::chrono::system_clock::now(), std::nullopt, std::chrono::system_clock::now() + std::chrono::minutes(1)};

		stmt.bindColumn(1, opt_ints);
		stmt.bindColumn(2, opt_uints);
		stmt.bindColumn(3, opt_bigints);
		stmt.bindColumn(4, opt_ubigints);
		stmt.bindColumn(5, opt_doubles);
		stmt.bindColumn(6, opt_strings);
		stmt.bindColumn(7, opt_blobs);
		stmt.bindColumn(8, opt_times);

		stmt.executePrepared();
		std::cout << "-> NULL対応データ3行を一括挿入しました。HeidiSQLで確認してください。" << std::endl;
		press_any_key_to_continue();

		// --- 8. 全件 SELECT (fetchAll) ---
		std::cout << "## 8. 全件 SELECT (fetchAll) ##" << std::endl;
		stmt.execute("SELECT * FROM prepared_stmt_test ORDER BY id");
		auto results = stmt.fetchAll();
		std::cout << "-> " << results.size() << "件のデータを取得しました。内容を確認してください:" << std::endl;

		for (const auto& row : results) {
			std::cout << "\n--- ID: " << std::any_cast<int>(row.at("id")) << " ---" << std::endl;
			try{ std::cout << "  Int     : " << (row.at("int_val").has_value() ? std::to_string(std::any_cast<int>(row.at("int_val"))) : "NULL") << std::endl; } catch(...){}
			try{ std::cout << "  UInt    : " << (row.at("uint_val").has_value() ? std::to_string(std::any_cast<unsigned int>(row.at("uint_val"))) : "NULL") << std::endl; } catch(...){}
			try{ std::cout << "  BigInt  : " << (row.at("bigint_val").has_value() ? std::to_string(std::any_cast<long long>(row.at("bigint_val"))) : "NULL") << std::endl; } catch(...){}
			try{ std::cout << "  UBigInt : " << (row.at("ubigint_val").has_value() ? std::to_string(std::any_cast<unsigned long long>(row.at("ubigint_val"))) : "NULL") << std::endl; } catch(...){}
			try{ std::cout << "  Double  : " << (row.at("double_val").has_value() ? std::to_string(std::any_cast<double>(row.at("double_val"))) : "NULL") << std::endl; } catch(...){}
			try{ std::cout << "  String  : " << (row.at("string_val").has_value() ? std::any_cast<std::string>(row.at("string_val")) : "NULL") << std::endl; } catch(...){}
			try{ std::cout << "  Blob    : " << (row.at("blob_val").has_value() ? std::to_string(std::any_cast<std::vector<char>>(row.at("blob_val")).size()) + " bytes" : "NULL") << std::endl; } catch(...){}
			try{ std::cout << "  Time    : " << (row.at("timestamp_val").has_value() ? format_timepoint(std::any_cast<std::chrono::system_clock::time_point>(row.at("timestamp_val"))) : "NULL") << std::endl; } catch(...){}
		}
		press_any_key_to_continue();

		// --- 9. 後片付け ---
		std::cout << "## 9. 後片付け: テーブルを空にします ##" << std::endl;
		int deleted_rows = stmt.execute("DELETE FROM prepared_stmt_test");
		std::cout << "-> " << deleted_rows << "件のデータを削除しました。" << std::endl;
		press_any_key_to_continue();

		std::cout << "[[ 全てのテストが完了しました ]]" << std::endl;

	} catch (const std::exception& e) {
		std::cerr << "エラーが発生しました: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}