# USB Serial on Kraken SG2002

This tree now includes a **TinyUSB-shaped device stack scaffold** for SG2002.

## What is included

- `third_party/tinyusb/src/tusb.c`
  - minimal TinyUSB-compatible task and init entry points
- `third_party/tinyusb/src/class/cdc/cdc_device.c`
  - CDC ACM software FIFOs and class-facing API
- `third_party/tinyusb/src/portable/synopsys/dwc2/dcd_dwc2.c`
  - SG2002-targeted DWC2 device-controller scaffold
- `shared/usb_dwc2_board.c`
  - SG2002 board glue for device-mode USB control and PHY helper registers
- `shared/usb_serial.c`
  - kernel console integration using the TinyUSB-style CDC API
- `shared/usb_descriptors.c`
  - placeholder CDC ACM device/config/string descriptors for the later full stack

## Current status

This is **not yet a full upstream TinyUSB import**. It is a compile-clean scaffold that gives the repo:

- the right file layout for a later drop-in of upstream TinyUSB
- a real SG2002 DWC2 base-address integration point at `0x04340000`
- the Nano W companion USB syscon window at `0x03006000`
- the Nano W vendor-DTB USB interrupt line (`PLIC` source `30`)
- a CDC ACM console API boundary in the kernel
- a minimal `PLIC` claim/complete polling path so the scaffold can consume pending USB IRQs without needing a full trap-return implementation yet
- a small amount of reconnect hygiene in the CDC ACM path so stale TX data is not replayed after the host closes and reopens the port
- documented DWC2 device-mode setup steps from the SG2002 TRM:
  `SOFT_RSTN_0[USB]`, `SOFT_RSTN_1[USB_PHY]`, `GUSBCFG.ForceDevMode`, and
  `DCTL.SftDiscon`

## What still needs to be finished

1. EP0 setup packet handling and descriptor responses
2. Bulk IN/OUT endpoint scheduling for CDC ACM
3. Trap-driven supervisor external interrupt handling instead of claim/complete polling from `tud_task()`
4. Final SG2002-specific clock and companion-syscon sequencing for the vendor
   DTB window at `0x03006000` (the TRM leaves that range undocumented)
5. FIFO sizing validation on hardware

## Build impact

The project Makefile now compiles the TinyUSB-shaped scaffold sources directly,
so the repo builds without requiring an external TinyUSB checkout.

The scaffold is disabled by default because the current DWC2 backend does not
yet implement EP0 handling or endpoint scheduling. Enable it only when you are
actively experimenting with the USB bring-up path:

`make EXTRA_CFLAGS='-DKRAKEN_ENABLE_USB_DWC2_SCAFFOLD=1'`

## Current CDC ACM behavior

The current experimental USB console path now has two defensive behaviors:

- `usb_serial_write()` only queues bytes when TinyUSB reports the CDC ACM interface as connected
- `tud_cdc_line_state_cb()` flushes queued TX bytes and returns the console state to `USB_SERIAL_READY` when the host drops DTR

These changes improve host reconnect behavior, but they do **not** make the USB path production-ready. Full EP0 handling, endpoint scheduling, and SG2002-specific interrupt/controller validation are still required.
