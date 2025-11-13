/**
 * @file url_transfer.cpp
 * @brief URLを使ったデータ転送を行うクラスの実装ファイル。
 */
#include "url_transfer.hpp"
#include <SimpleGtk>

// SimpleGtkのヘルパー関数を使う (sg名前空間をusing)
// 必要に応じて #include "sg/crypto" などを直接インクルード
using namespace sg; // sg::base64_encode のため

namespace url_transfer {

// CurlRequestクラスの実装
CurlRequest::CurlRequest() {
	easyhandle = curl_easy_init();
	if (!easyhandle) {
		// 例外を投げる前に libcurl のエラーコードを取得するのは難しい
		// 通常 easy_init が失敗することは稀
		throw std::runtime_error("curl_easy_init() failed");
	}
}

CurlRequest::~CurlRequest() {
	if (headers) {
		curl_slist_free_all(headers);
	}
	if (easyhandle) {
		curl_easy_cleanup(easyhandle);
	}
}

void CurlRequest::set_url(const std::string& url) {
	curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());
}

void CurlRequest::set_post_data(const std::string& post_data, const std::string& content_type) {
	// POSTメソッドを指定
	curl_easy_setopt(easyhandle, CURLOPT_POST, 1L);
	// POSTデータを設定
	curl_easy_setopt(easyhandle, CURLOPT_POSTFIELDS, post_data.c_str());

	// TODO: 将来的な改善：
	// 内部コンテナにヘッダーを保持する場合、ここで既存の "Content-Type" を削除してから追加する。
	add_header("Content-Type: " + content_type);
}

void CurlRequest::set_basic_auth(const std::string& username, const std::string& password) {
	std::string credentials = username + ":" + password;
	// SimpleGtk の base64_encode を使用
	std::string encoded_credentials = base64_encode(credentials);
	std::string auth_header = "Authorization: Basic " + encoded_credentials;

	// TODO: 将来的な改善：
	// 内部コンテナにヘッダーを保持する場合、ここで既存の "Authorization" を削除してから追加する。
	add_header(auth_header);
}

void CurlRequest::add_header(const std::string& header) {
	// TODO: 将来的な改善：
	// header文字列を解析し、ヘッダー名が "Content-Type" や "Authorization" など、
	// 専用セッターで設定すべきものであれば、std::invalid_argument 例外をスローする。
	// そのためには、まずヘッダーを内部コンテナに保持する方式に変更する必要がある。
	headers = curl_slist_append(headers, header.c_str());
	if (!headers) {
		throw std::runtime_error("curl_slist_append() failed to add header: " + header);
	}
}

void CurlRequest::add_header(const std::string& header_name, const std::string& header_value) {
	// ヘッダー名と値を ": " で結合して、既存の add_header に渡す
	add_header(header_name + ": " + header_value);
}

void CurlRequest::add_header(const std::pair<std::string, std::string>& header_pair) {
	// pair の first と second を使って、別のオーバーロードに渡す
	add_header(header_pair.first, header_pair.second);
}

void CurlRequest::set_timeout(long timeout_seconds) {
	curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, timeout_seconds);
}

void CurlRequest::perform() {
	// ヘッダーリストを設定 (add_headerが呼ばれていなくても headers == nullptr なので問題ない)
	curl_easy_setopt(easyhandle, CURLOPT_HTTPHEADER, headers);

	// レスポンスボディをバッファに書き込むための設定
	response_body_buffer.clear(); // 前回の結果をクリア
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, write_callback_static);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, this); // コールバックにthisポインタを渡す

	// HTTPリクエストを実行
	CURLcode res = curl_easy_perform(easyhandle);

	// エラーチェック
	if (res != CURLE_OK) {
		throw CurlException("curl_easy_perform() failed", res);
	}
}

long CurlRequest::get_response_code() const {
	long http_code = 0;
	// perform() が成功した後でのみ意味のある値を返す
	curl_easy_getinfo(easyhandle, CURLINFO_RESPONSE_CODE, &http_code);
	return http_code;
}

const std::string& CurlRequest::get_response_body() const {
	return response_body_buffer;
}

// 静的コールバック関数
size_t CurlRequest::write_callback_static(void* contents, size_t size, size_t nmemb, void* userp) {
	// userp を CurlRequest のインスタンスポインタにキャスト
	CurlRequest* request_instance = static_cast<CurlRequest*>(userp);
	size_t real_size = size * nmemb;
	// インスタンスのバッファにデータを追記
	request_instance->response_body_buffer.append(static_cast<char*>(contents), real_size);
	return real_size;
}

} // namespace url_transfer
