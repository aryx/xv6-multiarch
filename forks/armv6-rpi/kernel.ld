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
   * had commented out - confirmed via "-d in_asm" that QEMU's "-M
   * raspi1ap" (real BCM2835 board emulation, unlike the "-M versatilepb"
   * this port's own Makefile used, whose peripheral map doesn't match
   * this kernel's hardcoded GPIO/mailbox addresses at all - see
   * notes_arch_armv6_rpi.txt) loads a raw "-kernel kernel.img" at
   * physical 0x10000, not 0x8000. The entry.S/mmu.c code that runs
   * before paging is enabled only depends on a fixed physical-to-
   * virtual KZERO offset (+0x80000000), not on 0x8000 specifically, so
   * this is a safe, load-address-only change. */
  . = 0x80010000;
  .start_sec 0x80010000 : AT(0x10000) {
    build/entry.o(.text)
    build/mmu.o(.text .text.*)

    build/entry.o(.rodata .rodata.*)
    build/mmu.o(.rodata .rodata.*)

    build/entry.o(.data .data.*)
    build/mmu.o(.data .data.*)

    PROVIDE(edata_entry = .);

    build/entry.o(.bss .bss.* COMMON)
    build/mmu.o(.bss .bss.* COMMON)

    PROVIDE(end_entry = .);
  }

	/* 
	* Next we put the rest of the code.
	*/
	.text : {
		*.c.o(.text)
		*(.text .stub .text.*)
	}

	/*
	* read-only data
	*/
	.rodata : {
		*(.rodata .rodata.*)
	}

	/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x1000);
	
	/* 
	* Next we put the data.
	*/
	.data : {
		*(.data)
		*.c.o(*)
	}

	.bss : {
		*(.bss)
	}
  
. = ALIGN(0x1000);
  PROVIDE (end = .);
}
