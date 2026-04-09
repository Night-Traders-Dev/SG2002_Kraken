#include "kraken.h"

uint32_t sg2002_platform_caps(void) {
    uint32_t caps = PLATCAP_RISCV_C906 |
                    PLATCAP_WORKER_RELEASE |
                    PLATCAP_USB_DWC2_SCAFFOLD |
                    PLATCAP_FAULT_LOG |
                    PLATCAP_RISCV_TRAPS;
#if KRAKEN_ENABLE_WORKER_RESET_HOOK
    caps |= PLATCAP_WORKER_RESET_HOOK;
#endif
#if KRAKEN_ENABLE_PLATFORM_DCACHE_OPS
    caps |= PLATCAP_DCACHE_OPS;
#endif
#if WORKER_STAGING_ADDR != 0
    caps |= PLATCAP_WORKER_STAGING;
#endif
    return caps;
}

void sg2002_boot_8051(uintptr_t entry_addr) {
    MMIO32(SG2002_RTC_CTRL_BASE + 0x028) = 0x5a5a0001u;
    MMIO32(SG2002_RTC_CTRL_BASE + 0x020) = (uint32_t)(entry_addr >> 11);
    MMIO32(SG2002_RTC_CTRL_BASE + 0x024) = 0;
    MMIO32(SG2002_RTC_CTRL_BASE + 0x02c) = 1;
    mailbox_send_8051(CMD_BOOT, (uint32_t)entry_addr);
}

void kraken_jump_to(uintptr_t entry_addr) {
    void (*entry)(void) = (void (*)(void))(uintptr_t)entry_addr;
    fence_rw();
    fence_i();
    entry();
    for (;;) cpu_relax();
}
