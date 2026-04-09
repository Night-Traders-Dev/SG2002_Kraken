#include "kraken.h"

static int load_staged_footer(const uint8_t *base, size_t off, size_t max_len,
                              kraken_staged_image_footer_t *footer) {
    kraken_staged_image_footer_t candidate;

    if (off > max_len || (max_len - off) < sizeof(candidate))
        return 0;

    sg2002_memcpy(&candidate, base + off, sizeof(candidate));
    if (candidate.magic != KRAKEN_STAGED_IMAGE_MAGIC ||
        candidate.version != KRAKEN_STAGED_IMAGE_VERSION ||
        candidate.tail_magic != KRAKEN_STAGED_IMAGE_TAIL)
        return 0;
    if (candidate.payload_size == 0 || candidate.payload_size != off)
        return 0;
    if ((candidate.payload_crc32 ^ candidate.payload_crc32_inv) != 0xffffffffu)
        return 0;

    if (footer) {
        footer->magic = candidate.magic;
        footer->version = candidate.version;
        footer->image_kind = candidate.image_kind;
        footer->flags = candidate.flags;
        footer->load_addr = candidate.load_addr;
        footer->entry_addr = candidate.entry_addr;
        footer->payload_size = candidate.payload_size;
        footer->payload_crc32 = candidate.payload_crc32;
        footer->payload_crc32_inv = candidate.payload_crc32_inv;
        footer->tail_magic = candidate.tail_magic;
    }
    return 1;
}

static int maybe_staged_footer_tail(const uint8_t *base, size_t off, size_t max_len) {
    uint32_t tail_magic;

    if (off > max_len || (max_len - off) < sizeof(kraken_staged_image_footer_t))
        return 0;

    sg2002_memcpy(&tail_magic,
                  base + off + offsetof(kraken_staged_image_footer_t, tail_magic),
                  sizeof(tail_magic));
    return tail_magic == KRAKEN_STAGED_IMAGE_TAIL;
}

int sg2002_image_present(uintptr_t addr) {
    kraken_staged_image_footer_t footer;
    uint32_t nonzero = 0;
    uint32_t all_ones = 0;
    const volatile uint32_t *p = (const volatile uint32_t *)(uintptr_t)addr;

    if (sg2002_find_staged_image(addr,
                                 WORKER_IMAGE_MAX_BYTES +
                                     sizeof(kraken_staged_image_footer_t),
                                 &footer)) {
        return 1;
    }

    for (uint32_t i = 0; i < WORKER_IMAGE_PROBE_WORDS; ++i) {
        uint32_t v = p[i];
        if (v != 0u) nonzero++;
        if (v == 0xffffffffu) all_ones++;
    }
    return nonzero != 0u && all_ones != WORKER_IMAGE_PROBE_WORDS;
}

uint32_t sg2002_crc32(const void *buf, size_t len) {
    const uint8_t *p = (const uint8_t *)buf;
    uint32_t crc = 0xffffffffu;

    for (size_t i = 0; i < len; ++i) {
        crc ^= p[i];
        for (uint32_t bit = 0; bit < 8u; ++bit) {
            if (crc & 1u) crc = (crc >> 1) ^ 0xedb88320u;
            else crc >>= 1;
        }
    }
    return crc ^ 0xffffffffu;
}

int sg2002_find_staged_image(uintptr_t addr, size_t max_len,
                             kraken_staged_image_footer_t *footer) {
    const uint8_t *base = (const uint8_t *)(uintptr_t)addr;
    size_t off;

    if (max_len < sizeof(kraken_staged_image_footer_t))
        return 0;

    off = max_len - sizeof(kraken_staged_image_footer_t);
    if (load_staged_footer(base, off, max_len, footer))
        return 1;

    /*
     * Some callers provide a search window rather than an exact staged-image
     * length, so retain a compatibility scan. Filter candidates by tail magic
     * first to avoid copying a full footer at every byte offset.
     */
    for (;;) {
        if (maybe_staged_footer_tail(base, off, max_len) &&
            load_staged_footer(base, off, max_len, footer))
            return 1;
        if (off == 0)
            break;
        --off;
    }
    return 0;
}

int sg2002_validate_staged_image(uintptr_t addr, size_t max_len,
                                 uint32_t expected_kind,
                                 uintptr_t expected_load_addr,
                                 uintptr_t expected_entry_addr,
                                 kraken_staged_image_footer_t *footer) {
    kraken_staged_image_footer_t candidate;
    uint32_t crc;

    if (!sg2002_find_staged_image(addr, max_len, &candidate))
        return 0;
    if (expected_kind != 0u && candidate.image_kind != expected_kind)
        return 0;
    if (expected_load_addr != 0u &&
        candidate.load_addr != (uint32_t)(expected_load_addr & 0xffffffffu))
        return 0;
    if (expected_entry_addr != 0u &&
        candidate.entry_addr != (uint32_t)(expected_entry_addr & 0xffffffffu))
        return 0;

    crc = sg2002_crc32((const void *)(uintptr_t)addr, candidate.payload_size);
    if (crc != candidate.payload_crc32)
        return 0;

    if (footer) {
        footer->magic = candidate.magic;
        footer->version = candidate.version;
        footer->image_kind = candidate.image_kind;
        footer->flags = candidate.flags;
        footer->load_addr = candidate.load_addr;
        footer->entry_addr = candidate.entry_addr;
        footer->payload_size = candidate.payload_size;
        footer->payload_crc32 = candidate.payload_crc32;
        footer->payload_crc32_inv = candidate.payload_crc32_inv;
        footer->tail_magic = candidate.tail_magic;
    }
    return 1;
}

void sg2002_copy_image(uintptr_t dst, uintptr_t src, size_t len, size_t *copied_len) {
    size_t n = 0;
    const uint8_t *s = (const uint8_t *)(uintptr_t)src;
    uint8_t *d = (uint8_t *)(uintptr_t)dst;

    while (n < len) {
        d[n] = s[n];
        ++n;
        if ((n & 63u) == 0u)
            flush_dcache_range(dst + n - 64u, dst + n);
    }
    flush_dcache_range(dst, dst + n);
    fence_i();
    if (copied_len) *copied_len = n;
}
