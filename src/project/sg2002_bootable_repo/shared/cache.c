#include "kraken.h"

#if KRAKEN_ENABLE_PLATFORM_DCACHE_OPS
extern void sg2002_platform_flush_dcache_line(uintptr_t addr);
extern void sg2002_platform_inval_dcache_line(uintptr_t addr);
extern void sg2002_platform_flush_all_caches(void);
#endif

static uintptr_t line_align_down(uintptr_t v) {
    return v & ~(uintptr_t)63;
}

void flush_dcache_range(uintptr_t start, uintptr_t end) {
    if (end <= start) return;
    fence_rw();
#if KRAKEN_ENABLE_PLATFORM_DCACHE_OPS
    for (uintptr_t p = line_align_down(start); p < end; p += 64)
        sg2002_platform_flush_dcache_line(p);
#else
    for (uintptr_t p = line_align_down(start); p < end; p += 64) {
        (void)p;
        __asm__ volatile ("fence rw,rw" ::: "memory");
    }
#endif
    fence_rw();
}

void inval_dcache_range(uintptr_t start, uintptr_t end) {
    if (end <= start) return;
    fence_rw();
#if KRAKEN_ENABLE_PLATFORM_DCACHE_OPS
    for (uintptr_t p = line_align_down(start); p < end; p += 64)
        sg2002_platform_inval_dcache_line(p);
#else
    for (uintptr_t p = line_align_down(start); p < end; p += 64) {
        (void)p;
        __asm__ volatile ("fence rw,rw" ::: "memory");
    }
#endif
    fence_rw();
}

void flush_all_caches(void) {
#if KRAKEN_ENABLE_PLATFORM_DCACHE_OPS
    sg2002_platform_flush_all_caches();
#else
    fence_rw();
    fence_i();
    fence_rw();
#endif
}
