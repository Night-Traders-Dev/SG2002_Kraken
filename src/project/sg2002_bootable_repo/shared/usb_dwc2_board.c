#include "kraken.h"

typedef struct {
    volatile uint32_t regs[SG2002_USB_CTRL_SYS_SIZE / sizeof(uint32_t)];
} sg2002_usb_ctrl_sys_regs_t;

#define SG2002_USB_CTRL_SYS ((sg2002_usb_ctrl_sys_regs_t *)(uintptr_t)SG2002_USB_CTRL_SYS_BASE)

static uintptr_t plic_enable_reg(uint32_t context, uint32_t irq) {
    return SG2002_PLIC_ENABLE_BASE +
           (uintptr_t)context * SG2002_PLIC_ENABLE_STRIDE +
           (uintptr_t)((irq / 32u) * sizeof(uint32_t));
}

static uintptr_t plic_context_reg(uint32_t context, uint32_t offset) {
    return SG2002_PLIC_CONTEXT_BASE +
           (uintptr_t)context * SG2002_PLIC_CONTEXT_STRIDE +
           offset;
}

static void sg2002_usb_ctrl_sys_sync(void) {
    /*
     * The vendor Nano W DTB exposes a second USB companion/syscon window at
     * 0x03006000 in addition to the DWC2 core itself. The register layout is
     * still being decoded, so keep the interaction conservative for now:
     * touch the first and last words so bring-up paths model the full USB
     * block and future register sequencing has a stable home.
     */
    volatile uint32_t first = SG2002_USB_CTRL_SYS->regs[0];
    volatile uint32_t last =
        SG2002_USB_CTRL_SYS->regs[(SG2002_USB_CTRL_SYS_SIZE / sizeof(uint32_t)) - 1u];
    (void)first;
    (void)last;
}

void sg2002_usb_board_init(void) {
    /*
     * SG2002-specific USB bring-up scaffold for the Nano W vendor DTB:
     *   - DWC2 core at 0x04340000
     *   - companion USB syscon window at 0x03006000
     *   - OTG mode with the board's single USB2.0 Type-C port
     *
     * The real implementation still needs the final TRM-backed clock/reset/
     * PHY sequence. These writes intentionally keep to the known top-misc USB
     * helper registers used by vendor device trees while staying conservative.
     */
    sg2002_usb_ctrl_sys_sync();
    MMIO32(SG2002_USB_CTRL0_REG) |= SG2002_USB_CTRL0_DEVICE_MODE;
    MMIO32(SG2002_USB_PHY_CTRL_REG) |= SG2002_USB_PHY_CTRL_ENABLE;
    MMIO32(SG2002_USB_PHY_CTRL_REG) |= SG2002_USB_PHY_CTRL_PLL_EN;
    sg2002_usb_ctrl_sys_sync();
    fence_rw();
}

void sg2002_usb_plic_init(void) {
    uintptr_t enable_reg = plic_enable_reg(SG2002_PLIC_SMODE_CONTEXT,
                                           SG2002_USB_DWC2_IRQ);
    uint32_t enable_mask = 1u << (SG2002_USB_DWC2_IRQ % 32u);

    /*
     * The Nano W vendor DTB routes the USB controller through PLIC source 30
     * to the supervisor external interrupt context. Keep the threshold at 0 so
     * a later trap-driven path can receive the interrupt immediately; for now
     * the TinyUSB scaffold polls the claim/complete register from tud_task().
     */
    MMIO32(SG2002_PLIC_BASE + (SG2002_USB_DWC2_IRQ * sizeof(uint32_t))) = 1u;
    MMIO32(enable_reg) |= enable_mask;
    MMIO32(plic_context_reg(SG2002_PLIC_SMODE_CONTEXT, 0x0)) = 0u;
    fence_rw();
}

uint32_t sg2002_usb_plic_claim(void) {
    return MMIO32(plic_context_reg(SG2002_PLIC_SMODE_CONTEXT, 0x4));
}

void sg2002_usb_plic_complete(uint32_t irq) {
    MMIO32(plic_context_reg(SG2002_PLIC_SMODE_CONTEXT, 0x4)) = irq;
}
