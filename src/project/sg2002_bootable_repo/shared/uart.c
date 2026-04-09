#include "kraken.h"
#define UART_THR 0x00
#define UART_LSR 0x14
#define UART_LSR_THRE (1u << 5)

static void uart_putc(char c) {
    while ((MMIO32(SG2002_UART0_BASE + UART_LSR) & UART_LSR_THRE) == 0) {}
    MMIO32(SG2002_UART0_BASE + UART_THR) = (uint32_t)c;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

void uart_puthex(uint32_t v) {
    static const char hex[] = "0123456789abcdef";
    for (int i = 7; i >= 0; --i) uart_putc(hex[(v >> (i * 4)) & 0xf]);
}

void uart_puthex64(uint64_t v) {
    uart_puthex((uint32_t)(v >> 32));
    uart_puthex((uint32_t)(v & 0xffffffffu));
}
