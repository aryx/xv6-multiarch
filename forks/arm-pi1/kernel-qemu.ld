/******************************************************************************
*	kernel-qemu.ld
*
*	claude: QEMU-only variant of kernel.ld, for "-M raspi1ap" - real
*	Raspberry Pi 1 firmware always loads a kernel.img at physical 0x8000
*	(matched by kernel.ld's own AT(0x8000)), but QEMU's raspi1ap "-kernel"
*	loader (for a raw binary image) places it at physical 0x10000 instead
*	(confirmed via the QEMU monitor's "xp" command reading back this
*	kernel's own code at that exact address - see notes_arch_rpi1.txt).
*	kernel.ld itself is deliberately left untouched: it is what real
*	hardware boots from, and the user has physical Raspberry Pi 1/2
*	boards to test on - this file exists purely so a QEMU-only image can
*	be built from the exact same sources without touching that path at
*	all. Keep this file's SECTIONS layout in sync with kernel.ld's own if
*	that one is ever restructured (not just the base address) - only the
*	load address differs here.
******************************************************************************/

SECTIONS {
        /* Link the kernel at this address: "." means the current address */
        . = 0x80010000;


	/*
	* First and formost we need the .init section, containing the code to
        * be run first. We allow room for the ATAGs and stack and conform to
        * the bootloader's expectation by putting this code at 0x8000.
	*/
	.init 0x80010000 : AT(0x10000) {
		*(.init)
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

	/* claude: marks the start of .bss so cmain() can zero it explicitly
	 * - see main.c's own "bsszero()"/its call site comment. */
	PROVIDE(bss_start = .);
	.bss : {
		*(.bss)
	}

	PROVIDE(end = .);

	/*
	* Finally comes everything else. A fun trick here is to put all other
	* sections into this section, which will be discarded by default.
	*/
	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
}
