#pragma once
#include <SimpleGtk>
#include <memory>
#include <optional>

// ■■■■ 課題 (コンパイルエラー修正) ここから修正 ■■■■
// #include "glob_var.hpp" // 循環参照の原因となるため削除
// ■■■■ 課題 (コンパイルエラー修正) ここまで修正 ■■■■

#include "parameters.hpp"
#include "typedef.hpp"
#include "SimpleGtk_extension.hpp"
#include "tracked_map.hpp"