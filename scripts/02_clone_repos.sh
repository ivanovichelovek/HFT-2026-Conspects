#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/00_config.sh"

info "Клонирование репозиториев..."
if [ ! -d "$DPDK_SRC" ]; then
    git clone https://github.com/DPDK/dpdk.git "$DPDK_SRC"
    info "DPDK склонирован"
else
    warn "DPDK уже существует, пропускаем"
fi

if [ ! -d "$FSTACK_SRC" ]; then
    git clone https://github.com/F-Stack/f-stack.git "$FSTACK_SRC"
    info "F-Stack склонирован"
else
    warn "F-Stack уже существует, пропускаем"
fi
