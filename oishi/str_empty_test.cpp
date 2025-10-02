#include <iostream>
#include <string>

int main() {
    std::string str{}; // デフォルトコンストラクタで空の文字列として初期化

    if (str.empty()) {
        std::cout << "strは空です。" << std::endl;
    } else {
        std::cout << "strは空ではありません。" << std::endl;
    }

    //ちなみに、以下の書き方でも同様に空の文字列になります。
    std::string s1;
    std::string s2 = "";

    std::cout << std::boolalpha; // bool値を "true" / "false" で表示
    std::cout << "s1.empty() is " << s1.empty() << std::endl;
    std::cout << "s2.empty() is " << s2.empty() << std::endl;

    return 0;
}