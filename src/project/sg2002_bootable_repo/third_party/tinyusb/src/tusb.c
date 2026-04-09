#include "class/cdc/cdc_device.h"
#include "tusb.h"

void tinyusb_dwc2_init(uint8_t rhport);
void tinyusb_dwc2_task(void);
bool tinyusb_dwc2_ready(void);

static bool s_tusb_inited;

bool tusb_init(void) {
    if (s_tusb_inited) return true;
    tinyusb_cdc_device_init();
    tinyusb_dwc2_init(0);
    s_tusb_inited = tinyusb_dwc2_ready();
    return s_tusb_inited;
}

bool tud_inited(void) {
    return s_tusb_inited;
}

void tud_task(void) {
    if (!s_tusb_inited) return;
    tinyusb_dwc2_task();
    tinyusb_cdc_device_task();
}

bool tud_cdc_n_connected(uint8_t itf) {
    (void)itf;
    return tinyusb_cdc_connected();
}

uint32_t tud_cdc_n_available(uint8_t itf) {
    (void)itf;
    return tinyusb_cdc_available();
}

uint32_t tud_cdc_n_read(uint8_t itf, void *buffer, uint32_t bufsize) {
    (void)itf;
    return tinyusb_cdc_read(buffer, bufsize);
}

uint32_t tud_cdc_n_write(uint8_t itf, const void *buffer, uint32_t bufsize) {
    (void)itf;
    return tinyusb_cdc_write(buffer, bufsize);
}

void tud_cdc_n_write_flush(uint8_t itf) {
    (void)itf;
    tinyusb_cdc_write_flush();
}

void __attribute__((weak)) tud_mount_cb(void) {}
void __attribute__((weak)) tud_umount_cb(void) {}
void __attribute__((weak)) tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; }
void __attribute__((weak)) tud_resume_cb(void) {}
void __attribute__((weak)) tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void)itf; (void)dtr; (void)rts;
}
void __attribute__((weak)) tud_cdc_rx_cb(uint8_t itf) { (void)itf; }
