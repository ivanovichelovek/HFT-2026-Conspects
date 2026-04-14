#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/00_config.sh"

info "Создание конфигурации F-Stack..."

cat > "$FSTACK_PREFIX/etc/f-stack.conf" <<EOF
[dpdk]
lcore_mask=1
channel=4
promiscuous=1
numa_on=1
allow=$PCI_ADDR
port_list=0

[port0]
addr=$CLIENT_IP
netmask=$NETMASK
broadcast=$BCAST
gateway=$GW
EOF

info "Конфиг создан: $FSTACK_PREFIX/etc/f-stack.conf"
cat "$FSTACK_PREFIX/etc/f-stack.conf"
