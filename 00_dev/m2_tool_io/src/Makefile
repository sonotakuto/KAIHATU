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
# OPFLAGS		: コマンドラインから任意のオプションを追加可能
#	例: make OPFLAGS="-O3 -DTEST_MODE"
# `pkg-config --cflags $(PACKAGES)` : 各パッケージに必要なコンパイルフラグを取得
CXXFLAGS = -std=c++17 -Wall -MMD -MP `pkg-config --cflags $(PACKAGES)` $(OPFLAGS)

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
# ▼▼▼ 環境切り替え設定 (追加部分) ▼▼▼
# ==============================================================================

# 環境変数 BUILD_ENV で切り替え (デフォルト: dev)
# 実行方法:
#   開発: make
#   本番: make BUILD_ENV=prod
BUILD_ENV ?= dev

ifeq ($(BUILD_ENV), prod)
	# 本番環境用マクロ
	CXXFLAGS += -DPRODUCTION
	ENV_MSG = PRODUCTION (本番環境)
else
	# 開発環境用マクロ
	CXXFLAGS += -DDEVELOPMENT -DDEV -DSTUB_PLC
	ENV_MSG = DEVELOPMENT (開発環境)
endif

# ==============================================================================
# プロジェクトのファイル設定【ここを編集してください】
# ==============================================================================

# コンパイル対象のソースファイル（.cpp）をスペース区切りで列挙
# 例: SOURCES = main.cpp utils.cpp
SOURCES = db_client.cpp keyboard_utils.cpp odbc.cpp plc_communicator.cpp signal.cpp tool_io_gui.cpp tool_io_main.cpp tracking_communicator.cpp url_transfer.cpp

# 「このヘッダを変更したら、全てのファイルを再コンパイルしたい」というヘッダ（.h, .hpp）をスペース区切りで列挙
# これを指定すると、ヘッダーファイルの変更時にもソースが再コンパイルされる
# 通常、各ソースの依存関係はコンパイラにより自動で生成されるため、基本的には不要
# 例: HEADERS = utils.h
#HEADERS = any_extension.hpp db_client.hpp develop.hpp glob_var.hpp includes.hpp math_utils.hpp odbc.hpp parameters.hpp periodic_timer.hpp plc_communicator.hpp safe_queue.hpp SimpleGtk_extension.hpp tool_io_gui.hpp tracked_map.hpp tracking_communicator.hpp typedef.hpp url_transfer.hpp utility.hpp
HEADERS =

# 中間生成物（オブジェクトファイル）を格納するディレクトリ名
# 例: OBJECTS_DIR = objects
OBJECTS_DIR = objects

# 実行ファイルを格納するディレクトリ名
# 例: BIN_DIR = bin
BIN_DIR = bin

# 生成する実行ファイル名（通常版）
# 例: TARGET = my_app
TARGET = tool_io_exe

# 生成するデバッグ版実行ファイル名（TARGETに連動）
# 例: my_app -> my_app_debug
DEBUG_TARGET = $(TARGET)_debug

# ==============================================================================
# ビルドルール（通常は編集不要）
# ==============================================================================

# 実行ファイルのパス (例: bin/my_app)
TARGET_PATH = $(BIN_DIR)/$(TARGET)

# デバッグ版実行ファイル名のパス (例: bin/my_app_debug)
DEBUG_TARGET_PATH = $(BIN_DIR)/$(DEBUG_TARGET)

# フラグ管理ファイル
FLAGS_FILE = $(OBJECTS_DIR_INTERNAL)/.build_flags

# オブジェクトファイルの格納場所をターゲットごとに分ける
# 例: TARGET=my_app なら objects/my_app/ に保存される
OBJECTS_DIR_INTERNAL = $(OBJECTS_DIR)/$(TARGET)

# ソースファイル名からオブジェクトファイル名（.o）のリストを自動生成
OBJECTS = $(addprefix $(OBJECTS_DIR_INTERNAL)/, $(SOURCES:.cpp=.o))

# ソースファイル名からデバッグ用オブジェクトファイル名（_debug.o）のリストを自動生成
DEBUG_OBJECTS = $(addprefix $(OBJECTS_DIR_INTERNAL)/, $(SOURCES:.cpp=_debug.o))

# 各オブジェクトファイルの依存関係ファイルのリスト
DEPS = $(OBJECTS:.o=.d)
DEBUG_DEPS = $(DEBUG_OBJECTS:.o=.d)

# --- メインターゲット ---

# 'make' または 'make all' で、実行されるデフォルトのターゲット
# 通常版の実行ファイルをビルドする
all: print_env $(TARGET_PATH)

# 'make main' でもビルドできるようにするためのエイリアス
main: all

# 環境表示用ターゲット (Phony)
print_env:
	@echo "=========================================="
	@echo " Build Environment: $(ENV_MSG)"
	@echo "=========================================="

# 'make rebuild' で、クリーンしてから再ビルドする
rebuild:
	$(MAKE) clean
	$(MAKE) all

# 'make exec' で、ビルド後に実行する
exec: $(TARGET_PATH)
	./$(TARGET_PATH)

# 'make debug' で、デバッグ版をビルドする
debug: $(DEBUG_TARGET_PATH)
	@echo [F5]でデバッグを開始します。

# 'make check' で、ソースコードの構文チェックを行う
check: $(SOURCES)
	@echo "Checking source files in $(ENV_MSG) mode..."
	@$(foreach src, $(SOURCES), echo "Checking $(src)"; $(CXX) $(CXXFLAGS) $(CHECKFLAGS) $(src) -o /dev/null; echo "";)

# 'make clean' で、現在のターゲットのファイルのみ削除
clean:
	rm -rf $(OBJECTS_DIR_INTERNAL) $(TARGET_PATH) $(DEBUG_TARGET_PATH)

# 'make clean_all' で、全てのオブジェクトファイルと、bin内の全実行ファイルを削除
clean_all:
	rm -rf $(OBJECTS_DIR) $(BIN_DIR)

# --- リンクルール ---

# 通常版の実行ファイルをリンクして生成するルール
# $@: ターゲットファイル名（$(TARGET)）
# $^: すべての依存ファイル名（$(OBJECTS)）
$(TARGET_PATH): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# デバッグ版の実行ファイルをリンクして生成するルール
$(DEBUG_TARGET_PATH): $(DEBUG_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -o $@ $^ $(LDFLAGS)

# --- フラグ変更検知 ---

# CXXFLAGSの内容が変わったら再ビルドをトリガーする
$(FLAGS_FILE): FORCE
	@mkdir -p $(dir $@)
	@echo "$(CXXFLAGS)" | cmp -s - $@ || echo "$(CXXFLAGS)" > $@

FORCE:

# --- オブジェクトファイル生成ルール ---

# 通常版のオブジェクトファイル（.o）を生成するルール
# $<: 最初の依存ファイル名（.cppファイル）
# オブジェクトディレクトリが無い場合は作成する
$(OBJECTS_DIR_INTERNAL)/%.o: %.cpp $(HEADERS) $(MAKEFILE_LIST) $(FLAGS_FILE)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# デバッグ版のオブジェクトファイル（_debug.o）を生成するルール
# オブジェクトディレクトリが無い場合は作成する
$(OBJECTS_DIR_INTERNAL)/%_debug.o: %.cpp $(HEADERS) $(MAKEFILE_LIST) $(FLAGS_FILE)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -c $< -o $@

# 生成された依存関係ファイルを読み込む
# （ファイルが存在しなくてもエラーにしないために -include を使う）
-include $(DEPS)
-include $(DEBUG_DEPS)

# 偽物のターゲット（Phony Target）を宣言
# これにより、同名のファイルが存在してもコマンドが正しく実行される
.PHONY: all main rebuild exec debug check clean clean_all print_env FORCE
