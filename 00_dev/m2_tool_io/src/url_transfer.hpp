/**
 * @file url_transfer.hpp
 * @brief URLを使ったデータ転送を行うクラスのヘッダーファイル。
 */
#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <utility> // std::pair用
#include <curl/curl.h>

namespace url_transfer {

/**
 * @class CurlException
 * @brief libcurl関連のエラーが発生した際にスローされる例外クラス。
 */
class CurlException : public std::runtime_error {
public:
	/**
	 * @brief CurlException を構築します。
	 * @param message エラーメッセージ。
	 * @param code libcurlのエラーコード (CURLcode)。
	 */
	CurlException(const std::string& message, CURLcode code)
		: std::runtime_error(message + " (libcurl error: " + curl_easy_strerror(code) + ")"),
		error_code(code) {}

	/**
	 * @brief libcurlのエラーコードを取得します。
	 * @return CURLcode エラーコード。
	 */
	CURLcode get_curl_code() const { return error_code; }

private:
	CURLcode error_code;
};

/**
 * @class CurlRequest
 * @brief libcurlのEasy Handleをラップし、HTTPリクエストの構築と実行を容易にするクラス。
 * @details RAIIパターンに基づき、ハンドルの初期化とクリーンアップを自動化します。
 * @note アプリケーションの開始時に一度 curl_global_init(CURL_GLOBAL_ALL) を、
 * 終了時に一度 curl_global_cleanup() を呼び出す必要があります。
 */
class CurlRequest {
public:
	/**
	 * @brief CurlRequest のコンストラクタ。
	 * @details libcurlのEasy Handleを初期化します。
	 * @throw CurlException ハンドルの初期化に失敗した場合。
	 */
	CurlRequest();

	/**
	 * @brief CurlRequest のデストラクタ。
	 * @details libcurlのEasy Handleとヘッダーリストをクリーンアップします。
	 */
	~CurlRequest();

	// コピーは禁止
	CurlRequest(const CurlRequest&) = delete;
	CurlRequest& operator=(const CurlRequest&) = delete;

	// ムーブは許可
	CurlRequest(CurlRequest&&) = default;
	CurlRequest& operator=(CurlRequest&&) = default;

	/**
	 * @brief リクエスト先のURLを設定します。
	 * @param url URL文字列。
	 */
	void set_url(const std::string& url);

	/**
	 * @brief HTTP POSTメソッドを使用し、送信するデータを設定します。
	 * @param post_data URLエンコードされたPOSTデータ文字列 (例: "key1=value1&key2=value2") や JSON 文字列など。
	 * @param content_type 送信するデータの Content-Type ヘッダー値。デフォルトは "application/x-www-form-urlencoded"。
	 */
	void set_post_data(const std::string& post_data, const std::string& content_type = "application/x-www-form-urlencoded");

	/**
	 * @brief Basic認証用のユーザー名とパスワードを設定します。
	 * @details 認証情報が "Authorization: Basic <base64>" ヘッダーとして追加されます。
	 * 既に Authorization ヘッダーが存在する場合、このメソッドの呼び出しにより上書きされます (将来の拡張)。
	 * @param username ユーザー名。
	 * @param password パスワード。
	 */
	void set_basic_auth(const std::string& username, const std::string& password);

	/**
	 * @brief カスタムHTTPヘッダーを追加します (例: "X-Custom-Header: value")。
	 * @param header 完全なヘッダー文字列。
	 * @warning Content-Type や Authorization など、他のメソッドで設定されるヘッダーを
	 * この関数で追加すると、予期しない動作を引き起こす可能性があります。
	 * 将来のバージョンでは、特定のヘッダー名が禁止され、例外がスローされる可能性があります。
	 */
	void add_header(const std::string& header);

	/**
	 * @brief カスタムHTTPヘッダーを追加します (キーと値で指定)。
	 * @param header_name ヘッダー名 (例: "X-Custom-Header")。
	 * @param header_value ヘッダー値 (例: "MyValue")。
	 * @warning Content-Type や Authorization など、他のメソッドで設定されるヘッダーを
	 * この関数で追加すると、予期しない動作を引き起こす可能性があります。
	 * 将来のバージョンでは、特定のヘッダー名が禁止され、例外がスローされる可能性があります。
	 */
	void add_header(const std::string& header_name, const std::string& header_value);

	/**
	 * @brief カスタムHTTPヘッダーを追加します (std::pairで指定)。
	 * @param header_pair ヘッダー名と値のペア。
	 * @warning Content-Type や Authorization など、他のメソッドで設定されるヘッダーを
	 * この関数で追加すると、予期しない動作を引き起こす可能性があります。
	 * 将来のバージョンでは、特定のヘッダー名が禁止され、例外がスローされる可能性があります。
	 */
	void add_header(const std::pair<std::string, std::string>& header_pair);


	/**
	 * @brief リクエストのタイムアウト時間を秒単位で設定します。
	 * @param timeout_seconds タイムアウト時間(秒)。0はタイムアウトなしを意味します。
	 */
	void set_timeout(long timeout_seconds);

	/**
	 * @brief 設定された内容に基づいてHTTPリクエストを実行します。
	 * @throw CurlException リクエストの実行に失敗した場合。
	 */
	void perform();

	/**
	 * @brief 最後に実行したリクエストのHTTPレスポンスコードを取得します。
	 * @return HTTPステータスコード (例: 200, 404)。
	 */
	long get_response_code() const;

	/**
	 * @brief 最後に実行したリクエストのHTTPレスポンスボディを取得します。
	 * @return レスポンスボディの文字列。
	 */
	const std::string& get_response_body() const;

private:
	/**
	 * @brief libcurl がレスポンスボディを受信したときに呼び出す静的コールバック関数。
	 * @param contents 受信データへのポインタ。
	 * @param size データ要素のサイズ。
	 * @param nmemb データ要素の数。
	 * @param userp CURLOPT_WRITEDATA で渡されたポインタ (ここでは CurlRequest*)。
	 * @return 処理したバイト数。
	 */
	static size_t write_callback_static(void* contents, size_t size, size_t nmemb, void* userp);

	/// @brief libcurlのEasy Handle。
	CURL* easyhandle = nullptr;
	/// @brief HTTPヘッダーリスト。
	struct curl_slist* headers = nullptr;
	/// @brief レスポンスボディを格納するバッファ。
	std::string response_body_buffer;
};

} // namespace url_transfer
