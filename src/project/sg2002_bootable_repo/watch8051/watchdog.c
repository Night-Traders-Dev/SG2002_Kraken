#include <stdint.h>

/*
 * Shared control page fields mirrored into 8051 xdata offsets.
 * These offsets match the early words in shared_ctrl_t in include/kraken.h.
 * If shared_ctrl_t changes, keep the mirrored offsets in sync.
 */
__xdata __at (0x0008) volatile uint32_t system_stage;
__xdata __at (0x000c) volatile uint32_t system_flags;
__xdata __at (0x0010) volatile uint32_t reset_reason;
__xdata __at (0x0020) volatile uint32_t kernel_heartbeat;
__xdata __at (0x0024) volatile uint32_t worker_heartbeat;
__xdata __at (0x0028) volatile uint32_t worker_state;
__xdata __at (0x0030) volatile uint32_t kernel_pet_seq;
__xdata __at (0x0034) volatile uint32_t watchdog_last_kernel_seq;
__xdata __at (0x0038) volatile uint32_t watchdog_last_worker_seq;
__xdata __at (0x003c) volatile uint32_t watchdog_pet_count;

static void pet_hw(void) {
    watchdog_pet_count++;
}

/*
 * Signal a watchdog reset request to the C906 side.
 *
 * This path does not directly drive a hardware reset line yet. It depends on
 * the vendor watchdog or the C906 kernel turning SYSF_WATCHDOG_TIMEOUT into a
 * real reset. Until the hardware watchdog path is wired up, this loop only
 * keeps the 8051 alive while the SoC waits for the next reset mechanism.
 */

/*
 * do_signal_reset: Signal to the C906 kernel that a watchdog reset is needed
 * by setting SYSF_WATCHDOG_TIMEOUT in system_flags and recording the reason.
 *
 * This function now also writes to the hardware RTCSYS WDT to force a real
 * system reset after signaling the kernel.
 *
 * The RTCSYS WDT is armed by the vendor FSBL. When kernel_pet_seq stops advancing,
 * the WDT will fire and reset the SoC.
 */
static void do_signal_reset(uint32_t reason) {
    reset_reason = reason;
    system_flags |= 0x80000000UL;  /* SYSF_WATCHDOG_TIMEOUT */

    /* Feed the hardware RTCSYS WDT before final reset */
    MMIO32(SG2002_RTCSYS_WDT_BASE + 0x00) = 0x1999u;  /* WDT_CR - feed */
    MMIO32(SG2002_RTCSYS_WDT_BASE + 0x04) = 0x0666u;  /* WDT_TORR - reload */

    /* Trigger hardware reset */
    MMIO32(SG2002_RTC_SOFT_RSTN) = 0u;  /* Full system warm reset */

    for (;;) {
        pet_hw();
    }
}

    reset_reason = reason;
    system_flags |= 0x80000000UL;
    for (;;) {
        pet_hw();
    }
}

void main(void) {
    uint32_t last_pet = kernel_pet_seq;
    uint32_t last_worker = worker_heartbeat;
    uint16_t kernel_timeout = 0;
    uint16_t worker_timeout = 0;

    watchdog_last_kernel_seq = last_pet;
    watchdog_last_worker_seq = last_worker;
    watchdog_pet_count = 0;

    while (1) {
        if (kernel_pet_seq != last_pet) {
            last_pet = kernel_pet_seq;
            watchdog_last_kernel_seq = last_pet;
            kernel_timeout = 0;
            pet_hw();
        } else if (++kernel_timeout > 60000u) {
            do_signal_reset(0x80510001UL);
        }

        if (worker_heartbeat != last_worker) {
            last_worker = worker_heartbeat;
            watchdog_last_worker_seq = last_worker;
            worker_timeout = 0;
        } else if (worker_state == 3u && ++worker_timeout > 60000u) {
            do_signal_reset(0x80510002UL);
        }
    }
}
