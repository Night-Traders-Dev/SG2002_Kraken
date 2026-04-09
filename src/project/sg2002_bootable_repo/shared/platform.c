#include "kraken.h"

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
