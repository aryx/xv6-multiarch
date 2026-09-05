OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

ENTRY_SVC_STACK_SIZE = 0x1000;

SECTIONS
{
  /* the entry point, before enabling paging. The code to enable paing
   needs to have the same virtual/physical address. entry.S and start.c
   run in this initial setting.*/
  /* . = 0x10000; */
  . = 0x80008000;
  .start_sec 0x80008000 : AT(0x8000) {
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
