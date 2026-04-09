OUTPUT_ARCH(riscv)
ENTRY(_start)
MEMORY { ram (rwx) : ORIGIN = 0x80180000, LENGTH = 128K }
SECTIONS {
  . = ORIGIN(ram);
  .text : { KEEP(*(.text.start)) *(.text*) *(.rodata*) } > ram
  .data : { *(.data*) } > ram
  .bss (NOLOAD) : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; } > ram
  . = ALIGN(16); . += 4K; __stack_top = .;
}
