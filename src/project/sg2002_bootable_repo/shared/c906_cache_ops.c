#include "kraken.h"

#if KRAKEN_ENABLE_PLATFORM_DCACHE_OPS

/*
 * These T-Head cache-management instructions target the XuanTie C906 used on
 * the SG2002 RISC-V boot path. They assume the current Nano W firmware is
 * running in the same flat DDR mapping the repo already uses for fixed load
 * addresses and shared-memory control.
 */
static inline void c906_dcache_clean_line(uintptr_t addr) {
    __asm__ volatile (".insn i 0x0B, 0, x0, %0, 0x028" :: "r"(addr) : "memory");
}

static inline void c906_dcache_inval_line(uintptr_t addr) {
    __asm__ volatile (".insn i 0x0B, 0, x0, %0, 0x02A" :: "r"(addr) : "memory");
}

static inline void c906_dcache_clean_inval_all(void) {
    __asm__ volatile (".insn i 0x0B, 0, x0, x0, 0x003" ::: "memory");
}

static inline void c906_icache_inval_all(void) {
    __asm__ volatile (".insn i 0x0B, 0, x0, x0, 0x010" ::: "memory");
}

void sg2002_platform_flush_dcache_line(uintptr_t addr) {
    c906_dcache_clean_line(addr);
}

void sg2002_platform_inval_dcache_line(uintptr_t addr) {
    c906_dcache_inval_line(addr);
}

void sg2002_platform_flush_all_caches(void) {
    c906_dcache_clean_inval_all();
    c906_icache_inval_all();
    fence_rw();
    fence_i();
    fence_rw();
}

#endif
