#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <curl/curl.h>

// Base64エンコードを行う簡単な関数
// 本番環境ではより堅牢なライブラリの使用を推奨します
std::string base64_encode(const std::string &in) {
    const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

// 現在時刻からファイル名を生成する関数
// "recording_YYYYMMDD_HHMMSS.mp4" の形式
std::string get_timestamp_filename() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char buffer[80];
    strftime(buffer, 80, "recording_%Y%m%d_%H%M%S.mp4", ltm);
    return std::string(buffer);
}

// libcurlからのレスポンスを受け取るためのコールバック関数
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// サーバーにHTTP POSTリクエストを送信するメイン関数
void send_request(const std::string& command, const std::string& server_url, const std::string& username, const std::string& password) {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if(curl) {
        // std::string url = server_url + "/record/" + command;
        std::string url = server_url + "/helloWeb-1.0/record/" + command;   //tomcatに合わせて修正
        std::string post_data = "";

        if (command == "start") {
            std::string filename = get_timestamp_filename();
            post_data = "filename=" + filename;
            std::cout << "録画を開始します。ファイル名: " << filename << std::endl;
        } else {
            std::cout << "録画を停止します。" << std::endl;
        }

        // 認証情報の作成 ("username:password" を Base64エンコード)
        std::string credentials = username + ":" + password;
        std::string encoded_credentials = base64_encode(credentials);
        std::string auth_header = "Authorization: Basic " + encoded_credentials;

        // HTTPヘッダーの設定
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, auth_header.c_str());

        // curlオプション設定
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        if (!post_data.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // HTTPリクエストの実行
        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            std::cout << "サーバーからの応答 (HTTP " << http_code << "): " << readBuffer << std::endl;
        }

        // 後処理
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "使い方: " << argv[0] << " <start|stop>" << std::endl;
        return 1;
    }

    std::string command = argv[1];
    if (command != "start" && command != "stop") {
        std::cerr << "エラー: コマンドは 'start' または 'stop' である必要があります。" << std::endl;
        return 1;
    }

    // --- 設定値 (ご自身の環境に合わせて変更してください) ---
    // const std::string SERVER_URL = "http://10.19.133.118:8080"; // JavaサーバーのIPアドレスとポート
    const std::string SERVER_URL = "http://10.19.133.120:8080"; // JavaサーバーのIPアドレスとポート
    const std::string USERNAME = "yourUsername"; // Basic認証のユーザー名
    const std::string PASSWORD = "yourPassword"; // Basic認証のパスワード
    // ----------------------------------------------------

    send_request(command, SERVER_URL, USERNAME, PASSWORD);

    return 0;
}