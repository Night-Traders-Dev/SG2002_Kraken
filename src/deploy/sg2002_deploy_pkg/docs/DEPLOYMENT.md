# Custom deployment flow for Kraken SG2002

Board target: Sipeed LicheeRV Nano W.

This package stages the new four-image layout:

- `bootloader.bin`
- `kernel.bin`
- `worker.bin`
- `worker_staged.bin` for optional manager-side worker staging
- `mars_mcu_fw.bin`

## Included helpers

- `deploy_sg2002_custom.sh`: stage bootloader/kernel/worker/8051 artifacts.
- `package_for_board.sh`: repack a finished package directory.
- `linux_enable_usb_acm.sh`: enable USB CDC ACM gadget mode under vendor Linux.
- `linux_disable_usb_gadget.sh`: tear down the gadget cleanly.

## Manual integration points

You still need to provide:

- your own DDR writer or boot script to place all four binaries;
- a `WORKER_STAGING_ADDR` load step for `worker_staged.bin` if you choose kernel-side worker copying;
- confirmation that the board remains strapped for C906 major-core boot instead of A53 boot;
- clearing the shared control page at `0x80170000` before first boot;
- any board-specific secondary-core reset deassert beyond the public top-misc enable bit;
- an eventual native bare-metal USB device backend for the board's USB OTG Type-C port with full EP0, endpoint scheduling, and SG2002-specific interrupt wiring.
