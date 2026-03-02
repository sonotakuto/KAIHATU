#!/bin/bash

# スクリプト自身のディレクトリを取得
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

echo "スクリプトに実行権限を付与します..." && \
chmod +x "${SCRIPT_DIR}/apply_themes.sh" && \
chmod +x "${SCRIPT_DIR}/apply_layouts.sh" && \
chmod +x "${SCRIPT_DIR}/set_autostart.sh" && \
echo "権限の付与が完了しました。" && \
echo "" && \
echo "設定の適用を開始します..." && \
"${SCRIPT_DIR}/apply_themes.sh" && \
"${SCRIPT_DIR}/apply_layouts.sh" && \
"${SCRIPT_DIR}/set_autostart.sh" && \
echo "設定の適用が完了しました。"
