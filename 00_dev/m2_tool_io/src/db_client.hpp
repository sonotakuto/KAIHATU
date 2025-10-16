/**
 * @file db_client.hpp
 * @brief データベースにアクセスするための関数をまとめたヘッダファイル。
 */
#pragma once
#include <SimpleGtk>
#include "./typedef.hpp"
#include "./parameters.hpp"

/**
 * @namespace db
 * @brief データベース接続に関連する関数と定数を格納する名前空間。
 */
namespace db {

constexpr int	RETRY_LIMIT	= 3;	// DB接続のリトライ回数
constexpr const char*	ODBC_DSN	= "odbc_test";	// ODBCのDSN名
constexpr const char*	ODBC_USER	= "pi";			// ODBCのユーザー名
constexpr const char*	ODBC_PASS	= "pipasswd";	// ODBCのパスワード

/**
 * @brief QRコードの種類を示すenum
 */
enum class QrCodeType {
	User,		// ユーザーID
	Tool,		// 個別工具ID
	Program,	// プログラム割当ID
	Unknown		// 不明
};

/**
 * @brief パースしたQRコードのデータを格納する構造体
 */
struct QrCodeData {
	QrCodeType type;
	long long id;
	std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief QRコードの文字列を解析し、QrCodeData構造体を返す
 * @param qr_string QRコードから読み取った文字列
 * @return パースに成功した場合はQrCodeDataを含むstd::optional、失敗した場合はstd::nullopt
 */
std::optional<QrCodeData> parse_qr(const std::string& qr_string);

bool connect(void);

void disconnect(void);

bool user_authenticate(int user_id);

bool save_history(const RecOperationTable& rec);

/**
 * @brief ユーザーIDを基にユーザー情報をデータベースから取得します。
 * @param qr_string 取得対象のQRコード文字列。
 * @return std::optional<MstUserInfo> ユーザー情報が見つかった場合はその構造体、見つからなければstd::nullopt。
 */
std::optional<MstUserInfo> get_MstUserInfo(const std::string& qr_string);

/**
 * @brief 個別工具IDを基に、関連する全ての情報（基本情報、保管領域情報など）を結合して取得します。
 * @param qr 取得対象のQRコード文字列。
 * @return std::optional<UniqueToolInfo> 工具情報が見つかった場合はその構造体、見つからなければstd::nullopt。
 */
std::optional<UniqueToolInfo> get_UniqueToolInfo(const std::string& qr_string);

/**
 * @brief プログラム割り当てIDを基に、ツーリングリスト全体の情報（それに含まれる全工具の情報を含む）を取得します。
 * @param qr 取得対象のQRコード文字列。
 * @return std::optional<ToolingListInfo> ツーリングリストが見つかった場合はその構造体、見つからなければstd::nullopt。
 */
std::optional<ToolingListInfo> get_ToolingListInfo(const std::string& qr_string);

} // namespace db