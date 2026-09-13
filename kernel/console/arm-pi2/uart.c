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

#define GPFSEL0			(MMIO_VA+0x200000)
#define GPFSEL1			(MMIO_VA+0x200004)
#define GPFSEL2			(MMIO_VA+0x200008)
#define GPFSEL3			(MMIO_VA+0x20000C)
#define	GPFSEL4			(MMIO_VA+0x200010)
#define	GPFSEL5			(MMIO_VA+0x200014)
#define GPSET0  		(MMIO_VA+0x20001C)
#define GPSET1			(MMIO_VA+0x200020)
#define GPCLR0  		(MMIO_VA+0x200028)
#define GPCLR1			(MMIO_VA+0x20002C)
#define GPPUD       		(MMIO_VA+0x200094)
#define GPPUDCLK0   		(MMIO_VA+0x200098)
#define GPPUDCLK1		(MMIO_VA+0x20009C)

#define AUX_IRQ			(MMIO_VA+0x215000)
#define AUX_ENABLES     	(MMIO_VA+0x215004)
#define AUX_MU_IO_REG   	(MMIO_VA+0x215040)
#define AUX_MU_IER_REG  	(MMIO_VA+0x215044)
#define AUX_MU_IIR_REG  	(MMIO_VA+0x215048)
#define AUX_MU_LCR_REG  	(MMIO_VA+0x21504C)
#define AUX_MU_MCR_REG  	(MMIO_VA+0x215050)
#define AUX_MU_LSR_REG  	(MMIO_VA+0x215054)
#define AUX_MU_MSR_REG  	(MMIO_VA+0x215058)
#define AUX_MU_SCRATCH  	(MMIO_VA+0x21505C)
#define AUX_MU_CNTL_REG 	(MMIO_VA+0x215060)
#define AUX_MU_STAT_REG 	(MMIO_VA+0x215064)
#define AUX_MU_BAUD_REG 	(MMIO_VA+0x215068)

// claude: PL011 (UART0), not the mini-UART/AUX above - added because
// QEMU's raspi2b "-nographic" console is backed by PL011, not the
// mini-UART (same finding as forks/arm-pi1's own uart.c - see
// notes_arch_arm_pi1.txt bug 5/6 and notes_arch_arm_pi2.txt's own "Bug
// 4"). Real hardware's own mini-UART path below is untouched - PL011
// output is added in front of it, not instead of it, so both still run
// on real hardware; PL011 is a real, always-present BCM283x peripheral
// there too, so writing to it is harmless even where nothing reads it.
#define UART0_DR		(MMIO_VA+0x201000)
#define UART0_FR		(MMIO_VA+0x201018)
#define UART0_IBRD		(MMIO_VA+0x201024)
#define UART0_FBRD		(MMIO_VA+0x201028)
#define UART0_LCRH		(MMIO_VA+0x20102C)
#define UART0_CR		(MMIO_VA+0x201030)
#define UART0_IMSC		(MMIO_VA+0x201038)
#define UART0_ICR		(MMIO_VA+0x201044)

static void
pl011putc(uint c)
{
	while (inw(UART0_FR) & (1 << 5)) ; // wait while TX FIFO full
	outw(UART0_DR, c);
}

static void
pl011init(void)
{
	outw(UART0_CR, 0);          // disable UART0
	outw(UART0_IBRD, 26);       // 115200 baud @ 48MHz UART clock
	outw(UART0_FBRD, 3);
	outw(UART0_LCRH, (3 << 5)); // 8n1, FIFOs enabled
	outw(UART0_IMSC, (1 << 4)); // unmask RXIM (RX interrupt)
	outw(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9)); // UARTEN | TXE | RXE
}

extern unsigned int core_clock_freq, boardmodel, boardrevision;

void
setgpioval(uint pin, uint val)
{
	uint sel, ssel, rsel, shift;

	if(pin > 53) return;
	if(pin >= 32) sel = 1; else sel = 0;
	ssel = GPSET0 + (sel << 2);
	rsel = GPCLR0 + (sel << 2);
	if(sel) shift = (pin - 32) & 0x1f;
	else shift = pin & 0x1f;
	if(val == 0) outw(rsel, 1<<shift);
	else outw(ssel, 1<<shift);
}


void
setgpiofunc(uint pin, uint func)
{
	uint sel, data, shift;

	if(pin > 53) return;
	sel = 0;
	while (pin > 10) {
	    pin = pin - 10;
	    sel++;
	}
	sel = (sel << 2) + GPFSEL0;
	data = inw(sel);
	shift = pin + (pin << 1);
	data &= ~(7 << shift);
	outw(sel, data);
	data |= func << shift;
	outw(sel, data);
}


void
uartputc(uint c)
{
	if(c=='\n') pl011putc(0x0d); // add CR before LF
	pl011putc(c);

	if(c=='\n') {
		while(1) if(inw(AUX_MU_LSR_REG) & 0x20) break;
		outw(AUX_MU_IO_REG, 0x0d); // add CR before LF
	}
	while(1) if(inw(AUX_MU_LSR_REG) & 0x20) break;
	outw(AUX_MU_IO_REG, c);
}

// claude: PL011 first, same reason as pl011putc() in uartputc() above -
// QEMU's raspi2b delivers typed keystrokes to PL011, not the
// mini-UART, so uartgetc() needs to check both. Real hardware keeps
// working identically - if PL011 has nothing pending, this falls
// through to the original mini-UART check unchanged.
static int
pl011getc(void)
{
	if (inw(UART0_FR) & (1 << 4)) return -1; // RX FIFO empty
	return inw(UART0_DR) & 0xff;
}

static int
uartgetc(void)
{
	int c = pl011getc();
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
        ip->gpuenable[1] |= (1 << 25);   // enable UART0/PL011 (IRQ 57)
}


void
miniuartintr(void)
{
  outw(UART0_ICR, 1 << 4); // clear PL011's own RX interrupt latch too
  consoleintr(uartgetc);
}

void
uartinit(void)
{
    unsigned int v;

	pl011init();

	outw(AUX_ENABLES, 1);
	outw(AUX_MU_CNTL_REG, 0);
	outw(AUX_MU_LCR_REG, 0x3);
	outw(AUX_MU_MCR_REG, 0);
	outw(AUX_MU_IER_REG, 0x1);
	outw(AUX_MU_IIR_REG, 0xC7);
	v = (core_clock_freq/(115200*8)) - 1;
	outw(AUX_MU_BAUD_REG, v);

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
