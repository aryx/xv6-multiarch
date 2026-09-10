/* claude: ported from forks/riscv/user/user.ld (same fix rv32 needed too -
 * see docs/claude_notes/notes_arch_riscv32.txt's "bug 2" and
 * notes_debugging_techniques.txt item 13) - this fork also links user
 * programs with a flat "-N -Ttext 0" (see its own Makefile's "_%:" rule)
 * and its own kernel/exec.c also enforces "ph.vaddr % PGSIZE == 0" on
 * every loaded segment, so the same missing page-alignment between
 * .text and .data would fault the same way at runtime. Added
 * proactively, before hitting the failure, since this fork shares the
 * exact same kernel/exec.c lineage as forks/riscv and forks/riscv32. The
 * "ALIGN(0x1000)" before .data is the fix - it page-aligns the second
 * PT_LOAD segment. */
OUTPUT_ARCH( "aarch64" )

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
