#ifndef KRAKEN_MINI_TUSB_H_
#define KRAKEN_MINI_TUSB_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "tusb_config.h"

#ifdef __cplusplus
extern "C" {
#endif

bool tusb_init(void);
bool tud_inited(void);
void tud_task(void);

bool tud_cdc_n_connected(uint8_t itf);
uint32_t tud_cdc_n_available(uint8_t itf);
uint32_t tud_cdc_n_read(uint8_t itf, void *buffer, uint32_t bufsize);
uint32_t tud_cdc_n_write(uint8_t itf, const void *buffer, uint32_t bufsize);
void tud_cdc_n_write_flush(uint8_t itf);

void dcd_int_handler(uint8_t rhport);

void tud_mount_cb(void);
void tud_umount_cb(void);
void tud_suspend_cb(bool remote_wakeup_en);
void tud_resume_cb(void);
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts);
void tud_cdc_rx_cb(uint8_t itf);

const uint8_t *tud_descriptor_device_cb(void);
const uint8_t *tud_descriptor_configuration_cb(uint8_t index);
const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid);

#ifdef __cplusplus
}
#endif

#endif
