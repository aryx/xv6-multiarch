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

// claude: PL011 - a second, real, always-physically-present UART on
// this SoC (this port's own board has it wired to GPIO14/15 ALT0,
// versus the Mini-UART's own ALT5 on the same pins - mutually
// exclusive on real hardware, but harmless to touch unconditionally:
// see pl011putc()'s own comment). QEMU's raspi1ap machine has no
// chardev backend for the Mini-UART at all ("info qtree" shows
// bcm2835-aux with an empty chardev), only for this PL011 - added so
// this fork gets a working QEMU console the same way forks/arm-pi1's
// own uart.c does (see notes_arch_arm_pi1.txt bug 4).
#define UART0_DR		0xFE201000
#define UART0_FR		0xFE201018
#define UART0_IBRD		0xFE201024
#define UART0_FBRD		0xFE201028
#define UART0_LCRH		0xFE20102C
#define UART0_CR		0xFE201030
#define UART0_IMSC		0xFE201038
#define UART0_ICR		0xFE201044

// memory mapped i/o access macros
#define write32(addr, v)      (*((volatile unsigned long  *)(addr)) = (unsigned long)(v))
#define read32(addr)          (*((volatile unsigned long  *)(addr)))


void
setgpioval(uint func, uint val)
{
	uint sel, ssel, rsel;

	if(func > 53) return;
	sel = func >> 5;
	ssel = GPSET0 + (sel << 2);
	rsel = GPCLR0 + (sel << 2);
	sel = func & 0x1f;
	if(val == 0) write32(rsel, 1<<sel);
	else write32(ssel, 1<<sel);
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
	write32(sel, data);
}


// claude: unconditional - real hardware and QEMU alike. PL011 is a
// real, always-physically-present peripheral on this SoC regardless
// of which UART's ALT function the GPIO pins are currently muxed to
// (see the #define block above) - writing to its own registers never
// affects anything else, real hardware included, since the pin mux is
// what decides whether those writes ever reach an actual wire, not
// whether the peripheral itself accepts them. No runtime detection
// needed here at all: this is safe unconditionally, unlike the
// framebuffer/USB fixes elsewhere in this port, which do need it.
static void
pl011putc(uint c)
{
	while(inw(UART0_FR) & (1 << 5))
		;
	write32(UART0_DR, c);
}

static int
pl011getc(void)
{
	if(inw(UART0_FR) & (1 << 4))
		return -1;
	return inw(UART0_DR);
}

static void
pl011init(void)
{
	write32(UART0_CR, 0);
	write32(UART0_IBRD, 26);   // 115200 baud @ 48MHz uartclk
	write32(UART0_FBRD, 3);
	write32(UART0_LCRH, 3 << 5);   // 8N1, no FIFO
	write32(UART0_IMSC, 1 << 4);   // RXIM
	write32(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));   // UARTEN|TXE|RXE
}

void
uartputc(uint c)
{
	pl011putc(c);
	if(c=='\n') {
		while(1) if(inw(AUX_MU_LSR_REG) & 0x20) break;
		write32(AUX_MU_IO_REG, 0x0d); // add CR before LF
	}
	while(1) if(inw(AUX_MU_LSR_REG) & 0x20) break;
	write32(AUX_MU_IO_REG, c);
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
  ip->gpuenable[1] |= (1 << 25);   // claude: PL011's real hardware IRQ (57 = bank1 bit25)
}


void
miniuartintr(void)
{
  write32(UART0_ICR, 1 << 4);   // claude: clear PL011's RX interrupt too
  consoleintr(uartgetc);
}

void ___puts(const char *s)
{
  while (*s) {
    if (*s == '\n')
      uartputc('\r');
    uartputc(*s++);
  }
}

void 
uartinit(void)
{
  /*
  write32(AUX_ENABLES, 1);
	write32(AUX_MU_CNTL_REG, 0);
	write32(AUX_MU_LCR_REG, 0x3);
	write32(AUX_MU_MCR_REG, 0);
	write32(AUX_MU_IER_REG, 0x1);
	write32(AUX_MU_IIR_REG, 0xC7);
	write32(AUX_MU_BAUD_REG, 270); // (250,000,000/(115200*8))-1 = 270
	
  setgpiofunc(14, 2); // gpio 14, alt 5
	setgpiofunc(15, 2); // gpio 15, alt 5

	write32(GPPUD, 0);
	delay(10);
	write32(GPPUDCLK0, (1 << 14) | (1 << 15) );
	delay(10);
	write32(GPPUDCLK0, 0);

	write32(AUX_MU_CNTL_REG, 3);
  */

	pl011init();
	enableirqminiuart();
}
