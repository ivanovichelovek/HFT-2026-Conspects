#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/00_config.sh"

info "Возврат NIC обратно в Linux..."

# Проверка драйвера (обычно r8169 для Realtek, или ixgbe для Intel)
DRIVER="r8169"  # Измените под свой NIC

sudo "$DPDK_PREFIX/bin/dpdk-devbind.py" -b "$DRIVER" "$PCI_ADDR"
sudo ip link set "$IFACE" up

info "NIC возвращен. Статус:"
sudo "$DPDK_PREFIX/bin/dpdk-devbind.py" --status
