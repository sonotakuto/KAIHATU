#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include "../../00_dev/m2_tool_io/src/odbc.hpp"

// --- メイン処理 ---
int main() {
	// DSN (データソース名)、ユーザー名、パスワードを環境に合わせて設定してください
	const std::string dsn = "odbc_test"; // 例: "MariaDB-TEST" など
	const std::string user = "pi";
	const std::string pass = "pipasswd";

	try {
		std::cout << "----------------------------------------" << std::endl;
		std::cout << "1. データベース接続テスト" << std::endl;
		OdbcConnection db(dsn, user, pass);
		std::cout << "-> 接続成功！" << std::endl;
		std::cout << "----------------------------------------" << std::endl;

		OdbcStatement stmt(db);

		// テスト前にテーブルを空にする
		stmt.execute("DELETE FROM products");

		// --- 書き込みテスト (INSERT) ---
		std::cout << "2. 書き込みテスト (INSERT)" << std::endl;
		stmt.execute("INSERT INTO products (name, price, weight) VALUES ('デスクトップPC', 128000, 5.5)");
		stmt.execute("INSERT INTO products (name, price, weight) VALUES ('メカニカルキーボード', 18500, 1.12)");
		stmt.execute("INSERT INTO products (name, price, weight) VALUES ('4Kモニター', 65000, 3.7)");
		std::cout << "-> 3件のデータを挿入しました。" << std::endl;
		std::cout << "----------------------------------------" << std::endl;

		// --- 書き込みテスト (UPDATE) ---
		std::cout << "3. 書き込みテスト (UPDATE)" << std::endl;
		stmt.execute("UPDATE products SET price = 19800 WHERE name = 'メカニカルキーボード'");
		std::cout << "-> 価格を更新しました。" << std::endl;
		std::cout << "----------------------------------------" << std::endl;

		// --- 単一行読み取りテスト (fetch, getData) ---
		std::cout << "4. 単一行読み取りテスト (fetch, getData)" << std::endl;
		stmt.execute("SELECT name, price, weight FROM products WHERE name = 'メカニカルキーボード'");

		if (stmt.fetch()) {
			// 各型でデータを取得
			auto name = stmt.getData<std::string>(1);
			auto price = stmt.getData<int>(2);
			auto weight = stmt.getData<double>(3);

			std::cout << "-> 取得データ:" << std::endl;
			std::cout << "   Name  : " << (name.has_value() ? name.value() : "NULL") << std::endl;
			std::cout << "   Price : " << (price.has_value() ? std::to_string(price.value()) : "NULL") << std::endl;
			// double型を小数点以下2桁で表示
			if (weight.has_value()) {
				std::cout << "   Weight: " << std::fixed << std::setprecision(2) << weight.value() << std::endl;
			} else {
				std::cout << "   Weight: NULL" << std::endl;
			}
		} else {
			std::cerr << "-> データが見つかりませんでした。" << std::endl;
		}

		// 次のクエリを実行する前に、現在のカーソルを閉じる
		stmt.closeCursor();

		std::cout << "----------------------------------------" << std::endl;

		// --- 全行読み取りテスト (fetchAll) ---
		std::cout << "5. 全行読み取りテスト (fetchAll)" << std::endl;
		stmt.execute("SELECT name, price, weight FROM products ORDER BY price DESC");
		auto results = stmt.fetchAll();

		std::cout << "-> " << results.size() << "件のデータを取得しました。(価格の高い順)" << std::endl;
		for (const auto& row : results) {
			// std::any_cast を使って各々の型に変換
			std::string name = std::any_cast<std::string>(row.at("name"));
			int price = std::any_cast<int>(row.at("price"));
			double weight = std::any_cast<double>(row.at("weight"));

			std::cout << "   - Name: " << name
					<< ", Price: " << price
					<< ", Weight: " << std::fixed << std::setprecision(2) << weight << std::endl;
		}
		std::cout << "----------------------------------------" << std::endl;

		// 次のクエリを実行する前に、現在のカーソルを閉じる
		stmt.closeCursor();

		// --- 後片付け (DELETE) ---
		std::cout << "6. 後片付け (DELETE)" << std::endl;
		int deleted_rows = stmt.execute("DELETE FROM products");
		std::cout << "-> " << deleted_rows << "件のテストデータを削除しました。" << std::endl; // <- このように行数を表示できる
		std::cout << "----------------------------------------" << std::endl;

		std::cout << "全てのテストが正常に完了しました！" << std::endl;

	} catch (const OdbcException& e) {
		std::cerr << "[DATABASE ERROR] " << e.what() << std::endl;
		return 1;
	} catch (const std::bad_any_cast& e) {
		std::cerr << "[TYPE CAST ERROR] " << e.what() << std::endl;
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "[GENERAL ERROR] " << e.what() << std::endl;
		return 1;
	}

	return 0;
}