#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/00_config.sh"

info "Применение патчей для F-Stack..."

FF_DPDK_IF="$FSTACK_SRC/lib/ff_dpdk_if.c"
FF_SYMLIST="$FSTACK_SRC/lib/ff_api.symlist"

# Проверка существования файлов
[ -f "$FF_DPDK_IF" ] || error "Файл $FF_DPDK_IF не найден"
[ -f "$FF_SYMLIST" ] || error "Файл $FF_SYMLIST не найден"

# Патч 1: rte_timer_meta_init
if ! grep -q "rte_timer_meta_init(void) __attribute__((weak))" "$FF_DPDK_IF"; then
    sed -i '/#include "ff_log.h"/a \ \n/*\n * F-Stack bundled DPDK has rte_timer_meta_init(); upstream DPDK may not.\n */\nextern int rte_timer_meta_init(void) __attribute__((weak));' "$FF_DPDK_IF"
    sed -i 's/rte_timer_meta_init();/if (rte_timer_meta_init != NULL)\n    rte_timer_meta_init();/' "$FF_DPDK_IF"
    info "Патч rte_timer_meta_init применен"
else
    warn "Патч rte_timer_meta_init уже применен"
fi

# Патч 2: экспорт ff_mbuf_set_timestamp
if ! grep -q "ff_mbuf_set_timestamp" "$FF_SYMLIST"; then
    sed -i '/ff_mbuf_set_vlan_info/a ff_mbuf_set_timestamp' "$FF_SYMLIST"
    info "Патч ff_mbuf_set_timestamp применен"
else
    warn "Патч ff_mbuf_set_timestamp уже применен"
fi

info "Патчи применены"
