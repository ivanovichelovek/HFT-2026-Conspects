#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "========================================="
echo "Полная сборка F-Stack/DPDK проекта"
echo "========================================="

# Проверка конфигурации
if [ ! -f "00_config.sh" ]; then
    echo "Ошибка: файл 00_config.sh не найден"
    echo "Скопируйте 00_config.sh.example и настройте его"
    exit 1
fi

# Шаги сборки
./01_install_deps.sh
./02_clone_repos.sh
./03_build_dpdk.sh
./04_fix_fstack.sh
./05_build_fstack.sh
./06_config_fstack.sh
./07_build_apps.sh

echo "========================================="
echo "Сборка завершена!"
echo "========================================="
echo "Для запуска:"
echo "1. В терминале 1: cd $APP_DIR && ./server_app"
echo "2. В терминале 2: ./scripts/08_setup_hugepages.sh"
echo "3. В терминале 2: ./scripts/09_run_client.sh"
echo "4. Для возврата NIC: ./scripts/10_cleanup_nic.sh"
