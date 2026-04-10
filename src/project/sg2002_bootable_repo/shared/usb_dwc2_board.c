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

static void sg2002_usb_deassert_resets(void) {
    MMIO32(SG200X_SOFT_RSTN0_REG) |= (1u << SG200X_RST_USB_BIT);
    MMIO32(SG200X_SOFT_RSTN1_REG) |= (1u << SG200X_RST_USB_PHY_BIT);
}

static void sg2002_usb_ctrl_sys_sync(void) {
    /*
     * The vendor Nano W DTB exposes a second USB companion/syscon window at
     * 0x03006000 in addition to the DWC2 core itself, even though the TRM
     * leaves that range undocumented. Keep the interaction conservative for
     * now: touch the first and last words so bring-up paths model the full
     * vendor block and future register sequencing has a stable home.
     */
    volatile uint32_t first = SG2002_USB_CTRL_SYS->regs[0];
    volatile uint32_t last =
        SG2002_USB_CTRL_SYS->regs[(SG2002_USB_CTRL_SYS_SIZE / sizeof(uint32_t)) - 1u];
    (void)first;
    (void)last;
}

void sg2002_usb_board_init(void) {
    uint32_t phy_ctrl;

    /*
     * SG2002-specific USB bring-up scaffold for the Nano W vendor DTB:
     *   - DWC2 core at 0x04340000
     *   - companion USB syscon window at 0x03006000
     *   - OTG mode with the board's single USB2.0 Type-C port
     *
     * The TRM-backed parts we can safely do here are:
     *   - deassert the documented USB and USB_PHY resets
     *   - leave VBUS sourcing disabled in device mode
     *
     * Mode forcing itself is done through DWC2 GUSBCFG in dcd_dwc2.c, which is
     * documented in the USBC chapter. The 0x03006000 window remains vendor-DTB
     * specific until its register layout is decoded.
     */
    sg2002_usb_deassert_resets();
    sg2002_usb_ctrl_sys_sync();
    phy_ctrl = MMIO32(SG2002_USB_PHY_CTRL_REG);
    phy_ctrl &= ~SG2002_USB_PHY_CTRL_DRIVE_VBUS;
    MMIO32(SG2002_USB_PHY_CTRL_REG) = phy_ctrl;
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
