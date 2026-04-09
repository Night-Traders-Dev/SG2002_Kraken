#include "kraken.h"

static inline uint32_t user_led_mask(void) {
    return 1u << KRAKEN_USER_LED_PIN;
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

void sg2002_boot_8051(uintptr_t entry_addr) {
    MMIO32(SG2002_RTC_CTRL_BASE + 0x028) = 0x5a5a0001u;
    MMIO32(SG2002_RTC_CTRL_BASE + 0x020) = (uint32_t)(entry_addr >> 11);
    MMIO32(SG2002_RTC_CTRL_BASE + 0x024) = 0;
    MMIO32(SG2002_RTC_CTRL_BASE + 0x02c) = 1;
    mailbox_send_8051(CMD_BOOT, (uint32_t)entry_addr);
}

void kraken_jump_to(uintptr_t entry_addr) {
    void (*entry)(void) = (void (*)(void))(uintptr_t)entry_addr;
    fence_rw();
    fence_i();
    entry();
    for (;;) cpu_relax();
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
