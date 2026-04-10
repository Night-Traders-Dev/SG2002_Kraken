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

static void rtc_ctrl0_unlock(void) {
    MMIO32(SG2002_RTC_CTRL0_UNLOCK_REG) = SG2002_RTC_CTRL0_UNLOCK_KEY;
}

static void sg2002_8051_power_on_domain(void) {
    uint32_t ip_pwr_req = MMIO32(SG2002_RTCSYS_IP_PWR_REQ_REG);
    uint32_t iso_ctrl = MMIO32(SG2002_RTCSYS_IP_ISO_CTRL_REG);

    ip_pwr_req |= SG2002_RTCSYS_IP_PWR_REQ_MCU_BIT;
    MMIO32(SG2002_RTCSYS_IP_PWR_REQ_REG) = ip_pwr_req;
    for (uint32_t i = 0; i < 100000u; ++i) {
        if ((MMIO32(SG2002_RTCSYS_IP_PWR_REQ_REG) &
             SG2002_RTCSYS_IP_PWR_ACK_MCU_BIT) != 0u)
            break;
        cpu_relax();
    }

    iso_ctrl &= ~SG2002_RTCSYS_IP_ISO_CTRL_MCU_BIT;
    MMIO32(SG2002_RTCSYS_IP_ISO_CTRL_REG) = iso_ctrl;
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
    uint32_t ctrl0;
    uint32_t ctrl1;

    /* SG2002 TRM Chapter 14 / RTCSYS chapter:
     *   1. power on the MCU domain and clear isolation
     *   2. hold the 8051 in reset
     *   3. program the external-ROM and XDATA windows
     *   4. release reset
     *
     * We keep the vendor-observed DDR boot contract, but model the fields
     * explicitly instead of OR-ing in an unexplained 0x84 constant.
     */
    sg2002_8051_power_on_domain();

    MMIO32(SG2002_RTCSYS_PMU_REG) |= SG2002_RTCSYS_PMU_CLK25M_REQ_BIT;
    MMIO32(SG2002_RTCSYS_PMU2_REG) |= SG2002_RTCSYS_PMU2_WKINT_DB_EN_BIT;

    rst_ctrl = MMIO32(SG2002_RTCSYS_RST_CTRL_REG);
    rst_ctrl |= SG2002_RTCSYS_RST_CTRL_RTC2AP_BIT;
    MMIO32(SG2002_RTCSYS_RST_CTRL_REG) =
        rst_ctrl & ~SG2002_RTCSYS_RST_CTRL_MCU51_BIT;

    ctrl0 = sg2002_8051_window_base(entry_addr);
    ctrl0 |= (SG2002_RTCSYS_MCU51_ROM_ADDR_SIZE_DDR &
              SG2002_RTCSYS_MCU51_ROM_ADDR_SIZE_MASK)
             << SG2002_RTCSYS_MCU51_ROM_ADDR_SIZE_SHIFT;
    ctrl0 |= SG2002_RTCSYS_MCU51_ROM_ADDR_DEF_BIT;
    ctrl1 = sg2002_8051_window_base(SHARED_CTRL_ADDR);

    MMIO32(SG2002_RTCSYS_MCU51_CTRL0_REG) = ctrl0;
    MMIO32(SG2002_RTCSYS_MCU51_CTRL1_REG) = ctrl1;

    MMIO32(SG2002_RTCSYS_RST_CTRL_REG) =
        rst_ctrl | SG2002_RTCSYS_RST_CTRL_MCU51_BIT;
    mailbox_send_8051(CMD_BOOT, (uint32_t)entry_addr);
}

void sg2002_request_watchdog_reset(void) {
    MMIO32(SG2002_RTCSYS_POR_RST_CTRL_REG) |=
        SG2002_RTCSYS_POR_RST_CTRL_RTCSYS_RESET_EN_BIT;
    MMIO32(SG2002_RTC_EN_WDG_RST_REQ_REG) |= SG2002_RTC_EN_WDG_RST_REQ_BIT;
    rtc_ctrl0_unlock();
    MMIO32(SG2002_RTC_CTRL0_REG) = SG2002_RTC_CTRL0_REQ_SW_WDG_RST;
    fence_rw();
    for (;;) cpu_relax();
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
