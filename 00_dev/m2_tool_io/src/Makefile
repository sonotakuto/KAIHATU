# ==============================================================================
# コンパイラとコンパイルオプションの設定
# ==============================================================================

# C++コンパイラを指定
CXX = g++
#CXX = g++-10

# 使用するパッケージを指定
PACKAGES = libcurl SimpleGtk

# コンパイル時の共通オプション
# -std=c++17	: C++17規格を使用
# -Wall			: 主要な警告をすべて有効にする
# `pkg-config --cflags $(PACKAGES)` : 各パッケージに必要なコンパイルフラグを取得
CXXFLAGS = -std=c++17 -Wall -MMD -MP `pkg-config --cflags $(PACKAGES)`

# リンク時のオプション
# `pkg-config --libs $(PACKAGES)` : 各パッケージに必要なライブラリフラグを取得
LDFLAGS = `pkg-config --libs $(PACKAGES)` -lcrypt

# デバッグビルド時のみ追加されるオプション
# -g	: デバッグ情報（シンボルなど）を埋め込む
# -O0	: 最適化を無効にする（デバッグしやすくなる）
# -DDEBUG -DDEBUG_LEVEL=3 : デバッグ用のマクロを定義
DEBUGFLAGS = -g -O0 -DDEBUG -DDEBUG_LEVEL=3
#DEBUGFLAGS = -g -O0 -DDEBUG -DDEBUG_LEVEL=3 -fsanitize=address -fno-omit-frame-pointer

# 構文チェック時のみ追加されるオプション
# -fsyntax-only : 構文チェックのみを行い、オブジェクトファイルは生成しない
CHECKFLAGS = -fsyntax-only

# ==============================================================================
# プロジェクトのファイル設定【ここを編集してください】
# ==============================================================================

# コンパイル対象のソースファイル（.cpp）をスペース区切りで列挙
# 例: SOURCES = main.cpp utils.cpp
SOURCES = db_client.cpp odbc.cpp plc_communicator.cpp tool_io_gui.cpp tool_io_main.cpp tracking_communicator.cpp url_transfer.cpp

# 「このヘッダを変更したら、全てのファイルを再コンパイルしたい」というヘッダ（.h, .hpp）をスペース区切りで列挙
# これを指定すると、ヘッダーファイルの変更時にもソースが再コンパイルされる
# 通常、各ソースの依存関係はコンパイラにより自動で生成されるため、基本的には不要
# 例: HEADERS = utils.h
#HEADERS = any_extension.hpp db_client.hpp develop.hpp glob_var.hpp includes.hpp math_utils.hpp odbc.hpp parameters.hpp periodic_timer.hpp plc_communicator.hpp safe_queue.hpp SimpleGtk_extension.hpp tool_io_gui.hpp tracked_map.hpp tracking_communicator.hpp typedef.hpp url_transfer.hpp utility.hpp
HEADERS =

# 中間生成物（オブジェクトファイル）を格納するディレクトリ名
OBJECTS_DIR = objects

# 生成する実行ファイル名（通常版）
# 例: TARGET = my_app
TARGET = tool_io_exe

# 生成する実行ファイル名（デバッグ版）
# 例: DEBUG_TARGET = my_app_debug
DEBUG_TARGET = tool_io_debug

# ==============================================================================
# ビルドルール（通常は編集不要）
# ==============================================================================

# ソースファイル名からオブジェクトファイル名（.o）のリストを自動生成
OBJECTS = $(addprefix $(OBJECTS_DIR)/, $(SOURCES:.cpp=.o))

# ソースファイル名からデバッグ用オブジェクトファイル名（_debug.o）のリストを自動生成
DEBUG_OBJECTS = $(addprefix $(OBJECTS_DIR)/, $(SOURCES:.cpp=_debug.o))

# 各オブジェクトファイルの依存関係ファイルのリスト
DEPS = $(OBJECTS:.o=.d)
DEBUG_DEPS = $(DEBUG_OBJECTS:.o=.d)


# --- メインターゲット ---

# 'make' または 'make all' で実行されるデフォルトのターゲット
# 通常版の実行ファイルをビルドする
all: $(TARGET)

# 'make main' でもビルドできるようにするためのエイリアス
main: $(TARGET)

# 'make rebuild' でクリーンしてから再ビルドする
rebuild:
	$(MAKE) clean
	$(MAKE) all

# 'make exec' でビルド後に実行する
exec: $(TARGET)
	./$(TARGET)

# 通常版の実行ファイルをリンクして生成するルール
# $@: ターゲットファイル名（$(TARGET)）
# $^: すべての依存ファイル名（$(OBJECTS)）
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# デバッグ版の実行ファイルをリンクして生成するルール
$(DEBUG_TARGET): $(DEBUG_OBJECTS)
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -o $@ $^ $(LDFLAGS)

# --- オブジェクトファイル生成ルール ---

# 通常版のオブジェクトファイル（.o）を生成するルール
# $<: 最初の依存ファイル名（.cppファイル）
# オブジェクトディレクトリが無い場合は作成する
$(OBJECTS_DIR)/%.o: %.cpp $(HEADERS) $(MAKEFILE_LIST)
	@mkdir -p $(OBJECTS_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# デバッグ版のオブジェクトファイル（_debug.o）を生成するルール
# オブジェクトディレクトリが無い場合は作成する
$(OBJECTS_DIR)/%_debug.o: %.cpp $(HEADERS) $(MAKEFILE_LIST)
	@mkdir -p $(OBJECTS_DIR)
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -c $< -o $@

# --- その他のコマンド ---

# 'make debug' でデバッグ版をビルドする
debug: $(DEBUG_TARGET)
	@echo [F5]でデバッグを開始します。

# 'make check' でソースコードの構文チェックを行う
check: $(SOURCES)
	@$(foreach src, $(SOURCES), echo "Checking $(src)"; $(CXX) $(CXXFLAGS) $(CHECKFLAGS) $(src) -o /dev/null; echo "";)

# 'make clean' で生成物を削除する
clean:
	rm -rf $(OBJECTS_DIR) $(TARGET) $(DEBUG_TARGET)

# 生成された依存関係ファイルを読み込む
# （ファイルが存在しなくてもエラーにしないために -include を使う）
-include $(DEPS)
-include $(DEBUG_DEPS)

# 偽物のターゲット（Phony Target）を宣言
# これにより、同名のファイルが存在してもコマンドが正しく実行される
.PHONY: all main rebuild clean debug check
