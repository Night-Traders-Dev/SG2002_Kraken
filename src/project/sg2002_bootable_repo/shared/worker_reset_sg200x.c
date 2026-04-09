#include "kraken.h"

#if KRAKEN_ENABLE_WORKER_RESET_HOOK
static uint32_t worker_reset_mask(void) {
    return 1u << KRAKEN_WORKER_RESET_BIT;
}

static int worker_reset_write_expect(uint32_t value, uint32_t expect_set) {
    uint32_t mask = worker_reset_mask();

    MMIO32(KRAKEN_WORKER_RESET_REG) = value;
    fence_rw();
    if ((MMIO32(KRAKEN_WORKER_RESET_REG) & mask) != expect_set)
        return -1;
    return 0;
}

int sg2002_platform_worker_reset_deassert(void) {
    uint32_t mask = worker_reset_mask();
    uint32_t reg = MMIO32(KRAKEN_WORKER_RESET_REG);

    /* SG200X exposes the C906L reset as an active-low CPUSYS2 bit. */
    if (worker_reset_write_expect(reg & ~mask, 0) != 0)
        return -1;

    delay_cycles(KRAKEN_WORKER_RESET_PULSE_CYCLES);

    reg = MMIO32(KRAKEN_WORKER_RESET_REG);
    if (worker_reset_write_expect(reg | mask, mask) != 0)
        return -1;

    return 0;
}
#endif
