#include "kraken.h"
#include "tusb.h"

static uint32_t ring_next(uint32_t idx) {
    return (idx + 1u) & (USB_SERIAL_RING_SIZE - 1u);
}

static void console_hex32_to_buf(uint32_t v, char out[8]) {
    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < 8; ++i) {
        out[7 - i] = hex[v & 0xfu];
        v >>= 4;
    }
}

static uint32_t tx_ring_used(const shared_ctrl_t *ctl) {
    if (ctl->usb_tx_head >= ctl->usb_tx_tail) return ctl->usb_tx_head - ctl->usb_tx_tail;
    return USB_SERIAL_RING_SIZE - (ctl->usb_tx_tail - ctl->usb_tx_head);
}

static uint32_t rx_ring_free(const shared_ctrl_t *ctl) {
    uint32_t used;
    if (ctl->usb_rx_head >= ctl->usb_rx_tail) used = ctl->usb_rx_head - ctl->usb_rx_tail;
    else used = USB_SERIAL_RING_SIZE - (ctl->usb_rx_tail - ctl->usb_rx_head);
    return (USB_SERIAL_RING_SIZE - 1u) - used;
}

static void pump_tx_to_tinyusb(shared_ctrl_t *ctl) {
    if (!tud_cdc_n_connected(0)) return;
    while (ctl->usb_tx_tail != ctl->usb_tx_head) {
        uint8_t chunk[32];
        uint32_t n = 0;
        while (n < sizeof(chunk) && ctl->usb_tx_tail != ctl->usb_tx_head) {
            chunk[n++] = ctl->usb_tx_ring[ctl->usb_tx_tail];
            ctl->usb_tx_tail = ring_next(ctl->usb_tx_tail);
        }
        if (tud_cdc_n_write(0, chunk, n) != n) break;
    }
    tud_cdc_n_write_flush(0);
}

static void pump_rx_from_tinyusb(shared_ctrl_t *ctl) {
    while (tud_cdc_n_available(0) != 0 && rx_ring_free(ctl) != 0) {
        uint8_t chunk[32];
        uint32_t take = tud_cdc_n_read(0, chunk, sizeof(chunk));
        if (take == 0) break;
        for (uint32_t i = 0; i < take; ++i) {
            uint32_t next = ring_next(ctl->usb_rx_head);
            if (next == ctl->usb_rx_tail) return;
            ctl->usb_rx_ring[ctl->usb_rx_head] = chunk[i];
            ctl->usb_rx_head = next;
        }
    }
}

void usb_serial_set_state(uint32_t state, uint32_t error_code) {
    shared_ctrl_t *ctl = shared_ctrl();
    ctl->usb_state = state;
    ctl->usb_last_error = error_code;
    ctl_flush(ctl);
}

int usb_serial_connected(void) {
    return tud_cdc_n_connected(0) ? 1 : 0;
}

void usb_serial_init(void) {
    shared_ctrl_t *ctl = shared_ctrl();
    ctl->usb_state = USB_SERIAL_INIT;
    ctl->usb_flags = USB_SERIAL_PROTO_ACM;
    ctl->usb_rx_head = 0;
    ctl->usb_rx_tail = 0;
    ctl->usb_tx_head = 0;
    ctl->usb_tx_tail = 0;
    ctl->usb_last_error = 0;
    ctl_flush(ctl);

    if (!tusb_init()) {
        ctl->system_flags |= SYSF_USB_FAULT;
        ctl_set_platform_error(ctl, PLATERR_USB_INIT_FAILED);
        usb_serial_set_state(USB_SERIAL_ERROR, 0x54555342u);
        return;
    }

    ctl->usb_state = USB_SERIAL_READY;
    ctl_clear_platform_error(ctl, PLATERR_USB_INIT_FAILED);
    ctl_flush(ctl);
}

void usb_serial_poll(void) {
    shared_ctrl_t *ctl = shared_ctrl();
    ctl_invalidate(ctl);
    if (!tud_inited()) return;

    tud_task();
    pump_rx_from_tinyusb(ctl);
    pump_tx_to_tinyusb(ctl);

    ctl->usb_state = tud_cdc_n_connected(0) ? USB_SERIAL_ACTIVE : USB_SERIAL_READY;
    ctl->usb_flags = USB_SERIAL_PROTO_ACM | (tx_ring_used(ctl) << 16);
    ctl_flush(ctl);
}

void usb_serial_write(const char *buf, size_t len) {
    shared_ctrl_t *ctl = shared_ctrl();
    if (!ctl->usb_console_enabled) return;
    if (!tud_cdc_n_connected(0)) return;
    ctl_invalidate(ctl);
    for (size_t i = 0; i < len; ++i) {
        uint32_t head = ctl->usb_tx_head;
        uint32_t next = ring_next(head);
        if (next == ctl->usb_tx_tail) break;
        ctl->usb_tx_ring[head] = (uint8_t)buf[i];
        ctl->usb_tx_head = next;
    }
    ctl_flush(ctl);
}

void usb_serial_write_cstr(const char *s) {
    usb_serial_write(s, sg2002_strnlen(s, 4096));
}

size_t usb_serial_read(char *buf, size_t len) {
    shared_ctrl_t *ctl = shared_ctrl();
    size_t out = 0;
    ctl_invalidate(ctl);
    while (out < len && ctl->usb_rx_tail != ctl->usb_rx_head) {
        uint32_t tail = ctl->usb_rx_tail;
        buf[out++] = (char)ctl->usb_rx_ring[tail];
        ctl->usb_rx_tail = ring_next(tail);
    }
    ctl_flush(ctl);
    return out;
}

void console_puts(const char *s) {
    uart_puts(s);
    usb_serial_write_cstr(s);
}

void console_puthex(uint32_t v) {
    char buf[8];
    console_hex32_to_buf(v, buf);
    uart_puthex(v);
    usb_serial_write(buf, sizeof(buf));
}

void console_puthex64(uint64_t v) {
    console_puthex((uint32_t)(v >> 32));
    console_puthex((uint32_t)(v & 0xffffffffu));
}

void tud_mount_cb(void) {
    usb_serial_set_state(USB_SERIAL_ACTIVE, 0);
}

void tud_umount_cb(void) {
    usb_serial_set_state(USB_SERIAL_READY, 0);
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
}

void tud_resume_cb(void) {}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    shared_ctrl_t *ctl = shared_ctrl();
    (void)itf;
    (void)rts;
    if (dtr) {
        usb_serial_set_state(USB_SERIAL_ACTIVE, 0);
        return;
    }

    /* Host released the ACM port. Drop any queued log bytes so a later
     * reconnect does not dump stale data into a fresh TinyUSB session. */
    ctl->usb_tx_head = ctl->usb_tx_tail;
    usb_serial_set_state(USB_SERIAL_READY, 0);
}

void tud_cdc_rx_cb(uint8_t itf) {
    (void)itf;
}
