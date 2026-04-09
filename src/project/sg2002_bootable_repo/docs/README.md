# Kraken SG2002 platform repo

This tree is now organized around the platform you described:

Target board: Sipeed LicheeRV Nano W.

Board assumptions for this tree:

- the SG2002 major core is left in C906 boot mode rather than A53 mode;
- the secondary C906 is released by the kernel after DDR staging;
- USB bring-up is aimed at the board's single USB2.0 OTG Type-C path;
- the W suffix only changes onboard wireless peripherals, not the core boot flow.

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
- `build/package/worker_staged.bin`

## Design intent

This is an **AMP platform layout**:

- one controlling kernel on the main C906;
- one worker/RTOS image on the second C906;
- one independent watchdog/supervisor image on the 8051.

The current code provides the control skeleton, shared-memory ABI, staged boot flow,
worker release path, watchdog hooks, and USB console subsystem contract.

If you enable `WORKER_STAGING_ADDR`, the package now also emits `worker_staged.bin`,
which wraps the raw worker payload with a small footer carrying size and CRC32 so the
kernel can validate and copy it safely.

For the RISC-V Nano W path, you can also enable the optional XuanTie C906 cache
maintenance backend at build time with:

`make EXTRA_CFLAGS='-DKRAKEN_ENABLE_PLATFORM_DCACHE_OPS=1'`

That switches the shared cache helpers from fence-only placeholders to the
T-Head cache-management instructions used by the C906 core.

The shared control page now also carries a small fault log ring so the bootloader,
kernel, and worker can leave bring-up breadcrumbs that you can inspect with the
`faults` console command.

The RISC-V path now also installs a machine-mode trap vector in all three C906
images. Bootloader, kernel, and worker exceptions are printed on UART and latched
into the shared control page so the kernel `trap` command can show the last worker
trap cause, EPC, trap value, and status snapshot.

The `status` command now also reports a platform capability bitmask and platform
error bitmask so Nano W bring-up can distinguish missing optional hooks from
runtime failures.


## Recent USB work

The repo now includes a TinyUSB-shaped USB CDC scaffold for SG2002 DWC2 bring-up. See `docs/USB_SERIAL.md`.
