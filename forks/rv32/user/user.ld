/* claude: ported verbatim from forks/riscv/user/user.ld (MIT's current
 * xv6-riscv) - this fork never had a user-program linker script at all,
 * instead linking user programs with a flat "-N -Ttext 0" (see
 * kernel/exec.c's own ph.vaddr % PGSIZE check, which the resulting
 * non-page-aligned .data segment failed at runtime: "vaddr not page
 * aligned"). The "ALIGN(0x1000)" before .data is the fix - it's what
 * page-aligns the second PT_LOAD segment. */
OUTPUT_ARCH( "riscv" )

SECTIONS
{
 . = 0x0;
 
  .text : {
    *(.text .text.*)
  }

  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(16);
    *(.rodata .rodata.*)
  }

  .eh_frame : {
       *(.eh_frame)
       *(.eh_frame.*)
   }

  . = ALIGN(0x1000);
  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*) /* do not need to distinguish this from .data */
    . = ALIGN(16);
    *(.data .data.*)
  }

  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
  }

  PROVIDE(end = .);
}
