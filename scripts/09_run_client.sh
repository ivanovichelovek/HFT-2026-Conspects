#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/00_config.sh"

info "Запуск клиента F-Stack..."

cd "$APP_DIR"

# Проверка, что сервер запущен
if ! pgrep -f "server_app" > /dev/null; then
    warn "Сервер, возможно, не запущен. Убедитесь, что server_app работает в другом терминале. Запустите его ./server_app"
fi

sudo ./client_app_26 \
    --conf "$FSTACK_PREFIX/etc/f-stack.conf" \
    --proc-type=primary \
    --proc-id=0
