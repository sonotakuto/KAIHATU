#pragma once
#include <SimpleGtk>
#include <map>
#include <memory>

#ifdef DECL
    // グローバル変数の定義
    std::map<int, std::unique_ptr<sg::Window>> win;
    auto& confirmDialog  = win[5];
#else
    // 外部参照宣言
    extern std::unique_ptr<sg::Window>& confirmDialog;
#endif