#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/00_config.sh"

info "Сборка приложений..."

cd "$APP_DIR"

# Сборка сервера
info "Сборка server_app..."
g++ -std=c++23 server.cpp -o server_app

# Сборка клиента
info "Сборка client_app_26..."
PKG_CONFIG_PATH="$DPDK_PC" \
g++ -std=c++23 client.cpp -o client_app_26 \
    -I"$FSTACK_PREFIX/include" \
    $(PKG_CONFIG_PATH="$DPDK_PC" pkg-config --cflags libdpdk) \
    -L"$FSTACK_PREFIX/lib" \
    -Wl,--whole-archive -lfstack -Wl,--no-whole-archive \
    $(PKG_CONFIG_PATH="$DPDK_PC" pkg-config --static --libs libdpdk) \
    -lcrypto -lrt

info "Приложения собраны: server_app, client_app_26"
