#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/00_config.sh"

info "Сборка F-Stack..."

# Создание структуры директорий
mkdir -p "$FSTACK_PREFIX"/{bin,lib,include,etc}

cd "$FSTACK_SRC/lib"
export FF_PATH="$FSTACK_SRC"
export PKG_CONFIG_PATH="$DPDK_PC"

make clean
make -j"$(nproc)"
make install PREFIX="$FSTACK_PREFIX" F-STACK_CONF="$FSTACK_PREFIX/etc/f-stack.conf"

info "F-Stack собран и установлен в $FSTACK_PREFIX"
