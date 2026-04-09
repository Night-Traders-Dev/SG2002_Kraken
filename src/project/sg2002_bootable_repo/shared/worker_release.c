#include "kraken.h"

#if KRAKEN_ENABLE_WORKER_RESET_HOOK
extern void sg2002_platform_worker_reset_deassert(void);
#endif

int sg2002_release_worker_core(uintptr_t entry_addr) {
    MMIO32(SG2002_SYS_C906L_BOOTADDR_LO) = (uint32_t)(entry_addr & 0xffffffffu);
    MMIO32(SG2002_SYS_C906L_BOOTADDR_HI) = (uint32_t)((entry_addr >> 32) & 0xffffffffu);
    MMIO32(SG2002_SYS_C906L_CTRL_REG) |= SG2002_SYS_C906L_CTRL_EN;
    fence_rw();
#if KRAKEN_ENABLE_WORKER_RESET_HOOK
    sg2002_platform_worker_reset_deassert();
#endif
    fence_i();
    return 0;
}
