# Kraken SG2002 platform repo

This tree is now organized around the platform you described:

- `bootloader/` — primary C906 stage-2 bootloader
- `kernel/` — primary C906 kernel / supervisor
- `worker_rtos/` — secondary C906 service firmware
- `watch8051/` — low-power 8051 watchdog firmware
- `shared/` — common MMIO, mailbox, shared-state, console, and USB-serial code
- `tools/` — packaging helpers

## Boot flow

1. vendor FSBL/OpenSBI/U-Boot stages your images into DDR;
2. `bootloader.bin` starts on the main C906;
3. bootloader clears and initializes the shared control page;
4. bootloader starts the 8051 watchdog firmware;
5. bootloader hands off to `kernel.bin`;
6. kernel verifies and releases `worker.bin` on the secondary C906;
7. kernel supervises the worker and pets the 8051 watchdog.

## Output artifacts

`make` now builds:

- `build/bootloader.bin`
- `build/kernel.bin`
- `build/worker.bin`
- `build/mars_mcu_fw.bin`
- `build/package/manifest.json`
- `build/package/8051_boot_cfg.ini`
- `build/package/load_demo.sh`

## Design intent

This is an **AMP platform layout**:

- one controlling kernel on the main C906;
- one worker/RTOS image on the second C906;
- one independent watchdog/supervisor image on the 8051.

The current code provides the control skeleton, shared-memory ABI, staged boot flow,
worker release path, watchdog hooks, and USB console subsystem contract.


## Recent USB work

The repo now includes a TinyUSB-shaped USB CDC scaffold for SG2002 DWC2 bring-up. See `docs/USB_SERIAL.md`.
