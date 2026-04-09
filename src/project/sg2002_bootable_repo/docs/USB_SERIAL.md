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
- a real SG2002 DWC2 base-address integration point
- a CDC ACM console API boundary in the kernel
- a place to add EP0 control handling, descriptor responses, endpoint scheduling, and PLIC interrupt routing

## What still needs to be finished

1. EP0 setup packet handling and descriptor responses
2. Bulk IN/OUT endpoint scheduling for CDC ACM
3. Correct PLIC interrupt hookup for the USB controller
4. Final SG2002 clock/reset/PHY sequencing from the TRM
5. FIFO sizing validation on hardware

## Build impact

The project Makefile now compiles the TinyUSB-shaped scaffold sources directly,
so the repo builds without requiring an external TinyUSB checkout.

The scaffold is disabled by default because the current DWC2 backend does not
yet implement EP0 handling or endpoint scheduling. Enable it only when you are
actively experimenting with the USB bring-up path:

`make EXTRA_CFLAGS='-DKRAKEN_ENABLE_USB_DWC2_SCAFFOLD=1'`
