#include "class/cdc/cdc_device.h"
#include "kraken.h"
#include "tusb.h"

typedef struct {
    volatile uint32_t gotgctl;
    volatile uint32_t gotgint;
    volatile uint32_t gahbcfg;
    volatile uint32_t gusbcfg;
    volatile uint32_t grstctl;
    volatile uint32_t gintsts;
    volatile uint32_t gintmsk;
    volatile uint32_t grxstsr;
    volatile uint32_t grxstsp;
    volatile uint32_t grxfsiz;
    volatile uint32_t gnptxfsiz;
    volatile uint32_t gnptxsts;
    volatile uint32_t gi2cctl;
    volatile uint32_t gpvndctl;
    uint32_t reserved0[2];
    volatile uint32_t ggpio;
    volatile uint32_t guid;
    volatile uint32_t gsnpsid;
    volatile uint32_t ghwcfg1;
    volatile uint32_t ghwcfg2;
    volatile uint32_t ghwcfg3;
    volatile uint32_t ghwcfg4;
    uint32_t reserved1[43];
    volatile uint32_t hptxfsiz;
    volatile uint32_t dieptxf1;
    volatile uint32_t dieptxf2;
    volatile uint32_t dieptxf3;
    uint32_t reserved2[176];
    volatile uint32_t dcfg;
    volatile uint32_t dctl;
    volatile uint32_t dsts;
    uint32_t reserved3;
    volatile uint32_t diepmsk;
    volatile uint32_t doepmsk;
    volatile uint32_t daint;
    volatile uint32_t daintmsk;
    uint32_t reserved4[2];
    volatile uint32_t dvbusdis;
    volatile uint32_t dvbuspulse;
    volatile uint32_t dtknqr1;
    volatile uint32_t dtknqr2;
    volatile uint32_t dtknqr3;
    volatile uint32_t dtknqr4;
} dwc2_regs_t;

#define DWC2 ((dwc2_regs_t *)(uintptr_t)SG2002_USB_DWC2_BASE)

#define DWC2_GAHBCFG_GINT         (1u << 0)
#define DWC2_GUSBCFG_PHYSEL       (1u << 6)
#define DWC2_GRSTCTL_CSRST        (1u << 0)
#define DWC2_GINTSTS_USBRST       (1u << 12)
#define DWC2_GINTSTS_ENUMDONE     (1u << 13)
#define DWC2_GINTSTS_USBSUSP      (1u << 11)
#define DWC2_GINTSTS_WKUPINT      (1u << 31)
#define DWC2_GINTSTS_IEPINT       (1u << 18)
#define DWC2_GINTSTS_OEPINT       (1u << 19)
#define DWC2_GINTSTS_RXFLVL       (1u << 4)
#define DWC2_DCFG_DEVSPD_FS       0x3u
#define DWC2_FIFO_RX_WORDS        256u
#define DWC2_FIFO_NPTX_WORDS       64u
#define DWC2_FIFO_EP0_WORDS        64u

typedef struct {
    uint8_t rhport;
    uint8_t configured;
    uint8_t suspended;
    uint8_t initialized;
    uint8_t reserved;
} dwc2_state_t;

static dwc2_state_t s_dwc2;

static void dwc2_wait_reset_clear(void) {
    for (uint32_t i = 0; i < 500000u; ++i) {
        if ((DWC2->grstctl & DWC2_GRSTCTL_CSRST) == 0) return;
        cpu_relax();
    }
}

static void dwc2_core_soft_reset(void) {
    DWC2->grstctl |= DWC2_GRSTCTL_CSRST;
    dwc2_wait_reset_clear();
}

static void dwc2_program_fifos(void) {
    DWC2->grxfsiz = DWC2_FIFO_RX_WORDS;
    DWC2->gnptxfsiz = (DWC2_FIFO_RX_WORDS & 0xffffu) |
                      ((DWC2_FIFO_NPTX_WORDS & 0xffffu) << 16);
    DWC2->dieptxf1 = ((DWC2_FIFO_RX_WORDS + DWC2_FIFO_NPTX_WORDS) & 0xffffu) |
                     ((DWC2_FIFO_EP0_WORDS & 0xffffu) << 16);
}

static void dwc2_enable_interrupts(void) {
    DWC2->gintsts = 0xffffffffu;
    DWC2->gintmsk = DWC2_GINTSTS_USBRST |
                    DWC2_GINTSTS_ENUMDONE |
                    DWC2_GINTSTS_USBSUSP |
                    DWC2_GINTSTS_WKUPINT |
                    DWC2_GINTSTS_IEPINT |
                    DWC2_GINTSTS_OEPINT |
                    DWC2_GINTSTS_RXFLVL;
    DWC2->gahbcfg |= DWC2_GAHBCFG_GINT;
}

static void dcd_init(uint8_t rhport) {
    s_dwc2.rhport = rhport;
    sg2002_usb_board_init();
    sg2002_usb_plic_init();
    DWC2->gahbcfg = 0;
    DWC2->gusbcfg |= DWC2_GUSBCFG_PHYSEL;
    dwc2_core_soft_reset();
    DWC2->dcfg = DWC2_DCFG_DEVSPD_FS;
    dwc2_program_fifos();
    dwc2_enable_interrupts();
    s_dwc2.initialized = 1;
}

static void dcd_task(void) {
    for (uint32_t budget = 0; budget < 4u; ++budget) {
        uint32_t irq = sg2002_usb_plic_claim();

        if (irq == 0u)
            break;

        if (irq == SG2002_USB_DWC2_IRQ) {
            dcd_int_handler(s_dwc2.rhport);
            sg2002_usb_plic_complete(irq);
            continue;
        }

        /* Unexpected IRQ on the same S-mode context. Complete it so we do not
         * wedge the claim register while the rest of the platform is still
         * using a polling-only interrupt model. */
        sg2002_usb_plic_complete(irq);
        break;
    }

    if ((DWC2->gintsts & DWC2->gintmsk) != 0u)
        dcd_int_handler(s_dwc2.rhport);
}

void dcd_int_handler(uint8_t rhport) {
    (void)rhport;
    uint32_t sts = DWC2->gintsts & DWC2->gintmsk;
    if (sts == 0) return;

    if (sts & DWC2_GINTSTS_USBRST) {
        DWC2->gintsts = DWC2_GINTSTS_USBRST;
        s_dwc2.configured = 0;
        s_dwc2.suspended = 0;
        tinyusb_cdc_set_connected(false);
        tud_umount_cb();
    }

    if (sts & DWC2_GINTSTS_ENUMDONE) {
        DWC2->gintsts = DWC2_GINTSTS_ENUMDONE;
        /*
         * Enumeration completion alone does not mean CDC is operational until
         * EP0 request handling is implemented. Leave the CDC link logically
         * disconnected so higher layers stay honest.
         */
    }

    if (sts & DWC2_GINTSTS_USBSUSP) {
        DWC2->gintsts = DWC2_GINTSTS_USBSUSP;
        s_dwc2.suspended = 1;
        tud_suspend_cb(false);
    }

    if (sts & DWC2_GINTSTS_WKUPINT) {
        DWC2->gintsts = DWC2_GINTSTS_WKUPINT;
        s_dwc2.suspended = 0;
        tud_resume_cb();
    }

    if (sts & DWC2_GINTSTS_RXFLVL) {
        DWC2->gintsts = DWC2_GINTSTS_RXFLVL;
    }

    if (sts & DWC2_GINTSTS_IEPINT) DWC2->gintsts = DWC2_GINTSTS_IEPINT;
    if (sts & DWC2_GINTSTS_OEPINT) DWC2->gintsts = DWC2_GINTSTS_OEPINT;
}

bool tinyusb_dwc2_ready(void) {
    return s_dwc2.initialized != 0;
}

void tinyusb_dwc2_task(void) {
    dcd_task();
}

void tinyusb_dwc2_init(uint8_t rhport) {
    dcd_init(rhport);
}
