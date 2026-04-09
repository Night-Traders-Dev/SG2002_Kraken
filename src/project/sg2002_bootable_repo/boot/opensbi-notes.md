# OpenSBI integration notes

This project assumes the main C906 boots through the vendor FSBL/OpenSBI path first, then enters the manager payload in supervisor mode.
Public bring-up reports note that the OpenSBI environment may reserve low RAM and that the second core may only reliably observe a small upper RAM window, so the worker image and shared control block are deliberately placed high in RAM.

Practical implications:
- Avoid placing your payload at the very start of DRAM.
- Keep worker image and shared memory near the top of the worker-visible area.
- If you later switch to M-mode or vendor SPL handoff, revisit the linker addresses.
