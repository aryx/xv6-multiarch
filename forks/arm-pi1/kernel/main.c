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
extern char bss_start[]; // first address of .bss - see kernel.ld/kernel-qemu.ld
extern pde_t *kpgdir;
extern FBI fbinfo;
extern volatile uint *mailbuffer;

// claude: added 2026-09-09 while wiring up csud/ (the vendored USB
// driver). This kernel never zeroes .bss anywhere - proven by
// console.c's own pre-existing "panicked = 0; // must initialize in
// code since the compiler does not" comment/workaround, and confirmed
// as a REAL, currently-live bug by CSUD: its internal device-count
// globals (source/usbd/usbd.c's own zero-initialized statics) came up
// as leftover nonzero garbage left over in RAM from the bootloader/
// firmware/prior boot stage, which made KeyboardCount() appear
// nonzero from the very first call - permanently short-circuiting
// keyboard.s's own "no keyboard yet, poll for one" path (see
// notes_arch_arm_pi1.txt). A C runtime is entitled to assume a
// zero-initialized global really is zero at startup; every other
// file-scope "= 0" global in this kernel and every future one placed
// in .bss had exactly the same latent bug, just not yet triggered by
// visible behavior - this is a universal correctness fix, identical on
// real hardware and QEMU (memory contents predating a fresh boot are
// unpredictable on real hardware too, just less consistently
// reproducible than under emulation), not a QEMU-only workaround.
static void
bsszero(void)
{
  char *p;

  for(p = bss_start; p < end; p++)
    *p = 0;
}

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

int cmain()
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
  if(mailbuffer[1] != 0x80000000) cprintf("new error readmailbox\n");
  else 
cprintf("ARM memory is %x %x\n", mailbuffer[MB_HEADER_LENGTH + TAG_HEADER_LENGTH], mailbuffer[MB_HEADER_LENGTH + TAG_HEADER_LENGTH+1]);

  pinit();
  tvinit();
  cprintf("it is ok after tvinit\n");
  binit();
cprintf("it is ok after binit\n");
  fileinit();
cprintf("it is ok after fileinit\n");
  iinit();
cprintf("it is ok after iinit\n");
  ideinit();
cprintf("it is ok after ideinit\n");
  // claude: USB keyboard, wired up 2026-09-09 - see csud/ (the
  // vendored CSUD driver, previously an unused prebuilt libcsud.a with
  // no caller anywhere in this tree) and notes_arch_arm_pi1.txt. Not
  // gated like the framebuffer's fb_ready: a failed/absent USB
  // controller (or no keyboard plugged in, real hardware's own normal
  // case) is not an error - the UART/serial console keeps working
  // exactly as before either way, so this only prints, never loops.
  if(UsbInitialise() != 0)
    cprintf("USB: not available (continuing on UART console only)\n");
  timer3init();
  kinit2(P2V(8*1024*1024), P2V(PHYSTOP));
cprintf("it is ok after kinit2\n");
  userinit();
cprintf("it is ok after userinit\n");
  scheduler();


  NotOkLoop();

  return 0;
}
