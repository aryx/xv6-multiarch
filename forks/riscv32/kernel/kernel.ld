OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

SECTIONS
{
  /*
   * ensure that entry.S / _entry is at 0x80000000,
   * where qemu's -kernel jumps.
   */
  . = 0x80000000;
  .text :
  {
    *(.text)
    . = ALIGN(0x1000);
    *(trampsec)
  }

  /*
   * claude: named explicitly rather than left to orphan-section
   * placement. GNU ld puts an unlisted .rodata after .text, but ld.lld
   * (LLVM=1) puts read-only orphans BEFORE it - .rodata.str1.1 landed at
   * 0x80000000 and pushed _entry to 0x80000650, away from where
   * "-bios none" jumps. See notes_arch_riscv32.txt.
   */
  .rodata : {
    *(.srodata .srodata.*)
    *(.rodata .rodata.*)
  }

  . = ALIGN(0x1000);
  PROVIDE(etext = .);

  /*
   * make sure end is after data and bss.
   */
  .data : {
    *(.data)
  }
  .bss : {
    *(.bss)
    *(.sbss*)
     PROVIDE(end = .);
  }
}
