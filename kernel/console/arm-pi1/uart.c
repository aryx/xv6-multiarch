/*****************************************************************
*       uart.c
*       by Zhiyi Huang, hzy@cs.otago.ac.nz
*       University of Otago
*
********************************************************************/



#include "types.h"
#include "defs.h"
#include "memlayout.h"
#include "traps.h"
#include "arm.h"

#define GPFSEL0			0xFE200000
#define GPFSEL1			0xFE200004
#define GPFSEL2			0xFE200008
#define GPFSEL3			0xFE20000C
#define	GPFSEL4			0xFE200010
#define	GPFSEL5			0xFE200014
#define GPSET0  		0xFE20001C
#define GPSET1			0xFE200020
#define GPCLR0  		0xFE200028
#define GPCLR1			0xFE20002C
#define GPPUD       		0xFE200094
#define GPPUDCLK0   		0xFE200098
#define GPPUDCLK1		0xFE20009C

#define AUX_IRQ			0xFE215000
#define AUX_ENABLES     	0xFE215004
#define AUX_MU_IO_REG   	0xFE215040
#define AUX_MU_IER_REG  	0xFE215044
#define AUX_MU_IIR_REG  	0xFE215048
#define AUX_MU_LCR_REG  	0xFE21504C
#define AUX_MU_MCR_REG  	0xFE215050
#define AUX_MU_LSR_REG  	0xFE215054
#define AUX_MU_MSR_REG  	0xFE215058
#define AUX_MU_SCRATCH  	0xFE21505C
#define AUX_MU_CNTL_REG 	0xFE215060
#define AUX_MU_STAT_REG 	0xFE215064
#define AUX_MU_BAUD_REG 	0xFE215068

// claude: PL011 (UART0) registers - real BCM2835 offsets (DEVSPACE +
// 0x201000, same +0x201000-from-peripheral-base convention as every
// other fork here). Written unconditionally alongside the Mini-UART
// below (see uartinit()/uartputc()'s own comments for why) - a real,
// always-physically-present peripheral on this SoC, so writing to it
// is safe regardless of environment; deliberately NOT gated behind any
// QEMU/real-hardware runtime check.
#define UART0_DR	0xFE201000
#define UART0_FR	0xFE201018
#define UART0_IBRD	0xFE201024
#define UART0_FBRD	0xFE201028
#define UART0_LCRH	0xFE20102C
#define UART0_CR	0xFE201030
#define UART0_IMSC	0xFE201038
#define UART0_ICR	0xFE201044

void
setgpioval(uint func, uint val)
{
	uint sel, ssel, rsel;

	if(func > 53) return;
	sel = func >> 5;
	ssel = GPSET0 + (sel << 2);
	rsel = GPCLR0 + (sel << 2);
	sel = func & 0x1f;
	if(val == 0) outw(rsel, 1<<sel);
	else outw(ssel, 1<<sel);
}


void
setgpiofunc(uint func, uint alt)
{
	uint sel, data, shift;

	if(func > 53) return;
	sel = 0;
	while (func > 10) {
	    func = func - 10;
	    sel++;
	}
	sel = (sel << 2) + GPFSEL0;
	data = inw(sel);
	shift = func + (func << 1);
	data &= ~(7 << shift);
	data |= alt << shift;
	outw(sel, data);
}


// claude: writes to PL011 (UART0) unconditionally, alongside the
// Mini-UART below - see pl011init()'s own comment for why this is
// always safe. Deliberately not gated behind GPIO pin-muxing: GPIO
// 14/15 stay muxed to ALT5 (Mini-UART) throughout, matching this
// port's own real-hardware design (this fork's actual primary console
// path) - remuxing them to ALT0 for PL011 would physically disconnect
// the Mini-UART on real hardware. Confirmed empirically that QEMU's
// own "-M raspi1ap" PL011 model doesn't check GPIO alt-function state
// at all before accepting writes and routing them to its console
// chardev, so no remux is needed for the QEMU side either.
static void
pl011putc(uint c)
{
	if(c=='\n') {
		while(inw(UART0_FR) & (1 << 5)) ; // wait while TXFF
		outw(UART0_DR, 0x0d); // add CR before LF
	}
	while(inw(UART0_FR) & (1 << 5)) ; // wait while TXFF
	outw(UART0_DR, c);
}

// claude: PL011's own RX FIFO - merged into uartgetc() below rather
// than kept separate, so consoleintr()'s existing uartgetc() callback
// picks up characters from either UART without needing its own
// dispatch. Safe to poll unconditionally, same reasoning as
// pl011putc() above.
static int
pl011getc(void)
{
	if(inw(UART0_FR) & (1 << 4)) return -1; // RXFE (receive FIFO empty)
	return inw(UART0_DR) & 0xFF;
}

void
uartputc(uint c)
{
	pl011putc(c);

	if(c=='\n') {
		while(1) if(inw(AUX_MU_LSR_REG) & 0x20) break;
		outw(AUX_MU_IO_REG, 0x0d); // add CR before LF
	}
	while(1) if(inw(AUX_MU_LSR_REG) & 0x20) break;
	outw(AUX_MU_IO_REG, c);
}

static int
uartgetc(void)
{
	int c;

	c = pl011getc();
	if(c != -1) return c;

	if(inw(AUX_MU_LSR_REG)&0x1) return inw(AUX_MU_IO_REG);
	else return -1;
}

void
enableirqminiuart(void)
{
        intctrlregs *ip;

        ip = (intctrlregs *)INT_REGS_BASE;
        ip->gpuenable[0] |= (1 << 29);   // enable the miniuart through Aux
        // claude: PL011 is real IRQ 57 = bank 1, bit 25 - trap.c's own
        // enable_intrs() (dead code, never called) already had a
        // commented-out "ip->gpuenable[1] |= 1 << 25" anticipating
        // exactly this. Safe to enable unconditionally alongside the
        // Mini-UART, same reasoning as pl011putc()/pl011init() above.
        ip->gpuenable[1] |= (1 << 25);
}


void
miniuartintr(void)
{
  // claude: clear PL011's own RX interrupt too (harmless if it wasn't
  // set) - consoleintr() below drains uartgetc(), which now checks
  // PL011 first regardless of which UART's IRQ actually fired.
  outw(UART0_ICR, 1 << 4); // clear RX interrupt (RXIC)
  consoleintr(uartgetc);
}

// claude: unconditional PL011 init, called from uartinit() below on
// every boot - real, always-physically-present peripheral on this SoC
// (BCM2835 includes a full PL011 IP block on every model, whether or
// not GPIO routes it anywhere in particular), so enabling and
// configuring it is safe regardless of environment. See pl011putc()'s
// own comment for why this doesn't touch GPIO pin muxing at all.
static void
pl011init(void)
{
	outw(UART0_CR, 0); // disable UART0 while configuring it

	// real hardware's PL011 reference clock is 48MHz; harmless if
	// QEMU's own emulated PL011 doesn't model baud timing at all.
	outw(UART0_IBRD, 26); // 48000000 / (16 * 115200) = 26.04
	outw(UART0_FBRD, 3);  // 0.04 * 64 + 0.5 = 3
	outw(UART0_LCRH, (3 << 5)); // 8 bits, no parity, FIFOs disabled
	outw(UART0_IMSC, 1 << 4); // enable RX interrupt (RXIM)
	outw(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9)); // UARTEN | TXE | RXE
}

void
uartinit(void)
{
	pl011init();

	outw(AUX_ENABLES, 1);
	outw(AUX_MU_CNTL_REG, 0);
	outw(AUX_MU_LCR_REG, 0x3);
	outw(AUX_MU_MCR_REG, 0);
	outw(AUX_MU_IER_REG, 0x1);
	outw(AUX_MU_IIR_REG, 0xC7);
	outw(AUX_MU_BAUD_REG, 270); // (250,000,000/(115200*8))-1 = 270

	setgpiofunc(14, 2); // gpio 14, alt 5
	setgpiofunc(15, 2); // gpio 15, alt 5

	outw(GPPUD, 0);
	delay(10);
	outw(GPPUDCLK0, (1 << 14) | (1 << 15) );
	delay(10);
	outw(GPPUDCLK0, 0);

	outw(AUX_MU_CNTL_REG, 3);
	enableirqminiuart();
}
