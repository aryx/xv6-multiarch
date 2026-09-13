// driver for ARM PrimeCell UART (PL011)
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "memlayout.h"

//static volatile uint *uart_base;
void isr_uart (struct trapframe *tf, int idx);

// ***************************************************************************

// Loop <delay> times
inline void udelay(unsigned int count)
{
  asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
     : "=r"(count): [count]"0"(count) : "cc");
}

// need to change the code to handle kernelbase relocation
/*
void uart_init ( void )
{
  unsigned int ra;

  // enable Mini UART
  write32(AUX_ENABLES,1);
  
  // Mini UART Interrupt
  // bit0: 0/1 = Disable/Enable transmit interrupt
  // bit1: 0/1 = Disable/Enable receive interrupt
  // disable receive & transmit UART interrupt
  write32(AUX_MU_IER_REG,0);
  
  // Mini UART Extra Control
  // bit0: 0/1 = mini UART receiver Disable/Enable
  // bit1: 0/1 = mini UART transmitter Disable/Enable
  // disable UART receive & transmit
  write32(AUX_MU_CNTL_REG,0);
  
  // Mini UART Line Control
  // bit0: 0=7bit mode | 1=8bit mode # bit1: unk
  write32(AUX_MU_LCR_REG,3);

  // Mini UART Modem Control
  // bit1: 0=UART1_RTS line is high | 1=UART1_RTS line is low
  write32(AUX_MU_MCR_REG,0);
  
  // Mini UART Interrupt
  // disable receive & transmit interrupt
  write32(AUX_MU_IER_REG,0);

  // Mini UART Interrupt Identify
  // bit0:R 0/1 Interrupt pending / no interrupt pending
  // bit1-2: 2+4 = Clear receive FIFO + Clear transmit FIFO
  write32(AUX_MU_IIR_REG,0xC6);
  
  // Mini UART Baudrate
  // bit0-15: baud rate counter
  write32(AUX_MU_BAUD_REG,270);

  // Setup the GPIO pin 14 && 15
  ra=read32(GPFSEL1);
  ra&=~(7<<12); //gpio14
  ra|=2<<12;    //alt5
  ra&=~(7<<15); //gpio15
  ra|=2<<15;    //alt5
  write32(GPFSEL1,ra);
  
  // Disable pull up/down for all GPIO pins & delay for 150 cycles
  write32(GPPUD,0);
  xdelay(150);
  //for(ra=0;ra<150;ra++) dummy(ra);
  
  // Disable pull up/down for pin 14,15 & delay for 150 cycles
  write32(GPPUDCLK0,(1<<14)|(1<<15));
  xdelay(150);
  //for(ra=0;ra<150;ra++) dummy(ra);
  
  // Write 0 to GPPUDCLK0 to make it take effect.
  write32(GPPUDCLK0,0);

  // Mini UART Extra Control
  // enable UART receive & transmit
  write32(AUX_MU_CNTL_REG,3);
  
}
*/

//******************************************************************************
// claude: switched from the Mini-UART (AUX_*) to the PL011 (UART0_*) -
// see start.c's own "_uart_init"/"_uart_putc" for the full writeup of
// why (QEMU's "-M raspi2b" doesn't wire the Mini-UART to any chardev at
// all). Must match start.c's own choice of peripheral - this is the
// same physical UART, just accessed post-MMU via its KERNBASE-offset
// virtual alias instead of pre-MMU via its raw physical address.
void uartputc ( int byte )
{
  while(read32(UART0_FR+KERNBASE)&(1<<5)) ; // wait while TXFF
  write32(UART0_DR+KERNBASE, byte);
}

void uart_puts(const char *s)
{
  while (*s) {
    if (*s == '\n')
      uartputc('\r');
    uartputc(*s++);
  }
}

int uartgetc ()
{
  if(read32(UART0_FR+KERNBASE)&(1<<4)) { // RXFE (receive FIFO empty)
    return -1;
  } else {
    return(read32(UART0_DR+KERNBASE)&0xFF);
  }
}

void print_hex(uint val) {
    char digit[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    char number[8] = {'0','0','0','0','0','0','0','0'};
    uint base = 16;
    int i = 7;
    uartputc('0');
    uartputc('x');

    while(val > 0) {
        number[i--] = digit[val % base];
        val /= base;

    }
    for(i=0;i<8;++i) {
        uartputc(number[i]);
    }
    uartputc('\r');
    uartputc('\n');
}

void hexstrings ( unsigned int d )
{
  unsigned int rb;
  unsigned int rc;

  rb=32;
  while(1)
  {
    rb-=4;
    rc=(d>>rb)&0xF;
    if(rc>9) rc+=0x37; else rc+=0x30;
    uartputc(rc);
    if(rb==0) break;
  }
  uartputc(0x20);
}

void hexstring ( unsigned int d )
{
  hexstrings(d);
  uartputc(0x0D);
  uartputc(0x0A);
}

//***********************************************************************

// enable the receive (interrupt) for uart (after PIC has initialized)
//
// claude: switched from the Mini-UART's own AUX_MU_IER_REG/IRQ_ENABLE1
// (bit 29, GPU0 bank) to the PL011's own UARTIMSC register and its real
// interrupt number (57, GPU1 bank) - matching bug 10's console switch
// (see start.c/device/uart.c's own uartputc/uartgetc). RXIM (bit 4) and
// RTIM (bit 6, the receive-timeout interrupt - needed so a single
// received byte sitting below the FIFO trigger level still eventually
// generates an interrupt) are the standard PL011 receive-interrupt
// bits. pic_enable() now correctly routes GPU1-range interrupt numbers
// to the real ENABLE_IRQS_2 register itself (see device/gic.c's own
// comment) - see notes_arch_armv7_rpi.txt's own "Gap 2" for the full
// investigation this fixes.
void uart_enable_rx ()
{
    write32(UART0_IMSC+KERNBASE, (1<<4)|(1<<6)); // RXIM | RTIM

    pic_enable(PIC_UART0_PL011, isr_uart);
}


void isr_uart (struct trapframe *tf, int idx)
{
    //if (uart_base[UART_MIS] & UART_RXI) {
        consoleintr(uartgetc);
    //}

    // clear the interrupt
    //uart_base[UART_ICR] = UART_RXI | UART_TXI;
}
