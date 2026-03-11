#!/bin/bash

# スクリプト自身のディレクトリを取得
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

# sudo経由での実行を禁止するチェック
if [ -n "$SUDO_USER" ]; then
    echo "エラー: このスクリプトを sudo で実行しないでください。"
    echo "一般ユーザーとして直接実行してください（必要な箇所で個別に sudo が呼ばれます）。"
    exit 1
fi

mkdir -p "${HOME}/.local/share/onboard/layouts/" && \
cp -f "${SCRIPT_DIR}/layouts/"* "${HOME}/.local/share/onboard/layouts/"
