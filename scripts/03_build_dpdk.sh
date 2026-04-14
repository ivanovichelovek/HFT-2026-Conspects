#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/00_config.sh"

info "Сборка DPDK..."
cd "$DPDK_SRC"

# Очистка предыдущей сборки
rm -rf build-fstack

meson setup build-fstack \
    -Dprefix="$DPDK_PREFIX" \
    -Dlibdir=lib/x86_64-linux-gnu \
    -Ddefault_library=static

ninja -C build-fstack
ninja -C build-fstack install

# Проверка версии
PKG_CONFIG_PATH="$DPDK_PC" pkg-config --modversion libdpdk
info "DPDK собран и установлен в $DPDK_PREFIX"
