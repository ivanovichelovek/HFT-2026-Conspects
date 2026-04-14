#!/bin/bash
set -euo pipefail

# ============================================
# Kонфигурация проекта - ИЗМЕНИТЕ ПОД СЕБЯ, КАК ИЗМЕНИТЬ - ЧИТАТЬ ВНИЗУ
# ДЛЯ НАСТРОЙКИ НЕОБХОДИМО ПОДКЛЮЧИТЬ Ethernet кабель
# ============================================


# Пути
export APP_DIR="$HOME/dpdk_again"
export FSTACK_SRC="$HOME/f-stack"
export DPDK_SRC="$HOME/dpdk"
export DPDK_PREFIX="$HOME/dpdk-install"
export FSTACK_PREFIX="$HOME/f-stack-install-26"

# Сеть
export IFACE="enp1s0"
export PCI_ADDR="0000:01:00.0"
export CLIENT_IP="192.168.1.26"
export NETMASK="255.255.255.0"
export BCAST="192.168.1.255"
export GW="192.168.1.1"
export SERVER_IP="192.168.1.160"

# ============================================
# Производные переменные (не менять)
# ============================================
export DPDK_PC="$DPDK_PREFIX/lib/x86_64-linux-gnu/pkgconfig"
export PKG_CONFIG_PATH="$DPDK_PC"

# ============================================
# Цветной вывод
# ============================================
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

info() { echo -e "${GREEN}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

# ============================================
# APP_DIR - папка с кодами client.cpp и server.cpp
# FSTACK_SRC - папка f-stack (склонировать можно позже)
# DPDK_SRC - папка DPDK (склонировать можно позже)
# DPDK_PREFIX и FSTACK_PREFIX можно выбрать место по желанию
# IFACE - имя Ethernet-интерфейса, который отдаешь DPDK (например enp1s0). 
# Смотреть так:
# - ip -br link
# Искать  именно проводной интерфейс (enp.../eth...),
# 
# SERVER_IP - IP машины, где запущен сервер (Boost.Asio).
# CLIENT_IP — свободный IP для F-Stack/DPDK порта в той же подсети.
# NETMASK, BCAST — из параметров этой подсети.
# GW — IP шлюза (обычно роутер).
# 
# На машине, где сервер:
# ```bash
# ip -4 addr
# ip route
# ```
# Пример вывода:
#   - inet 192.168.1.160/24 brd 192.168.1.255 ...
#   - default via 192.168.1.1 ...
# Тогда:
#   - SERVER_IP=192.168.1.160
#   - NETMASK=255.255.255.0 (из /24)
#   - BCAST=192.168.1.255
#   - GW=192.168.1.1
# CLIENT_IP выбираешь свободный в этой же сети
