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

1. vendor FSBL/OpenSBI/U-Boot stages your images into DDR, either from a
   manual U-Boot script path or from the Nano W `fip.bin` + `boot.sd` SD path;
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

`src/build.sh` can then package those artifacts in two ways:

- `BUILD_TARGET=staging` for an existing vendor staging flow;
- `BUILD_TARGET=licheerv_nano_w_riscv` for a Nano W FAT card containing
  `fip.bin` and `boot.sd`.

The Nano W packaging path now follows the same rootless SD image shape used by
Sipeed's release flow:

- partition 1 is a bootable `16 MiB` FAT32 volume at sector `1`
- partition 2 is an ext4 placeholder partition that preserves the normal
  two-partition SD layout
- the boot partition carries `fip.bin`, `boot.sd`, the Kraken payloads, and the
  usual vendor marker files such as `usb.dev`, `usb.ncm`, `usb.rndis`,
  `wifi.sta`, `gt9xx`, and `ver`

## Design intent

This is an **AMP platform layout**:

- one controlling kernel on the main C906;
- one worker/RTOS image on the second C906;
- one independent watchdog/supervisor image on the 8051.

The current code provides the control skeleton, shared-memory ABI, staged boot flow,
worker release path, watchdog hooks, and USB console subsystem contract.

The bare-metal DWC2 USB scaffold is now disabled by default. That keeps the
status and platform-capability reports honest until EP0 handling and endpoint
scheduling are implemented. The current CDC ACM experiment also avoids one class
of reconnect loop by refusing to queue USB console output unless the host has
opened the port and by dropping stale TX data when DTR deasserts. If you want
to keep experimenting with the USB scaffold, build with:

`make EXTRA_CFLAGS='-DKRAKEN_ENABLE_USB_DWC2_SCAFFOLD=1'`

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

The shared control page now also records `misa`, `mvendorid`, `marchid`, `mimpid`,
and `mhartid` for the bootloader, kernel, and worker images. Use the kernel `cpu`
command to confirm which RISC-V harts and ISA profile the Nano W actually booted.

The worker release path now also verifies that the secondary C906 boot address
registers and enable bit actually latch before it starts waiting for a boot ACK.
That turns "never came online" into a concrete release failure when the register
programming itself does not stick.

For the Nano W RISC-V path, the worker reset hook is now enabled by default.
It pulses the SG200X active-low CPU reset bit for `CPUSYS2` before the kernel
waits for the worker boot ACK. If you need to override the published reset line
or disable it during board experiments, set `KRAKEN_WORKER_RESET_REG`,
`KRAKEN_WORKER_RESET_BIT`, or `KRAKEN_ENABLE_WORKER_RESET_HOOK=0` at build time.

The `status` command now also reports a platform capability bitmask and platform
error bitmask so Nano W bring-up can distinguish missing optional hooks from
runtime failures.

For the Nano W ROM-boot path, `src/build.sh` now generates a `boot.sd` FIT
payload that uses vendor U-Boot's normal `fatload ... boot.sd ; bootm ...`
flow. The FIT now boots `kernel.bin` directly as the primary payload, carries
the vendor `sg2002_licheervnano_sd` DTB, and preloads `worker.bin` plus
`mars_mcu_fw.bin` through FIT `loadables`. You must still supply a
vendor-built `fip.bin` for the card to be ROM-bootable.

For bring-up without UART, the Nano W user LED now mirrors the major boot
milestones:

- one blink when `bootloader.bin` starts;
- two blinks when `kernel.bin` starts;
- a slow heartbeat while the kernel supervisor loop is alive;
- continuous blinking if bootloader or kernel panics.

## Recent USB work

The repo now includes a TinyUSB-shaped USB CDC scaffold for SG2002 DWC2 bring-up. See `docs/USB_SERIAL.md`.
