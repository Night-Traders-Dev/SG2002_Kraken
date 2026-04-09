#include "class/cdc/cdc_device.h"
#include "kraken.h"
#include "tusb_config.h"

#define CDC_RX_SIZE CFG_TUD_CDC_RX_BUFSIZE
#define CDC_TX_SIZE CFG_TUD_CDC_TX_BUFSIZE

typedef struct {
    uint8_t data[CDC_RX_SIZE];
    uint32_t head;
    uint32_t tail;
} cdc_rx_ring_t;

typedef struct {
    uint8_t data[CDC_TX_SIZE];
    uint32_t head;
    uint32_t tail;
} cdc_tx_ring_t;

static cdc_rx_ring_t s_rx;
static cdc_tx_ring_t s_tx;
static bool s_connected;

static uint32_t ring_next(uint32_t idx, uint32_t size) {
    return (idx + 1u) % size;
}

void tinyusb_cdc_device_init(void) {
    sg2002_memset(&s_rx, 0, sizeof(s_rx));
    sg2002_memset(&s_tx, 0, sizeof(s_tx));
    s_connected = false;
}

void tinyusb_cdc_device_task(void) {
    /*
     * Scaffold only. A real TinyUSB integration would advance EP0 and class
     * requests here via the upstream usbd task machinery.
     */
}

bool tinyusb_cdc_connected(void) {
    return s_connected;
}

void tinyusb_cdc_set_connected(bool connected) {
    s_connected = connected;
}

uint32_t tinyusb_cdc_available(void) {
    if (s_rx.head >= s_rx.tail) return s_rx.head - s_rx.tail;
    return CDC_RX_SIZE - (s_rx.tail - s_rx.head);
}

uint32_t tinyusb_cdc_read(void *buffer, uint32_t bufsize) {
    uint8_t *out = (uint8_t *)buffer;
    uint32_t n = 0;
    while (n < bufsize && s_rx.tail != s_rx.head) {
        out[n++] = s_rx.data[s_rx.tail];
        s_rx.tail = ring_next(s_rx.tail, CDC_RX_SIZE);
    }
    return n;
}

uint32_t tinyusb_cdc_write(const void *buffer, uint32_t bufsize) {
    const uint8_t *in = (const uint8_t *)buffer;
    uint32_t n = 0;
    while (n < bufsize) {
        uint32_t next = ring_next(s_tx.head, CDC_TX_SIZE);
        if (next == s_tx.tail) break;
        s_tx.data[s_tx.head] = in[n++];
        s_tx.head = next;
    }
    return n;
}

void tinyusb_cdc_write_flush(void) {
    /*
     * Scaffold only. The DWC2 endpoint backend will drain the TX FIFO once
     * transfer scheduling is implemented.
     */
}

void tinyusb_cdc_rx_feed(const void *buffer, uint32_t bufsize) {
    const uint8_t *in = (const uint8_t *)buffer;
    for (uint32_t i = 0; i < bufsize; ++i) {
        uint32_t next = ring_next(s_rx.head, CDC_RX_SIZE);
        if (next == s_rx.tail) break;
        s_rx.data[s_rx.head] = in[i];
        s_rx.head = next;
    }
}

uint32_t tinyusb_cdc_tx_take(void *buffer, uint32_t bufsize) {
    uint8_t *out = (uint8_t *)buffer;
    uint32_t n = 0;
    while (n < bufsize && s_tx.tail != s_tx.head) {
        out[n++] = s_tx.data[s_tx.tail];
        s_tx.tail = ring_next(s_tx.tail, CDC_TX_SIZE);
    }
    return n;
}
