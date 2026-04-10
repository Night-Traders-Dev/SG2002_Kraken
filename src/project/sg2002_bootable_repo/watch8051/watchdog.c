#include <stdint.h>

/*
 * Shared control page fields mirrored into 8051 xdata offsets.
 * These offsets match the early words in shared_ctrl_t in include/kraken.h.
 * The AP-side boot code must map 8051 XDATA 0x0000 onto SHARED_CTRL_ADDR
 * before releasing reset; otherwise these mirrored offsets will not land on
 * the real shared control page. If shared_ctrl_t changes, keep the mirrored
 * offsets in sync.
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
 * This firmware does not issue direct MMIO resets. It is compiled for a small
 * XDATA window that mirrors the shared control page, not for arbitrary 32-bit
 * SoC register access. Until a documented 8051-side reset sequence is wired up,
 * watchdog escalation is signal-only: set SYSF_WATCHDOG_TIMEOUT, record the
 * reason, and stop making forward progress so the next reset mechanism can act.
 */
static void do_signal_reset(uint32_t reason) {
    reset_reason = reason;
    system_flags |= 0x80000000UL;  /* SYSF_WATCHDOG_TIMEOUT */

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
