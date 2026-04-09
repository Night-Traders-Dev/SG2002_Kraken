#ifndef KRAKEN_SG2002_H
#define KRAKEN_SG2002_H

#include <stdint.h>
#include <stddef.h>

#define MMIO8(addr)  (*(volatile uint8_t  *)(uintptr_t)(addr))
#define MMIO16(addr) (*(volatile uint16_t *)(uintptr_t)(addr))
#define MMIO32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))
#define MMIO64(addr) (*(volatile uint64_t *)(uintptr_t)(addr))

#define SG2002_DDR_BASE               0x80000000ull
#define BOOTLOADER_LOAD_ADDR          0x80080000ull
#define KERNEL_LOAD_ADDR              0x80100000ull
#define SHARED_CTRL_ADDR              0x80170000ull
#define WORKER_LOAD_ADDR              0x80180000ull
#define WORKER_SHARED_TOP             0x801BFFFFull
#define FW8051_DDR_ADDR               0x83F80000ull

#define SG2002_UART0_BASE             0x04140000ull
#define SG2002_AP_MAILBOX_BASE        0x03050000ull
#define SG2002_RTC_CTRL_BASE          0x05025000ull
#define SG2002_RTCSYS_MBOX_BASE       0x05029000ull
#define SG2002_RTCSYS_WDT_BASE        0x0502D000ull

#define SG2002_TOP_MISC_BASE          0x03000000ull
#define SG2002_GPIO0_BASE             0x03020000ull
#define SG2002_SYS_C906L_CTRL_REG     (SG2002_TOP_MISC_BASE + 0x04)
#define SG2002_SYS_C906L_BOOTADDR_LO  (SG2002_TOP_MISC_BASE + 0x20)
#define SG2002_SYS_C906L_BOOTADDR_HI  (SG2002_TOP_MISC_BASE + 0x24)
#define SG2002_SYS_C906L_CTRL_EN      (1u << 13)
#define SG200X_RESET_BASE             0x03003000ull
#define SG200X_SOFT_CPU_RSTN_REG      (SG200X_RESET_BASE + 0x24)
#define SG200X_RST_CPUSYS2_BIT        6u

#define SG2002_USB_OTG_BASE           0x040C0000ull
#define SG2002_USB_HOST_BASE          0x040D0000ull
#define SG2002_USB_DEV_BASE           0x040E0000ull
#define SG2002_USB_DWC2_BASE          0x04340000ull
#define SG2002_USB_CTRL0_REG          (SG2002_TOP_MISC_BASE + 0x38)
#define SG2002_USB_PHY_CTRL_REG       (SG2002_TOP_MISC_BASE + 0x48)
#define SG2002_USB_CTRL0_DEVICE_MODE  (1u << 0)
#define SG2002_USB_PHY_CTRL_ENABLE    (1u << 0)
#define SG2002_USB_PHY_CTRL_PLL_EN    (1u << 1)
#define SG2002_GPIO_SWPORTA_DR        (SG2002_GPIO0_BASE + 0x00)
#define SG2002_GPIO_SWPORTA_DDR       (SG2002_GPIO0_BASE + 0x04)
#define SG2002_PINMUX_GPIOA14_REG     (SG2002_TOP_MISC_BASE + 0x1038)
#ifndef SG2002_USB_DWC2_IRQ
#define SG2002_USB_DWC2_IRQ           14u
#endif

#define KRAKEN_MAGIC                  0x4B52414Bu
#define KRAKEN_VERSION                6u
#define KRAKEN_STAGED_IMAGE_MAGIC     0x4B535447u
#define KRAKEN_STAGED_IMAGE_TAIL      0x4B454E44u
#define KRAKEN_STAGED_IMAGE_VERSION   1u
#define KRAKEN_BOOT_CYCLES_PER_MS     850000u
#define WORKER_ACK_TIMEOUT_MS         50u
#define WORKER_ACK_SPINS \
    ((WORKER_ACK_TIMEOUT_MS * KRAKEN_BOOT_CYCLES_PER_MS) / 64u)
#define WORKER_STALE_TIMEOUT_MS       100u
#define WORKER_STALE_SPINS \
    ((WORKER_STALE_TIMEOUT_MS * KRAKEN_BOOT_CYCLES_PER_MS) / 1000u)
#define WORKER_IMAGE_PROBE_WORDS      8u
#define KRAKEN_FAULT_LOG_SIZE         16u
#define USB_SERIAL_RING_SIZE          512u
#define USB_SERIAL_PROTO_ACM          1u
#define KRAKEN_STRLIT_BYTES(lit)      (sizeof(lit))
#define KRAKEN_STRLIT_LEN(lit)        (sizeof(lit) - 1u)

#ifndef WORKER_STAGING_ADDR
#define WORKER_STAGING_ADDR           0x00000000ull
#endif
#ifndef WORKER_IMAGE_MAX_BYTES
#define WORKER_IMAGE_MAX_BYTES        (128u * 1024u)
#endif
#ifndef KRAKEN_ENABLE_PLATFORM_DCACHE_OPS
#define KRAKEN_ENABLE_PLATFORM_DCACHE_OPS 0
#endif
#ifndef KRAKEN_ENABLE_USB_DWC2_SCAFFOLD
#define KRAKEN_ENABLE_USB_DWC2_SCAFFOLD 0
#endif
#ifndef KRAKEN_ENABLE_WORKER_RESET_HOOK
#define KRAKEN_ENABLE_WORKER_RESET_HOOK 1
#endif
#ifndef KRAKEN_ENABLE_NANOW_USER_LED
#define KRAKEN_ENABLE_NANOW_USER_LED  1
#endif
#ifndef KRAKEN_WORKER_RESET_REG
#define KRAKEN_WORKER_RESET_REG       SG200X_SOFT_CPU_RSTN_REG
#endif
#ifndef KRAKEN_WORKER_RESET_BIT
#define KRAKEN_WORKER_RESET_BIT       SG200X_RST_CPUSYS2_BIT
#endif
#ifndef KRAKEN_WORKER_RESET_PULSE_CYCLES
#define KRAKEN_WORKER_RESET_PULSE_CYCLES 256u
#endif
#ifndef KRAKEN_USER_LED_PIN
#define KRAKEN_USER_LED_PIN           14u
#endif
#ifndef KRAKEN_USER_LED_PINMUX_GPIO
#define KRAKEN_USER_LED_PINMUX_GPIO   0x3u
#endif
#ifndef KRAKEN_USER_LED_BLINK_DELAY_CYCLES
#define KRAKEN_USER_LED_BLINK_DELAY_CYCLES 40000000u
#endif

enum core_state {
    CORE_OFFLINE = 0,
    CORE_BOOTING = 1,
    CORE_IDLE    = 2,
    CORE_RUNNING = 3,
    CORE_FAULT   = 4,
};

enum system_stage {
    STAGE_COLD_RESET      = 0,
    STAGE_BOOTLOADER      = 1,
    STAGE_WATCHDOG_BOOT   = 2,
    STAGE_KERNEL_ENTRY    = 3,
    STAGE_WORKER_PREP     = 4,
    STAGE_WORKER_RELEASE  = 5,
    STAGE_WORKER_WAIT_ACK = 6,
    STAGE_OS_RUNNING      = 7,
    STAGE_PANIC           = 8,
};

enum cmd_id {
    CMD_NONE      = 0,
    CMD_BOOT      = 1,
    CMD_RUN_JOB   = 2,
    CMD_CANCEL    = 3,
    CMD_STOP      = 4,
    CMD_PET_8051  = 5,
    CMD_PANIC     = 6,
};

enum usb_serial_state {
    USB_SERIAL_OFF     = 0,
    USB_SERIAL_INIT    = 1,
    USB_SERIAL_READY   = 2,
    USB_SERIAL_ACTIVE  = 3,
    USB_SERIAL_ERROR   = 4,
};

enum system_flags {
    SYSF_NONE                 = 0,
    SYSF_BOOTLOADER_ACTIVE    = 1u << 0,
    SYSF_KERNEL_ACTIVE        = 1u << 1,
    SYSF_WORKER_IMAGE_MISSING = 1u << 2,
    SYSF_KERNEL_IMAGE_MISSING = 1u << 3,
    SYSF_WORKER_STALE         = 1u << 4,
    SYSF_WORKER_RESTARTING    = 1u << 5,
    SYSF_USB_FAULT            = 1u << 6,
    SYSF_RISCV_TRAP           = 1u << 7,
    SYSF_WORKER_RELEASE_FAIL  = 1u << 8,
    SYSF_WATCHDOG_TIMEOUT     = 1u << 31,
};

enum kraken_fault_source {
    FAULTSRC_BOOTLOADER = 1,
    FAULTSRC_KERNEL     = 2,
    FAULTSRC_WORKER     = 3,
    FAULTSRC_WATCHDOG   = 4,
};

enum kraken_platform_caps {
    PLATCAP_RISCV_C906        = 1u << 0,
    PLATCAP_WORKER_RELEASE    = 1u << 1,
    PLATCAP_WORKER_RESET_HOOK = 1u << 2,
    PLATCAP_DCACHE_OPS        = 1u << 3,
    PLATCAP_USB_DWC2_SCAFFOLD = 1u << 4,
    PLATCAP_FAULT_LOG         = 1u << 5,
    PLATCAP_WORKER_STAGING    = 1u << 6,
    PLATCAP_RISCV_TRAPS       = 1u << 7,
    PLATCAP_RISCV_IDENTITY    = 1u << 8,
};

enum kraken_platform_errors {
    PLATERR_NONE                   = 0,
    PLATERR_WORKER_STAGING_INVALID = 1u << 0,
    PLATERR_USB_INIT_FAILED        = 1u << 1,
    PLATERR_WORKER_ACK_TIMEOUT     = 1u << 2,
    PLATERR_WORKER_STALE           = 1u << 3,
    PLATERR_WORKER_RELEASE_FAILED  = 1u << 4,
};

enum sg2002_worker_release_status {
    SG2002_WORKER_RELEASE_OK = 0,
    SG2002_WORKER_RELEASE_BOOTADDR_LO_MISMATCH = 1,
    SG2002_WORKER_RELEASE_BOOTADDR_HI_MISMATCH = 2,
    SG2002_WORKER_RELEASE_ENABLE_LATCH_FAILED  = 3,
    SG2002_WORKER_RELEASE_RESET_HOOK_FAILED    = 4,
};

enum kraken_image_kind {
    KRAKEN_IMAGE_BOOTLOADER = 1,
    KRAKEN_IMAGE_KERNEL     = 2,
    KRAKEN_IMAGE_WORKER     = 3,
    KRAKEN_IMAGE_WATCHDOG   = 4,
};

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t image_kind;
    uint32_t flags;
    uint32_t load_addr;
    uint32_t entry_addr;
    uint32_t payload_size;
    uint32_t payload_crc32;
    uint32_t payload_crc32_inv;
    uint32_t tail_magic;
} kraken_staged_image_footer_t;

typedef struct {
    volatile uint32_t tag;
    volatile uint32_t code;
    volatile uint32_t arg0;
    volatile uint32_t arg1;
} kraken_fault_record_t;

enum kraken_riscv_identity_slot {
    RISCV_ID_BOOTLOADER = 0,
    RISCV_ID_KERNEL     = 1,
    RISCV_ID_WORKER     = 2,
    RISCV_IDENTITY_SLOTS = 3,
};

typedef struct {
    volatile uint32_t hartid;
    volatile uint32_t mvendorid;
    volatile uint32_t marchid;
    volatile uint32_t mimpid;
    volatile uint32_t misa;
} kraken_riscv_identity_t;

typedef struct {
    volatile uint32_t magic;                    /* 0x00 */
    volatile uint32_t version;                  /* 0x04 */
    volatile uint32_t system_stage;             /* 0x08 */
    volatile uint32_t system_flags;             /* 0x0c */
    volatile uint32_t reset_reason;             /* 0x10 */
    volatile uint32_t boot_count;               /* 0x14 */
    volatile uint32_t kernel_entry_addr;        /* 0x18 */
    volatile uint32_t worker_entry_addr;        /* 0x1c */
    volatile uint32_t kernel_heartbeat;         /* 0x20 */
    volatile uint32_t worker_heartbeat;         /* 0x24 */
    volatile uint32_t worker_state;             /* 0x28 */
    volatile uint32_t worker_restart_count;     /* 0x2c */
    volatile uint32_t kernel_pet_seq;           /* 0x30 */
    volatile uint32_t watchdog_last_kernel_seq; /* 0x34 */
    volatile uint32_t watchdog_last_worker_seq; /* 0x38 */
    volatile uint32_t watchdog_pet_count;       /* 0x3c */
    volatile uint32_t worker_cmd;               /* 0x40 */
    volatile uint32_t worker_arg0;              /* 0x44 */
    volatile uint32_t worker_arg1;              /* 0x48 */
    volatile uint32_t worker_result;            /* 0x4c */
    volatile uint32_t kernel_cmd_seq;           /* 0x50 */
    volatile uint32_t worker_cmd_ack;           /* 0x54 */
    volatile uint32_t worker_boot_ack;          /* 0x58 */
    volatile uint32_t usb_state;                /* 0x5c */
    volatile uint32_t usb_flags;                /* 0x60 */
    volatile uint32_t usb_rx_head;              /* 0x64 */
    volatile uint32_t usb_rx_tail;              /* 0x68 */
    volatile uint32_t usb_tx_head;              /* 0x6c */
    volatile uint32_t usb_tx_tail;              /* 0x70 */
    volatile uint32_t usb_last_error;           /* 0x74 */
    volatile uint32_t usb_console_enabled;      /* 0x78 */
    volatile uint32_t platform_caps;            /* 0x7c */
    volatile uint32_t platform_errors;          /* 0x80 */
    volatile uint32_t fault_log_head;           /* 0x84 */
    volatile uint32_t fault_log_count;          /* 0x88 */
    volatile uint32_t fault_last_tag;           /* 0x8c */
    volatile uint32_t fault_last_code;          /* 0x90 */
    volatile uint32_t fault_last_arg0;          /* 0x94 */
    volatile uint32_t fault_last_arg1;          /* 0x98 */
    volatile uint32_t trap_count;               /* 0x9c */
    volatile uint32_t trap_last_source;         /* 0xa0 */
    volatile uint32_t trap_last_cause;          /* 0xa4 */
    volatile uint32_t trap_last_epc;            /* 0xa8 */
    volatile uint32_t trap_last_tval;           /* 0xac */
    volatile uint32_t trap_last_status;         /* 0xb0 */
    volatile uint32_t boot_hartid;              /* 0xb4 */
    volatile uint32_t boot_dtb_addr;            /* 0xb8 */
    volatile kraken_riscv_identity_t riscv_identity[RISCV_IDENTITY_SLOTS]; /* 0xbc */
    volatile kraken_fault_record_t fault_log[KRAKEN_FAULT_LOG_SIZE];        /* 0x0f8 */
    volatile uint8_t usb_rx_ring[USB_SERIAL_RING_SIZE];                     /* 0x1f8 */
    volatile uint8_t usb_tx_ring[USB_SERIAL_RING_SIZE];                     /* 0x3f8 */
} shared_ctrl_t;

_Static_assert(offsetof(shared_ctrl_t, system_stage) == 0x08,
               "8051 xdata offset mismatch: system_stage");
_Static_assert(offsetof(shared_ctrl_t, system_flags) == 0x0c,
               "8051 xdata offset mismatch: system_flags");
_Static_assert(offsetof(shared_ctrl_t, reset_reason) == 0x10,
               "8051 xdata offset mismatch: reset_reason");
_Static_assert(offsetof(shared_ctrl_t, kernel_heartbeat) == 0x20,
               "8051 xdata offset mismatch: kernel_heartbeat");
_Static_assert(offsetof(shared_ctrl_t, worker_heartbeat) == 0x24,
               "8051 xdata offset mismatch: worker_heartbeat");
_Static_assert(offsetof(shared_ctrl_t, worker_state) == 0x28,
               "8051 xdata offset mismatch: worker_state");
_Static_assert(offsetof(shared_ctrl_t, kernel_pet_seq) == 0x30,
               "8051 xdata offset mismatch: kernel_pet_seq");
_Static_assert(offsetof(shared_ctrl_t, watchdog_last_kernel_seq) == 0x34,
               "8051 xdata offset mismatch: watchdog_last_kernel_seq");
_Static_assert(offsetof(shared_ctrl_t, watchdog_last_worker_seq) == 0x38,
               "8051 xdata offset mismatch: watchdog_last_worker_seq");
_Static_assert(offsetof(shared_ctrl_t, watchdog_pet_count) == 0x3c,
               "8051 xdata offset mismatch: watchdog_pet_count");
_Static_assert(sizeof(shared_ctrl_t) <= (WORKER_LOAD_ADDR - SHARED_CTRL_ADDR),
               "shared_ctrl_t overflows its reserved DDR region");

static inline shared_ctrl_t *shared_ctrl(void) {
    return (shared_ctrl_t *)(uintptr_t)SHARED_CTRL_ADDR;
}

static inline void fence_rw(void) {
    __asm__ volatile ("fence rw,rw" ::: "memory");
}

static inline void fence_i(void) {
    /*
     * Encode fence.i directly so older toolchains that default to
     * -march=rv64imac still assemble this header without requiring the
     * zifencei extension in the global ISA string.
     */
    __asm__ volatile (".word 0x0000100f" ::: "memory");
}

static inline void cpu_relax(void) {
    __asm__ volatile ("nop");
}

void flush_dcache_range(uintptr_t start, uintptr_t end);
void inval_dcache_range(uintptr_t start, uintptr_t end);
void flush_all_caches(void);

void mailbox_send_worker(uint32_t cmd, uint32_t arg);
void mailbox_send_8051(uint32_t cmd, uint32_t arg);

void uart_puts(const char *s);
void uart_puthex(uint32_t v);
void uart_puthex64(uint64_t v);
void delay_cycles(volatile uint32_t n);

void sg2002_memcpy(void *dst, const void *src, size_t len);
void sg2002_memset(void *dst, int c, size_t len);
int sg2002_memcmp(const void *a, const void *b, size_t len);
size_t sg2002_strnlen(const char *s, size_t max_len);

void ctl_flush(shared_ctrl_t *ctl);
void ctl_invalidate(shared_ctrl_t *ctl);
void ctl_init_defaults(shared_ctrl_t *ctl);
void ctl_set_stage(shared_ctrl_t *ctl, uint32_t stage);
uint32_t ctl_next_cmd_seq(shared_ctrl_t *ctl);
void ctl_note_boot_abi(shared_ctrl_t *ctl, uint32_t hartid, uintptr_t dtb_addr);
void ctl_fault_log(shared_ctrl_t *ctl, uint32_t tag, uint32_t code,
                   uint32_t arg0, uint32_t arg1);
void ctl_note_trap(shared_ctrl_t *ctl, uint32_t source_tag,
                   uint64_t mcause, uint64_t mepc,
                   uint64_t mtval, uint64_t mstatus);
void ctl_note_riscv_identity(shared_ctrl_t *ctl, uint32_t slot);
void ctl_note_riscv_boot_identity(shared_ctrl_t *ctl, uint32_t slot, uint32_t hartid);
void ctl_set_platform_error(shared_ctrl_t *ctl, uint32_t error_mask);
void ctl_clear_platform_error(shared_ctrl_t *ctl, uint32_t error_mask);

void usb_serial_init(void);
void usb_serial_poll(void);
void usb_serial_write(const char *buf, size_t len);
void usb_serial_write_cstr(const char *s);
size_t usb_serial_read(char *buf, size_t len);
void usb_serial_set_state(uint32_t state, uint32_t error_code);

int usb_serial_connected(void);

void console_puts(const char *s);
void console_puthex(uint32_t v);
void console_puthex64(uint64_t v);

void sg2002_usb_board_init(void);
uint32_t sg2002_platform_caps(void);
void sg2002_user_led_init(void);
void sg2002_user_led_set(int on);
void sg2002_user_led_toggle(void);
void sg2002_user_led_blink(uint32_t pulses);
void sg2002_user_led_panic_loop(void) __attribute__((noreturn));

int sg2002_image_present(uintptr_t addr);
uint32_t sg2002_crc32(const void *buf, size_t len);
int sg2002_find_staged_image(uintptr_t addr, size_t max_len,
                             kraken_staged_image_footer_t *footer);
int sg2002_validate_staged_image(uintptr_t addr, size_t max_len,
                                 uint32_t expected_kind,
                                 uintptr_t expected_load_addr,
                                 uintptr_t expected_entry_addr,
                                 kraken_staged_image_footer_t *footer);
void sg2002_copy_image(uintptr_t dst, uintptr_t src, size_t len, size_t *copied_len);
int sg2002_release_worker_core(uintptr_t entry_addr);
void sg2002_boot_8051(uintptr_t entry_addr);
typedef void (*kraken_entry_fn_t)(uintptr_t hartid, uintptr_t dtb_addr);
void kraken_jump_to(uintptr_t entry_addr) __attribute__((noreturn));
void kraken_trap_panic(uint64_t mcause, uint64_t mepc,
                       uint64_t mtval, uint64_t mstatus)
    __attribute__((noreturn));
uint64_t riscv_read_misa(void);
uint64_t riscv_read_mvendorid(void);
uint64_t riscv_read_marchid(void);
uint64_t riscv_read_mimpid(void);
uint64_t riscv_read_mhartid(void);
uint32_t kraken_trap_source_tag(void);
const char *kraken_trap_source_name(void);

#endif
