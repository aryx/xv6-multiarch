/* claude: this fork never had a user-program linker script - user
 * programs link with a flat "-N -e main -Ttext 0" (see the Makefile's
 * own "_%:" rule), which the default MIPS O32 linker script turns into
 * a single combined .text/.rodata/.data/.bss LOAD segment at vaddr 0
 * (trivially page-aligned) - that part works fine, unlike
 * forks/riscv64's own user.ld (which exists to page-align a SECOND
 * segment the default script would otherwise leave misaligned).
 *
 * The real problem here is different: GNU ld's default MIPS O32 script
 * also emits .reginfo as its own SEPARATE program header - typed LOAD,
 * in addition to (and covering the same bytes as) a REGINFO-typed one -
 * at whatever address .reginfo happens to land (readelf -l on a built
 * _init showed 0x004000f0, ~4MB past .text). exec()'s loaduvm() panics
 * on that one ("loaduvm: addr must be page aligned") on the very first
 * exec("/init", ...) call, before anything past "cpu0: starting" ever
 * prints. The existing "$(OBJCOPY) -R .MIPS.abiflags" step in this
 * Makefile's own "_%:" rule (added for a different, already-fixed bug)
 * is NOT enough on its own even extended to ".reginfo" too - objcopy
 * zeroes the section's size but leaves the (now-empty) LOAD program
 * header entry in place, still at its own non-page-aligned vaddr, which
 * loaduvm()'s page-align check runs unconditionally before ever looking
 * at size - confirmed by testing exactly that combination by hand
 * before reaching for this linker script instead. DISCARDing the
 * sections here, at link time, means the LOAD header for them is never
 * created at all - the same "these relocation-metadata sections are
 * dead weight, don't ship them" conclusion notes_arch_mips.txt's bug 4
 * already reached for initcode, applied at the right stage for this
 * ELF/exec() path (which has no objcopy step to intervene in at all).
 */
OUTPUT_ARCH(mips)
ENTRY(main)

SECTIONS
{
  . = 0;
  .text : { *(.text) *(.text.*) }
  .rodata : { *(.rodata) *(.rodata.*) }
  .data : { *(.data) *(.data.*) *(.sdata) }
  .bss : { *(.bss) *(.bss.*) *(.sbss) *(COMMON) }

  /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.comment) *(.mdebug*) *(.gnu.attributes) }
}
