#include "kraken.h"
#define MBOX_INFO0 0x20
#define MBOX_INFO1 0x24

void mailbox_send_worker(uint32_t cmd, uint32_t arg) {
    MMIO32(SG2002_AP_MAILBOX_BASE + MBOX_INFO0) = cmd;
    MMIO32(SG2002_AP_MAILBOX_BASE + MBOX_INFO1) = arg;
    fence_rw();
}

void mailbox_send_8051(uint32_t cmd, uint32_t arg) {
    MMIO32(SG2002_RTCSYS_MBOX_BASE + MBOX_INFO0) = cmd;
    MMIO32(SG2002_RTCSYS_MBOX_BASE + MBOX_INFO1) = arg;
    fence_rw();
}
