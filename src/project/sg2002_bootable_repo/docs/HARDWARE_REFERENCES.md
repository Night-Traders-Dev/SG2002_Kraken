# Hardware References

This repo now keeps an explicit pointer to the local SG2002 technical reference
manual copy we have been using for custom firmware bring-up:

- `SG2002 TRM (English) v1.02`
  - local file: `/home/kraken/sg2002_trm_en_v1.02.pdf`
  - local copy metadata:
    - created: `2025-02-26 21:50:39 EST`
    - pages: `841`
    - size: `7865221` bytes

Use this TRM as the primary hardware reference for:

- clock, reset, and syscon programming
- USB OTG / DWC2 controller bring-up
- GPIO, pinmux, and board helper registers
- RTC / RTCSYS control
- watchdog and restart paths
- storage-controller and filesystem bring-up work
- interrupt-controller and timer details

Notes for this tree:

- The Nano W firmware work should prefer this TRM over stale terminal notes or
  inferred register guesses.
- When vendor DTBs or vendor boot assets disagree with the current code, check
  this TRM and the extracted Nano W DTB together before changing MMIO paths.
- This file is only a pointer to the local PDF. The PDF itself remains outside
  the repo working tree.
