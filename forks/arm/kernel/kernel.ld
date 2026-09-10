OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

ENTRY_SVC_STACK_SIZE = 0x1000;

SECTIONS
{
  /* the entry point, before enabling paging. The code to enable paing
   needs to have the same virtual/physical address. entry.S and start.c
   run in this initial setting.*/
  /* claude: switched to the alternate 0x10000 base this file already
   * had commented out - confirmed via the QEMU monitor's "xp" command
   * that "-M raspi2b"'s "-kernel" loader (for a raw binary image) places
   * content at physical 0x10000, not 0x8000 - the exact same finding as
   * notes_arch_armv6_rpi.txt's own bug 5 for the sibling "-M raspi1ap"
   * port. See notes_arch_armv7_rpi.txt. */
  . = 0x00010000;
  /* claude: these name object files by their BUILD path, and entry.S and
   * start.c moved into kernel/ in Phase 0, so the objects are now
   * build/kernel/*.o. Worth knowing that ld reports this as "cannot find
   * build/entry.o" even though that path appears nowhere on the link
   * command line - a linker script naming inputs is easy to miss when
   * grepping the Makefile. See plan_factorization.md's gotcha 8. */
  .start_sec : {
    build/kernel/entry.o(.text)
    build/kernel/start.o(.text .text.*)

    build/kernel/entry.o(.rodata .rodata.*)
    build/kernel/start.o(.rodata .rodata.*)

    build/kernel/entry.o(.data .data.*)
    build/kernel/start.o(.data .data.*)

    PROVIDE(edata_entry = .);

    build/kernel/entry.o(.bss .bss.* COMMON)
    build/kernel/start.o(.bss .bss.* COMMON)

    /*define a stack for the entry*/
    . = ALIGN(0x1000);
    . += ENTRY_SVC_STACK_SIZE;

    PROVIDE (svc_stktop = .);

    /* define the kernel page table, must be 16K and 16K-aligned*/
    . = ALIGN(0x4000);
    PROVIDE (_kernel_pgtbl = .);
    . += 0x4000;

    /* we also need a user page table*/
    PROVIDE (_user_pgtbl = .);
    . += 0x1000;

    PROVIDE(end_entry = .);
  }

  /*the kernel executes at the higher 2GB address space, but loaded
   at the lower memory (0x20000)*/
  . = 0x80020000;  /* HCLIN: below text symbols is in VA space **/

  .text : AT(0x00020000){ /** here to make code also copied into phymem **/
    *(.text .text.* .gnu.linkonce.t.*)
  }

  PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */

  .rodata : {
    *(.rodata .rodata.* .gnu.linkonce.r.*)
  }

  /* aligned the data to a (4K) page, so it can be assigned
   different protection than the code*/
  . = ALIGN(0x1000);

  PROVIDE (data_start = .);

  .data : {
    *(.data .data.*)
  }

  PROVIDE (edata = .);

  .bss : {
    *(.bss .bss.* COMMON)
  }

  . = ALIGN(0x1000);
  PROVIDE (end = .);
}
