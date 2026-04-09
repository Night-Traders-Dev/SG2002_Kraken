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
#define SG2002_SYS_C906L_CTRL_REG     (SG2002_TOP_MISC_BASE + 0x04)
#define SG2002_SYS_C906L_BOOTADDR_LO  (SG2002_TOP_MISC_BASE + 0x20)
#define SG2002_SYS_C906L_BOOTADDR_HI  (SG2002_TOP_MISC_BASE + 0x24)
#define SG2002_SYS_C906L_CTRL_EN      (1u << 13)

#define SG2002_USB_OTG_BASE           0x040C0000ull
#define SG2002_USB_HOST_BASE          0x040D0000ull
#define SG2002_USB_DEV_BASE           0x040E0000ull
#define SG2002_USB_DWC2_BASE          0x04340000ull
#define SG2002_USB_CTRL0_REG          (SG2002_TOP_MISC_BASE + 0x38)
#define SG2002_USB_PHY_CTRL_REG       (SG2002_TOP_MISC_BASE + 0x48)
#define SG2002_USB_CTRL0_DEVICE_MODE  (1u << 0)
#define SG2002_USB_PHY_CTRL_ENABLE    (1u << 0)
#define SG2002_USB_PHY_CTRL_PLL_EN    (1u << 1)
#ifndef SG2002_USB_DWC2_IRQ
#define SG2002_USB_DWC2_IRQ           14u
#endif

#define KRAKEN_MAGIC                  0x4B52414Bu
#define KRAKEN_VERSION                4u
#define KRAKEN_STAGED_IMAGE_MAGIC     0x4B535447u
#define KRAKEN_STAGED_IMAGE_TAIL      0x4B454E44u
#define KRAKEN_STAGED_IMAGE_VERSION   1u
#define WORKER_IMAGE_PROBE_WORDS      8u
#define KRAKEN_FAULT_LOG_SIZE         16u
#define USB_SERIAL_RING_SIZE          512u
#define USB_SERIAL_PROTO_ACM          1u

#ifndef WORKER_STAGING_ADDR
#define WORKER_STAGING_ADDR           0x00000000ull
#endif
#ifndef WORKER_IMAGE_MAX_BYTES
#define WORKER_IMAGE_MAX_BYTES        (128u * 1024u)
#endif
#ifndef KRAKEN_ENABLE_PLATFORM_DCACHE_OPS
#define KRAKEN_ENABLE_PLATFORM_DCACHE_OPS 0
#endif
#ifndef KRAKEN_ENABLE_WORKER_RESET_HOOK
#define KRAKEN_ENABLE_WORKER_RESET_HOOK 0
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
    SYSF_WATCHDOG_TIMEOUT     = 1u << 31,
};

enum kraken_fault_source {
    FAULTSRC_BOOTLOADER = 1,
    FAULTSRC_KERNEL     = 2,
    FAULTSRC_WORKER     = 3,
    FAULTSRC_WATCHDOG   = 4,
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
    volatile uint32_t platform_caps;
    volatile uint32_t platform_errors;
    volatile uint32_t fault_log_head;
    volatile uint32_t fault_log_count;
    volatile uint32_t fault_last_tag;
    volatile uint32_t fault_last_code;
    volatile uint32_t fault_last_arg0;
    volatile uint32_t fault_last_arg1;
    volatile uint32_t reserved0;
    volatile kraken_fault_record_t fault_log[KRAKEN_FAULT_LOG_SIZE];
    volatile uint8_t usb_rx_ring[USB_SERIAL_RING_SIZE];
    volatile uint8_t usb_tx_ring[USB_SERIAL_RING_SIZE];
} shared_ctrl_t;

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
void ctl_fault_log(shared_ctrl_t *ctl, uint32_t tag, uint32_t code,
                   uint32_t arg0, uint32_t arg1);

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

int sg2002_image_present(uintptr_t addr);
uint32_t sg2002_crc32(const void *buf, size_t len);
int sg2002_find_staged_image(uintptr_t addr, size_t max_len,
                             kraken_staged_image_footer_t *footer);
int sg2002_validate_staged_image(uintptr_t addr, size_t max_len,
                                 uint32_t expected_kind,
                                 uintptr_t expected_load_addr,
                                 uintptr_t expected_entry_addr,
                                 kraken_staged_image_footer_t *footer);
void sg2002_copy_image(uintptr_t dst, uintptr_t src, size_t max_len, size_t *copied_len);
int sg2002_release_worker_core(uintptr_t entry_addr);
void sg2002_boot_8051(uintptr_t entry_addr);
void kraken_jump_to(uintptr_t entry_addr) __attribute__((noreturn));

#endif
