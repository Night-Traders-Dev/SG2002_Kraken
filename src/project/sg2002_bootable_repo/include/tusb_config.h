#ifndef TUSB_CONFIG_H_
#define TUSB_CONFIG_H_

#define OPT_MCU_SG2002_DWC2  0x2002
#define OPT_OS_NONE          1
#define OPT_MODE_DEVICE      0x01
#define OPT_MODE_FULL_SPEED  0x10

#define CFG_TUSB_MCU               OPT_MCU_SG2002_DWC2
#define CFG_TUSB_OS                OPT_OS_NONE
#define CFG_TUSB_DEBUG             0
#define CFG_TUD_ENABLED            1
#define CFG_TUD_CDC                1
#define CFG_TUD_CDC_RX_BUFSIZE     256
#define CFG_TUD_CDC_TX_BUFSIZE     256
#define CFG_TUSB_RHPORT0_MODE      (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

#endif
