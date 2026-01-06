#!/bin/bash

# スクリプト自身のディレクトリを取得
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

cp -f "${SCRIPT_DIR}/themes/Charcoal.colors" "/usr/share/onboard/themes/" && \
cp -f "${SCRIPT_DIR}/themes/My-JIS-Layout.theme" "${HOME}/.local/share/onboard/themes/"
