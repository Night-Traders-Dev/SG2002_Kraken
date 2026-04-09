#include "kraken.h"

void sg2002_usb_board_init(void) {
    /*
     * SG2002-specific USB bring-up scaffold.
     *
     * The real implementation still needs the final TRM-backed clock/reset/
     * PHY sequence and PLIC wiring. These writes intentionally keep to the
     * known top-misc USB helper registers used by vendor device trees while
     * staying conservative.
     */
    MMIO32(SG2002_USB_CTRL0_REG) |= SG2002_USB_CTRL0_DEVICE_MODE;
    MMIO32(SG2002_USB_PHY_CTRL_REG) |= SG2002_USB_PHY_CTRL_ENABLE;
    MMIO32(SG2002_USB_PHY_CTRL_REG) |= SG2002_USB_PHY_CTRL_PLL_EN;
    fence_rw();
}
