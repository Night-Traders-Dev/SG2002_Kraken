#include "kraken.h"

void flush_dcache_range(uintptr_t start, uintptr_t end) {
    if (end <= start) return;
    fence_rw();
#if KRAKEN_ENABLE_PLATFORM_DCACHE_OPS
    extern void sg2002_platform_flush_dcache_line(uintptr_t addr);
    uintptr_t line_align_start = start & ~(uintptr_t)63;

    for (uintptr_t p = line_align_start; p < end; p += 64)
        sg2002_platform_flush_dcache_line(p);
#else
    (void)start;
    (void)end;
#endif
    fence_rw();
}

void inval_dcache_range(uintptr_t start, uintptr_t end) {
    if (end <= start) return;
    fence_rw();
#if KRAKEN_ENABLE_PLATFORM_DCACHE_OPS
    extern void sg2002_platform_inval_dcache_line(uintptr_t addr);
    uintptr_t line_align_start = start & ~(uintptr_t)63;

    for (uintptr_t p = line_align_start; p < end; p += 64)
        sg2002_platform_inval_dcache_line(p);
#else
    (void)start;
    (void)end;
#endif
    fence_rw();
}

void flush_all_caches(void) {
#if KRAKEN_ENABLE_PLATFORM_DCACHE_OPS
    extern void sg2002_platform_flush_all_caches(void);

    sg2002_platform_flush_all_caches();
#else
    fence_rw();
    fence_i();
    fence_rw();
#endif
}
