/*****************************************************************
*       mmu.c
*       by Zhiyi Huang, hzy@cs.otago.ac.nz
*       University of Otago
*
********************************************************************/


#include "types.h"
#include "defs.h"
#include "memlayout.h"
#include "mmu.h"


#define IO_BASE 0x20000000

// BCM2835 ARM peripherals - Page 89
#define GPFSEL1         (IO_BASE + 0x00200004) // GPIO Function Select 1
#define GPPUD           (IO_BASE + 0x00200094) // GPIO Pin Pull-up/down Enable
#define GPPUDCLK0       (IO_BASE + 0x00200098) // GPIO Pin Pull-up/down Enable Clock 0

// BCM2835 ARM peripherals - Page 8-20
#define AUX_ENABLES     (IO_BASE + 0x00215004) // Auxiliary enables
#define AUX_MU_IO_REG   (IO_BASE + 0x00215040) // Mini Uart I/O Data
#define AUX_MU_IER_REG  (IO_BASE + 0x00215044) // Mini Uart Interrupt Enable
#define AUX_MU_IIR_REG  (IO_BASE + 0x00215048) // Mini Uart Interrupt Identify 
#define AUX_MU_LCR_REG  (IO_BASE + 0x0021504C) // Mini Uart Line Control
#define AUX_MU_MCR_REG  (IO_BASE + 0x00215050) // Mini Uart Modem Control
#define AUX_MU_LSR_REG  (IO_BASE + 0x00215054) // Mini Uart Line Status
//#define AUX_MU_MSR_REG  (IO_BASE + 0x00215058) // Mini Uart Modem Status
//#define AUX_MU_SCRATCH  (IO_BASE + 0x0021505C) // Mini Uart Scratch
#define AUX_MU_CNTL_REG (IO_BASE + 0x00215060) // Mini Uart Extra Control
//#define AUX_MU_STAT_REG (IO_BASE + 0x00215064) // Mini Uart Extra Status
#define AUX_MU_BAUD_REG (IO_BASE + 0x00215068) // Mini Uart Baudrate

// memory mapped i/o access macros
#define write32(addr, v)      (*((volatile unsigned long  *)(addr)) = (unsigned long)(v))
#define read32(addr)          (*((volatile unsigned long  *)(addr)))


// Loop <delay> times
inline void xdelay(unsigned int count)
{
  asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
     : "=r"(count): [count]"0"(count) : "cc");
}

void _uart_init ( void )
{
  unsigned int ra;

  // enable Mini UART
  write32(AUX_ENABLES,1);
  write32(AUX_MU_CNTL_REG,0);
  write32(AUX_MU_LCR_REG,3);
  write32(AUX_MU_MCR_REG,0);
  write32(AUX_MU_IER_REG,0x1);

  // Mini UART Interrupt Identify
  // bit0:R 0/1 Interrupt pending / no interrupt pending
  // bit1-2: 2+4 = Clear receive FIFO + Clear transmit FIFO
  write32(AUX_MU_IIR_REG,0xC7);
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

void _uart_putc ( unsigned int c )
{
  while(1) {
    if(read32(AUX_MU_LSR_REG)&0x20) break;
  }
  write32(AUX_MU_IO_REG,c);
}

void _puts(const char *s)
{
  while (*s) {
    if (*s == '\n')
      _uart_putc('\r');
    _uart_putc(*s++);
  }
}


void __uart_putc ( unsigned int c )
{
  while(1) {
    if(read32(0x20215054 + 0xDE000000)&0x20) break;
  }
  write32(0x20215040 + 0xDE000000,c);
}

void __puts(const char *s)
{
  while (*s) {
    if (*s == '\n')
      __uart_putc('\r');
    __uart_putc(*s++);
  }
}


void mmuinit0(void)
{
  pde_t *l1;
	pte_t *l2;
  uint pa, va, *p;
  
  _uart_init();
  _puts("..............\nBoot starting...\n");


	// diable mmu
	// use inline assembly here as there is a limit on 
	// branch distance after mmu is disabled
	asm volatile("mrc p15, 0, r1, c1, c0, 0\n\t"
		"bic r1,r1,#0x00000004\n\t"
		"bic r1,r1,#0x00001000\n\t"
		"bic r1,r1,#0x00000800\n\t"
		"bic r1,r1,#0x00000001\n\t"
		"mcr p15, 0, r1, c1, c0, 0\n\t"
		"mov r0, #0\n\t"
		"mcr p15, 0, r0, c7, c7, 0\n\t"
		"mcr p15, 0, r0, c8, c7, 0\n\t"
		::: "r0", "r1", "cc", "memory");


	for(p=(uint *)0x3000; p<(uint *)0x8000; p++) *p = 0;

        l1 = (pde_t *) K_PDX_BASE;
        l2 = (pte_t *) K_PTX_BASE;

        // map all of ram at KERNBASE
	va = KERNBASE;
	for(pa = PA_START; pa < PA_START+RAMSIZE; pa += MBYTE){
                l1[PDX(va)] = pa|DOMAIN0|PDX_AP(K_RW)|SECTION|CACHED|BUFFERED;
                va += MBYTE;
        }

        // identity map first MB of ram so mmu can be enabled
        l1[PDX(PA_START)] = PA_START|DOMAIN0|PDX_AP(K_RW)|SECTION|CACHED|BUFFERED;

        // map IO region
        va = DEVSPACE;
        for(pa = PHYSIO; pa < PHYSIO+IOSIZE; pa += MBYTE){
                l1[PDX(va)] = pa|DOMAIN0|PDX_AP(K_RW)|SECTION;
                va += MBYTE;
        }

	// map GPU memory
	va = GPUMEMBASE;
	for(pa = GPUMEMBASE; pa < (uint)GPUMEMBASE+(uint)GPUMEMSIZE; pa += MBYTE){
		l1[PDX(va)] = pa|DOMAIN0|PDX_AP(K_RW)|SECTION;
		va += MBYTE;
	}

        // double map exception vectors at top of virtual memory
        va = HVECTORS;
        l1[PDX(va)] = (uint)l2|DOMAIN0|COARSE;
        l2[PTX(va)] = PA_START|PTX_AP(K_RW)|SMALL;


	asm volatile("mov r1, #1\n\t"
                "mcr p15, 0, r1, c3, c0\n\t"
                "mov r1, #0x4000\n\t"
                "mcr p15, 0, r1, c2, c0\n\t"
                "mrc p15, 0, r0, c1, c0, 0\n\t"
                "mov r1, #0x00002000\n\t"
                "orr r1, #0x00000004\n\t"
                "orr r1, #0x00001000\n\t"
                "orr r1, #0x00000001\n\t"
		"orr r0, r1\n\t"
		"mcr p15, 0, r0, c1, c0, 0\n\t"
		"mov r1, #1\n\t"
		"mcr p15, 0, r1, c15, c12, 0\n\t"
                ::: "r0", "r1", "cc", "memory");

  //__puts("mmu started ...\n");
  
    uint val = 0;

    // flush all TLB
    asm("MCR p15, 0, %[r], c7, c7, 0" : :[r]"r" (val):);
    asm("MCR p15, 0, %[r], c8, c7, 0" : :[r]"r" (val):);
  //__puts("mmu flush ...\n");

}

void
mmuinit1(void)
{
  pde_t *l1;
	uint va1, va2;

  l1 = (pde_t*)(K_PDX_BASE);

  // undo identity map of first MB of ram
  l1[PDX(PA_START)] = 0;

	// drain write buffer; writeback data cache range [va, va+n]
	va1 = (uint)&l1[PDX(PA_START)];
	va2 = va1 + sizeof(pde_t);
	va1 = va1 & ~((uint)CACHELINESIZE-1);
	va2 = va2 & ~((uint)CACHELINESIZE-1);
	flush_dcache(va1, va2);

  // invalidate TLB; DSB barrier used
	flush_tlb();
  //__puts("mmu mmuinit1 flush ...\n");
}

