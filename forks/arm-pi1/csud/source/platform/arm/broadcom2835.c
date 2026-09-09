/******************************************************************************
*	platform/arm/broadcom2835.c
*	 by Alex Chadwick
*
*	A light weight implementation of the USB protocol stack fit for a simple
*	driver.
*
*	platform/arm/broadcom2835.c contains code for the broadcom2835 chip, used 
*	in the Raspberry Pi. Compiled conditionally on LIB_BCM2835=1.
******************************************************************************/
#include <configuration.h>
#include <platform/platform.h>
#include <types.h>

void Bcm2835Load()
{
	LOG_DEBUG("CSUD: Broadcom2835 driver version 0.1.\n");
}

#ifndef TYPE_DRIVER

/* claude: both addresses below were raw ARM PHYSICAL addresses
 * (0x20003004, 0x2000B880) - see configuration.h's own
 * HCD_DESIGNWARE_BASE comment for why that's wrong under this kernel's
 * memory map (no identity mapping for physical peripherals; everything
 * goes through the DEVSPACE=0xFE000000 virtual alias). Relocated to
 * that same alias, and cross-checked directly against this kernel's
 * OWN drivers for the identical hardware: 0xFE003004 matches
 * source/timer.c's own "TIMER_REGS_BASE 0xFE003000" + COUNTER_LO's
 * offset 0x4; 0xFE00B880 is byte-for-byte include/mailbox.h's own
 * "MAILBOX_BASE 0xFE00B880" - PowerOnUsb() below is doing the exact
 * same legacy "channel 0" (power management) mailbox protocol as this
 * kernel's own source/mailbox.c writemailbox()/readmailbox(), just
 * inlined with an immediate value instead of a buffer pointer. */
void MicroDelay(u32 delay) {
	volatile u64* timeStamp = (u64*)(0xFE000000 + 0x3004);
	u64 stop = *timeStamp + delay;

	while (*timeStamp < stop)
		__asm__("nop");
}

/* claude: declared in hcd/dwc/designware20.c - see its own comment.
 * No shared header pulls it in here, so declared directly rather than
 * adding a new header just for one function. */
extern bool HcdEmulating(void);

Result PowerOnUsb() {
	volatile u32* mailbox;
	u32 result;

	/* claude: this is the legacy "channel 0" (power management)
	 * mailbox protocol - a different channel from every one this
	 * kernel's own source/mailbox.c ever uses (channel 1 for the
	 * framebuffer, channel 8 for property tags). QEMU's mailbox model
	 * (hw/misc/bcm2835_mbox.c/bcm2835_property.c) does not answer it at
	 * all: writing the request leaves the "empty" status bit set
	 * forever, so the response-wait loop below hangs the boot solid
	 * (confirmed: "-d int" shows zero further interrupt activity once
	 * stuck here). Skipped under emulation - USB is already powered in
	 * a QEMU guest, there is no real power rail to sequence - real
	 * hardware is unaffected and still goes through the real handshake. */
	if (HcdEmulating())
		return OK;

	mailbox = (u32*)(0xFE000000 + 0xB880);
	while (mailbox[6] & 0x80000000);
	mailbox[8] = 0x80;
	do {
		while (mailbox[6] & 0x40000000);
	} while (((result = mailbox[0]) & 0xf) != 0);
	return result == 0x80 ? OK : ErrorDevice;
}

#endif