/*****************************************************************
*       main.c
*       by Zhiyi Huang, hzy@cs.otago.ac.nz
*       University of Otago
*
********************************************************************/


#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "arm.h"
#include "mailbox.h"

extern char end[]; // first address after kernel loaded from ELF file
extern char bss_start[]; // first address of .bss - see kernel.ld

// claude: this kernel never zeroes .bss anywhere - proven by console.c's
// own pre-existing "panicked = 0; // must initialize in code since the
// compiler does not" comment/workaround. A real, universal correctness
// fix (real hardware's own pre-boot RAM contents are just as
// unpredictable, only less consistently reproducible than under
// emulation), not a QEMU-only workaround - identical fix and identical
// root cause to forks/arm-pi1's own main.c (notes_arch_arm_pi1.txt,
// USB keyboard bring-up: a zero-initialized global reading back as
// nonzero garbage broke a "no device yet" check there).
static void
bsszero(void)
{
  char *p;

  for(p = bss_start; p < end; p++)
    *p = 0;
}
extern pde_t *kpgdir;
extern FBI fbinfo;
extern volatile uint *mailbuffer;

void OkLoop()
{
   setgpiofunc(16, 1); // gpio 16, set as an output
   while(1){
        setgpioval(16, 0);
        delay(1000000);
        setgpioval(16, 1);
        delay(1000000);
   }
}

void NotOkLoop()
{
   setgpiofunc(16, 1); // gpio 16, set as an output
   while(1){
        setgpioval(16, 0);
        delay(100000);
        setgpioval(16, 1);
        delay(100000);
   }
}

void machinit(void)
{
    memset(cpus, 0, sizeof(struct cpu)*NCPU);
}


void enableirqminiuart(void);


int cmain( uint r0)
{

  bsszero();
  mmuinit1();
  machinit();
  uartinit();
  dsb_barrier();
  consoleinit();
  cprintf("\nHello World from xv6\n");
  kinit1(end, P2V(8*1024*1024));  // reserve 8 pages for PGDIR
  kpgdir=p2v(K_PDX_BASE);

  mailboxinit();
  create_request(mailbuffer, MPI_TAG_GET_ARM_MEMORY, 8, 0, 0);
  writemailbox((uint *)mailbuffer, 8);
  readmailbox(8);
  if(mailbuffer[1] != 0x80000000) 
    cprintf("new error readmailbox\n");
  else 
    cprintf("ARM memory is %x %x\n", mailbuffer[MB_HEADER_LENGTH + TAG_HEADER_LENGTH], mailbuffer[MB_HEADER_LENGTH + TAG_HEADER_LENGTH+1]);

  pinit();
  tvinit();
  //cprintf("it is ok after tvinit\n");
  binit();
//cprintf("it is ok after binit\n");
  fileinit();
//cprintf("it is ok after fileinit\n");
  iinit();
//cprintf("it is ok after iinit\n");
  ideinit();
//cprintf("it is ok after ideinit\n");
  // claude: USB keyboard - see timer.c's own usbkbdgetc()/timer3intr()
  // hookup. Not gated like the framebuffer's fb_ready: a failed/absent
  // USB controller (or no keyboard plugged in, real hardware's own
  // normal case) is not an error - the UART/serial console keeps
  // working exactly as before either way. Identical fix to
  // forks/arm-pi1's own main.c (notes_arch_arm_pi1.txt bug 8).
  if(UsbInitialise() != 0)
    cprintf("USB: not available (continuing on UART console only)\n");
  timer3init();
  kinit2(P2V(8*1024*1024), P2V(PHYSTOP));
//cprintf("it is ok after kinit2\n");
  userinit();
//cprintf("it is ok after userinit\n");
  scheduler();


  NotOkLoop();

  return 0;
}
