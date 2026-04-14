#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/00_config.sh"

info "Установка зависимостей..."
sudo apt update
sudo apt install -y \
    git build-essential meson ninja-build pkg-config \
    libnuma-dev libssl-dev libelf-dev libpcap-dev \
    libmnl-dev libnl-3-dev libnl-route-3-dev \
    libsystemd-dev python3 python3-pip libboost-dev gdb

info "Зависимости установлены"
