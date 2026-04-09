#ifndef KRAKEN_MINI_TUSB_CDC_DEVICE_H_
#define KRAKEN_MINI_TUSB_CDC_DEVICE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void tinyusb_cdc_device_init(void);
void tinyusb_cdc_device_task(void);

bool tinyusb_cdc_connected(void);
void tinyusb_cdc_set_connected(bool connected);
uint32_t tinyusb_cdc_available(void);
uint32_t tinyusb_cdc_read(void *buffer, uint32_t bufsize);
uint32_t tinyusb_cdc_write(const void *buffer, uint32_t bufsize);
void tinyusb_cdc_write_flush(void);

void tinyusb_cdc_rx_feed(const void *buffer, uint32_t bufsize);
uint32_t tinyusb_cdc_tx_take(void *buffer, uint32_t bufsize);

#ifdef __cplusplus
}
#endif

#endif
