# 新しいMakefileがあるディレクトリ（相対パス推奨・区切りは / を使用）
NEW_DIR := ../

# 1. 引数なしで make が実行された場合
#    新しい場所のデフォルトターゲットを実行します
.DEFAULT_GOAL := default_shim

default_shim:
	@echo "通知: Makefile は $(NEW_DIR) に移動されました。転送中..."
	@$(MAKE) -C $(NEW_DIR)

# 2. 引数（ターゲット）指定で make xxx が実行された場合
#    そのターゲット名をそのまま新しい場所に渡します
%:
	@echo "通知: Makefile は $(NEW_DIR) に移動されました。転送中..."
	@$(MAKE) -C $(NEW_DIR) $@

# このルール自体をターゲットと誤認させないための設定
.PHONY: default_shim
