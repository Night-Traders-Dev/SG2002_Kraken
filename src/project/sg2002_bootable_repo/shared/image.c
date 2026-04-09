#include "kraken.h"

int sg2002_image_present(uintptr_t addr) {
    uint32_t nonzero = 0;
    uint32_t all_ones = 0;
    const volatile uint32_t *p = (const volatile uint32_t *)(uintptr_t)addr;
    for (uint32_t i = 0; i < WORKER_IMAGE_PROBE_WORDS; ++i) {
        uint32_t v = p[i];
        if (v != 0u) nonzero++;
        if (v == 0xffffffffu) all_ones++;
    }
    return nonzero != 0u && all_ones != WORKER_IMAGE_PROBE_WORDS;
}

void sg2002_copy_image(uintptr_t dst, uintptr_t src, size_t max_len, size_t *copied_len) {
    size_t n = 0;
    const uint8_t *s = (const uint8_t *)(uintptr_t)src;
    uint8_t *d = (uint8_t *)(uintptr_t)dst;

    while (n < max_len) {
        d[n] = s[n];
        ++n;
        if ((n & 63u) == 0u)
            flush_dcache_range(dst + n - 64u, dst + n);
    }
    flush_dcache_range(dst, dst + n);
    fence_i();
    if (copied_len) *copied_len = n;
}
