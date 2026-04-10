# Staged boot flow

This flow is written for the LicheeRV Nano W with the SG2002 major core left in
its default C906 boot configuration.

## DDR layout

- `0x80200000` — `bootloader.bin`
- `0x80280000` — `kernel.bin`
- `0x80300000` — shared control page
- `0x80380000` — `worker.bin`
- `0x83f80000` — `mars_mcu_fw.bin`

## Sequence

### 1. U-Boot or another staging loader
Loads all four images and clears the shared control page.

For the Nano W ROM-boot SD path, vendor U-Boot loads `bootloader.bin` as the
primary FIT payload from `boot.sd` and preloads `kernel.bin`, `worker.bin`,
and `mars_mcu_fw.bin` through FIT `loadables`.

If you enable manager-side worker staging, load `worker_staged.bin` at
`WORKER_STAGING_ADDR` instead of preloading the raw `worker.bin` at `0x80380000`.

### 2. Bootloader
Initializes the control page, enables USB-console state, and jumps to the kernel.

### 3. Kernel
Checks worker image availability, sends a mailbox boot command, writes the
secondary-core boot address, asserts the public enable bit, waits for boot ACK,
maps the 8051 ROM/XDATA windows, starts the 8051 watchdog, and enters supervision.

When `WORKER_STAGING_ADDR` is enabled and `worker.bin` is not already present at
`0x80380000`, the kernel now looks for a staged worker image footer, validates the
CRC32, copies the exact payload length into place, and then releases the worker core.

### 4. Worker
Acknowledges boot, services commands, and updates heartbeats.

### 5. 8051
Runs from `0x83f80000` and sees the shared control page through its XDATA window
starting at `0x0000`. It monitors kernel pet sequence and worker heartbeat and
escalates reset on timeout.
