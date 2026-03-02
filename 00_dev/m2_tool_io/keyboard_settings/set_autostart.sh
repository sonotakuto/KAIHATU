#!/bin/bash

# スクリプト自身のディレクトリを取得
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

mkdir -p ~/.config/autostart && \
cp -f "${SCRIPT_DIR}/onboard.desktop" "${HOME}/.config/autostart/"
