#pragma once
#include <SimpleGtk>
#include <map>
#include <memory>

// 修正点: `Window`ではなく`sg::Window`を使い、
// ライブラリのクラスであることを明示します。
#ifdef DECL
    // グローバル変数の定義
    std::map<int, std::unique_ptr<sg::Window>> win;
    auto& loginWindow         = win[1];
    auto& mainWindow          = win[3];
    auto& confirmInsertDialog = win[5]; 
#else
    // 外部参照宣言
    extern std::unique_ptr<sg::Window>& loginWindow;
    extern std::unique_ptr<sg::Window>& mainWindow;
    extern std::unique_ptr<sg::Window>& confirmInsertDialog;
#endif