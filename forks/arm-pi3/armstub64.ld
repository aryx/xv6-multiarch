/* claude: QEMU-only armstub linker script - links at 0x80000, the
 * physical address QEMU's "-M raspi3b" places a raw "-kernel" image at
 * (confirmed via the monitor's "info roms"). See armstub64.S. */
ENTRY(_start)
SECTIONS
{
	. = 0x80000;
	.text : { *(.text) }
	.data : { *(.data) }
	.bss  : { *(.bss) }
}
