#include "kraken.h"

#if KRAKEN_ENABLE_WORKER_RESET_HOOK
extern int sg2002_platform_worker_reset_deassert(void);
#endif

int sg2002_release_worker_core(uintptr_t entry_addr) {
    uint32_t expected_lo = (uint32_t)(entry_addr & 0xffffffffu);
    uint32_t expected_hi = (uint32_t)((entry_addr >> 32) & 0xffffffffu);
    uint32_t ctrl;

    MMIO32(SG2002_SYS_C906L_BOOTADDR_LO) = expected_lo;
    MMIO32(SG2002_SYS_C906L_BOOTADDR_HI) = expected_hi;
    fence_rw();

    if (MMIO32(SG2002_SYS_C906L_BOOTADDR_LO) != expected_lo)
        return SG2002_WORKER_RELEASE_BOOTADDR_LO_MISMATCH;
    if (MMIO32(SG2002_SYS_C906L_BOOTADDR_HI) != expected_hi)
        return SG2002_WORKER_RELEASE_BOOTADDR_HI_MISMATCH;

    ctrl = MMIO32(SG2002_SYS_C906L_CTRL_REG);
    MMIO32(SG2002_SYS_C906L_CTRL_REG) = ctrl | SG2002_SYS_C906L_CTRL_EN;
    fence_rw();

    if ((MMIO32(SG2002_SYS_C906L_CTRL_REG) & SG2002_SYS_C906L_CTRL_EN) == 0)
        return SG2002_WORKER_RELEASE_ENABLE_LATCH_FAILED;

#if KRAKEN_ENABLE_WORKER_RESET_HOOK
    if (sg2002_platform_worker_reset_deassert() != 0)
        return SG2002_WORKER_RELEASE_RESET_HOOK_FAILED;
#endif
    fence_i();
    return SG2002_WORKER_RELEASE_OK;
}
