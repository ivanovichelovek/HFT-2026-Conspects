#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/00_config.sh"

info "Настройка hugepages и привязка NIC..."

# Загрузка модуля
sudo modprobe vfio-pci || warn "vfio-pci уже загружен"

# Настройка hugepages
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# Проверка статуса
info "Текущий статус NIC:"
sudo "$DPDK_PREFIX/bin/dpdk-devbind.py" --status

# Привязка интерфейса
sudo ip link set "$IFACE" down
sudo "$DPDK_PREFIX/bin/dpdk-devbind.py" -b vfio-pci "$PCI_ADDR"

info "Статус после привязки:"
sudo "$DPDK_PREFIX/bin/dpdk-devbind.py" --status

info "Hugepages настроены, NIC привязан к vfio-pci"
