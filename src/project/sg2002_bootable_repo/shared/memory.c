#include "kraken.h"

void sg2002_memcpy(void *dst, const void *src, size_t len) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (len--) *d++ = *s++;
}

void sg2002_memset(void *dst, int c, size_t len) {
    uint8_t *d = (uint8_t *)dst;
    uint8_t v = (uint8_t)c;
    while (len--) *d++ = v;
}

int sg2002_memcmp(const void *a, const void *b, size_t len) {
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    while (len--) {
        if (*pa != *pb) return (int)*pa - (int)*pb;
        ++pa;
        ++pb;
    }
    return 0;
}

size_t sg2002_strnlen(const char *s, size_t max_len) {
    size_t n = 0;
    while (n < max_len && s[n]) ++n;
    return n;
}
