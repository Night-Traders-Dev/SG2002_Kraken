#include "kraken.h"

static inline uint32_t user_led_mask(void) {
    return 1u << KRAKEN_USER_LED_PIN;
}

static void user_led_pulse_group(uint32_t pulses, uint32_t pulse_delay_cycles) {
#if KRAKEN_ENABLE_NANOW_USER_LED
    if (pulses == 0u)
        pulses = 1u;

    for (uint32_t i = 0; i < pulses; ++i) {
        sg2002_user_led_set(1);
        delay_cycles(pulse_delay_cycles);
        sg2002_user_led_set(0);
        delay_cycles(pulse_delay_cycles);
    }
    delay_cycles(KRAKEN_USER_LED_DIAG_GROUP_GAP_CYCLES);
#else
    (void)pulses;
    (void)pulse_delay_cycles;
#endif
}

static inline uint32_t sg2002_8051_window_base(uintptr_t addr) {
    return (uint32_t)(addr & ~0x7ffu);
}

uint32_t sg2002_platform_caps(void) {
    uint32_t caps = PLATCAP_RISCV_C906 |
                    PLATCAP_WORKER_RELEASE |
                    PLATCAP_FAULT_LOG |
                    PLATCAP_RISCV_TRAPS |
                    PLATCAP_RISCV_IDENTITY;
#if KRAKEN_ENABLE_USB_DWC2_SCAFFOLD
    caps |= PLATCAP_USB_DWC2_SCAFFOLD;
#endif
#if KRAKEN_ENABLE_WORKER_RESET_HOOK
    caps |= PLATCAP_WORKER_RESET_HOOK;
#endif
#if KRAKEN_ENABLE_PLATFORM_DCACHE_OPS
    caps |= PLATCAP_DCACHE_OPS;
#endif
#if WORKER_STAGING_ADDR != 0
    caps |= PLATCAP_WORKER_STAGING;
#endif
    return caps;
}

void sg2002_user_led_init(void) {
#if KRAKEN_ENABLE_NANOW_USER_LED
    MMIO32(SG2002_PINMUX_GPIOA14_REG) = KRAKEN_USER_LED_PINMUX_GPIO;
    MMIO32(SG2002_GPIO_SWPORTA_DDR) |= user_led_mask();
#endif
}

void sg2002_user_led_set(int on) {
#if KRAKEN_ENABLE_NANOW_USER_LED
    uint32_t dr;

    sg2002_user_led_init();
    dr = MMIO32(SG2002_GPIO_SWPORTA_DR);
    if (on)
        dr |= user_led_mask();
    else
        dr &= ~user_led_mask();
    MMIO32(SG2002_GPIO_SWPORTA_DR) = dr;
#else
    (void)on;
#endif
}

void sg2002_user_led_toggle(void) {
#if KRAKEN_ENABLE_NANOW_USER_LED
    sg2002_user_led_init();
    MMIO32(SG2002_GPIO_SWPORTA_DR) ^= user_led_mask();
#endif
}

void sg2002_user_led_blink(uint32_t pulses) {
#if KRAKEN_ENABLE_NANOW_USER_LED
    sg2002_user_led_set(0);
    for (uint32_t i = 0; i < pulses; ++i) {
        sg2002_user_led_set(1);
        delay_cycles(KRAKEN_USER_LED_BLINK_DELAY_CYCLES);
        sg2002_user_led_set(0);
        delay_cycles(KRAKEN_USER_LED_BLINK_DELAY_CYCLES);
    }
#else
    (void)pulses;
#endif
}

void sg2002_user_led_show_persist_summary(const kraken_persist_log_t *log) {
#if KRAKEN_ENABLE_NANOW_USER_LED
    uint32_t source;
    uint32_t detail;

    if (log == 0 ||
        log->magic != KRAKEN_PERSIST_MAGIC ||
        log->version != KRAKEN_PERSIST_VERSION ||
        log->last_boot_count == 0u)
        return;

    source = log->last_fault_tag != 0u ? log->last_fault_tag : log->last_trace_source;
    detail = log->last_fault_code != 0u ? log->last_fault_code : log->last_trace_code;

    /*
     * Replay the previous-boot summary as:
     *   3 pulses  -> summary marker
     *   N pulses  -> previous stage + 1
     *   N pulses  -> previous source + 1
     *   N pulses  -> low nibble of previous code + 1
     */
    user_led_pulse_group(3u, KRAKEN_USER_LED_DIAG_DELAY_CYCLES);
    user_led_pulse_group((log->last_stage & 0x0fu) + 1u,
                         KRAKEN_USER_LED_DIAG_DELAY_CYCLES);
    user_led_pulse_group((source & 0x0fu) + 1u,
                         KRAKEN_USER_LED_DIAG_DELAY_CYCLES);
    user_led_pulse_group((detail & 0x0fu) + 1u,
                         KRAKEN_USER_LED_DIAG_DELAY_CYCLES);
#else
    (void)log;
#endif
}

void sg2002_boot_8051(uintptr_t entry_addr) {
    uint32_t rst_ctrl;

    /* The 8051 needs two explicit mappings before it can run usefully:
     *   - external ROM fetches from the DDR blob at FW8051_DDR_ADDR
     *   - external RAM/XDATA window 0x0000.. maps onto SHARED_CTRL_ADDR
     * Both windows are 2 KiB aligned on SG200X. */
    MMIO32(SG2002_TOP_MISC_RTC2AP_REG) = SG2002_TOP_MISC_RTC2AP_ENABLE;

    rst_ctrl = MMIO32(SG2002_RTCSYS_RST_CTRL_REG);
    MMIO32(SG2002_RTCSYS_RST_CTRL_REG) =
        rst_ctrl & ~SG2002_RTCSYS_RST_CTRL_MCU51_BIT;

    MMIO32(SG2002_RTCSYS_MCU51_CTRL0_REG) =
        sg2002_8051_window_base(entry_addr) |
        SG2002_RTCSYS_MCU51_DDR_BOOT_FLAGS;
    MMIO32(SG2002_RTCSYS_MCU51_CTRL1_REG) =
        sg2002_8051_window_base(SHARED_CTRL_ADDR);

    MMIO32(SG2002_RTCSYS_RST_CTRL_REG) =
        rst_ctrl | SG2002_RTCSYS_RST_CTRL_MCU51_BIT;
    mailbox_send_8051(CMD_BOOT, (uint32_t)entry_addr);
}

void kraken_jump_to(uintptr_t entry_addr) {
    shared_ctrl_t *ctl = shared_ctrl();
    uintptr_t hartid = (uintptr_t)ctl->boot_hartid;
    uintptr_t dtb_addr = (uintptr_t)ctl->boot_dtb_addr;

    fence_rw();
    fence_i();
    ((kraken_entry_fn_t)(uintptr_t)entry_addr)(hartid, dtb_addr);
    __builtin_unreachable();
}

void sg2002_user_led_panic_loop(void) {
#if KRAKEN_ENABLE_NANOW_USER_LED
    sg2002_user_led_set(0);
    for (;;) {
        sg2002_user_led_toggle();
        delay_cycles(KRAKEN_USER_LED_BLINK_DELAY_CYCLES);
    }
#else
    for (;;) cpu_relax();
#endif
}
