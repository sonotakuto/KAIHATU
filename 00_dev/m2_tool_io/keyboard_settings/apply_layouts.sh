#!/bin/bash

# スクリプト自身のディレクトリを取得
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

cp -f "${SCRIPT_DIR}/layouts/"* "${HOME}/.local/share/onboard/layouts/"
