#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

extern struct proc *proc;

static volatile uint *vic_base;

#define VIC_IRQPENDING_ARM 0
//#define VIC_IRQPENDING 0
#define VIC_IRQPENDING_GPU0 1
#define VIC_IRQPENDING_GPU1 2
// claude: added - real BCM2835/36 word offsets from VIC_BASE (0xB200),
// same source as the ones already above: ENABLE_IRQS_1/ENABLE_IRQS_2
// (word 4/5, GPU-routed IRQs 0-31/32-63) and DISABLE_IRQS_1/
// DISABLE_IRQS_2 (word 7/8) - see pic_enable()/pic_disable() below for
// why these are now needed (they were previously always routed through
// VIC_INTENABLE/VIC_INTCLEAR below, which are only valid for the
// ARM-local "basic" sources like the timer, not any GPU-routed
// interrupt - see notes_arch_armv7_rpi.txt's own "Gap 2"/pic_enable()'s
// own comment for the full story).
#define VIC_INTENABLE1  4
#define VIC_INTENABLE2  5
#define VIC_INTENABLE  6
#define VIC_INTDISABLE1 7
#define VIC_INTDISABLE2 8
#define VIC_INTCLEAR   9

// claude: was 32 - grown to 64 so isrs[] can hold a real GPU1-range IRQ
// number (57, the PL011's own - see PIC_UART0_PL011 in memlayout.h)
// alongside PIC_TIMER0 (0) and the old Mini-UART's PIC_UART0 (29).
#define NUM_INTSRC 64
static ISR isrs[NUM_INTSRC];

static void default_isr(struct trapframe *tf, int n) {
    (void)tf;
    cprintf ("unhandled interrupt: %d\n", n);
    //uart_puts("unhandled interrupt: ");
    //print_hex(n);
}

void gic_init(void *base) {
    int i;

    vic_base = base;
    vic_base[VIC_INTCLEAR] = 0xFFFFFFFF;

    for(i=0;i<NUM_INTSRC;++i) {
        isrs[i] = default_isr;
    }
}

void pic_enable(int n, ISR isr) {
    if ((n<0) || (n>=NUM_INTSRC)) {
        panic("invalid interrupt source");
    }

    cprintf ("pic_enable: %d\n", n);

    isrs[n] = isr;

    // claude: n==0 is PIC_TIMER0, the ARM-local timer - dispatched via a
    // completely separate path below (checking VIC_IRQPENDING_ARM, not a
    // GPU pending word at all), and genuinely belongs on the BASIC
    // enable register (bit 0 there really is "ARM Timer IRQ" on real
    // BCM2835/36 hardware). Every other n here is a real GPU-side
    // interrupt number and needs the corresponding GPU0 (0-31) or GPU1
    // (32-63) enable register instead - the original code wrote every n
    // (including the old Mini-UART's own IRQ 29) through the BASIC
    // register, which is only actually valid for n==0 - this silently
    // never worked for any GPU-routed interrupt, masked until now by
    // every other bug already fixed in this port's own bring-up (see
    // notes_arch_armv7_rpi.txt's own "Gap 2").
    if (n == 0) {
        vic_base[VIC_INTENABLE] = (1<<n);
    } else if (n < 32) {
        vic_base[VIC_INTENABLE1] = (1<<n);
    } else {
        vic_base[VIC_INTENABLE2] = (1<<(n-32));
    }
}

void pic_disable(int n) {
    if ((n<0) || (n>=NUM_INTSRC)) {
        panic("invalid interrupt source");
    }
    if (n == 0) {
        vic_base[VIC_INTCLEAR] = (1<<n);
    } else if (n < 32) {
        vic_base[VIC_INTDISABLE1] = (1<<n);
    } else {
        vic_base[VIC_INTDISABLE2] = (1<<(n-32));
    }
    isrs[n] = default_isr;
}

#define INT_REGS_BASE (0x3F00B200)

// dispatch the interrupt
void pic_dispatch (struct trapframe *tf) {
    //uint intstatus_arm;
    //uint intstatus_gpu0;
    //uint intstatus_gpu1;
    //int i;
    uint istimer;
    
    
    /*
#define IRQ_PENDING_BASIC (IO_BASE + 0xB200) // VIC_IRQPENDING_ARM
#define IRQ_PENDING1      (IO_BASE + 0xB204) // VIC_IRQPENDING_GPU0
#define IRQ_PENDING2      (IO_BASE + 0xB208) // VIC_IRQPENDING_GPU1
//
#define IRQ_ENABLE1       (IO_BASE + 0xB210)

#define VIC_BASE (0x3F00B200)
*/    
    
    // should print intstatus to debug
    //intstatus_arm = vic_base[VIC_IRQPENDING_ARM];
    //cprintf ("pic_dispatch ip->armpending: %d\n", intstatus_arm );
    
    //intstatus_gpu0 = vic_base[VIC_IRQPENDING_GPU0];
    //cprintf ("pic_dispatch ip->gpupending[0]: %d\n", intstatus_gpu0 );

    /*
    intstatus_gpu1 = vic_base[VIC_IRQPENDING_GPU1];
    cprintf ("pic_dispatch ip->gpupending[1]: %d\n", intstatus_gpu1 );
    */
    /*
    //while(intstatus_gpu0 || intstatus_gpu1 || intstatus_arm) {
      if((!intstatus_gpu0) & intstatus_arm) {
        cprintf ("timer %s", "\n" );  
        isrs[0](tf, 0);  
      }
    
      if(intstatus_gpu0 & (1 << 29)) {
        cprintf ("uart %s", "\n" );
        isrs[29](tf, 29);  
      }
    
    //}
    */
    
    /*
    intctrlregs *ip;
    
    ip = (intctrlregs *)INT_REGS_BASE;
    while(ip->gpupending[0] || ip->gpupending[1] || ip->armpending){
      if(ip->gpupending[0] & (1 << 3)) {
        cprintf ("timer %s", "\n" );  
        isrs[1](tf, 0);  
        //timer3intr();
      }
      if(ip->gpupending[0] & (1 << 29)) {
        cprintf ("uart %s", "\n" );
        isrs[29](tf, 29);  
        //miniuartintr();
      }
    }
    */
    
    
    //#define PBASE 0x3F000000
    //#define INT_REGS_BASE     (PBASE+0xB200)
    //#define IRQ_PENDING_BASIC (PBASE+0xB200)
    //#define IRQ_PENDING1      (PBASE+0xB204)
    
    //ARM_TIMER_CLI 0x3F00B40C
    istimer = 0;
    
    //while( vic_base[VIC_IRQPENDING_GPU0] || vic_base[VIC_IRQPENDING_GPU1] || vic_base[VIC_IRQPENDING_ARM]) {
      // claude: "& (1 << PIC_TIMER0)", not just "if (word != 0)".
      //
      // On BCM2835/36 the BASIC pending register is not only the eight
      // ARM-local sources: bit 8 means "one or more bits set in GPU
      // pending register 1" and bit 9 the same for register 2. So the
      // moment ANY GPU-routed interrupt is pending - the System Timer
      // below, or either UART - this word is non-zero, and the original
      // unmasked test read that as "the ARM timer fired" and dispatched
      // isrs[PIC_TIMER0]. That was harmless only while nothing else was
      // ever pending; enabling the System Timer surfaced it immediately
      // as a flood of "unhandled interrupt: 0" (default_isr, since
      // PIC_TIMER0 is no longer registered). Bit 0 is the real "ARM
      // Timer IRQ" bit.
      if(vic_base[VIC_IRQPENDING_ARM] & (1 << PIC_TIMER0)) {
        //cprintf ("timer %s", "\n" );
        istimer = 1;  
        isrs[PIC_TIMER0](tf, PIC_TIMER0);  
      }
    
      // claude: the System Timer's compare-3 match, GPU IRQ 3 - this is
      // the tick that actually drives preemption on this port (the ARM
      // timer checked just above is a QEMU unimplemented-device stub and
      // never fires; see device/timer.c's own comment on timer3_init()).
      // Setting istimer here is the whole point: it is what makes
      // pic_dispatch's tail call yield(), so a CPU-bound user process
      // gets taken off the core. Without it usertests' preempt() hangs.
      if(vic_base[VIC_IRQPENDING_GPU0] & (1 << PIC_TIMER3)) {
        istimer = 1;
        isrs[PIC_TIMER3](tf, PIC_TIMER3);
      }

      if(vic_base[VIC_IRQPENDING_GPU0] & (1 << PIC_UART0)) {
        //cprintf ("uart %s", "\n" );
        isrs[PIC_UART0](tf, PIC_UART0);
      }

      // claude: added - PIC_UART0_PL011 (57) is in the GPU1 bank
      // (IRQs 32-63), which nothing here checked at all before (see
      // notes_arch_armv7_rpi.txt's own "Gap 2" - VIC_IRQPENDING_GPU1
      // was defined but never actually read anywhere in this function).
      if(vic_base[VIC_IRQPENDING_GPU1] & (1 << (PIC_UART0_PL011 - 32))) {
        isrs[PIC_UART0_PL011](tf, PIC_UART0_PL011);
      }

    //}
    
    /*
    for(i=0;i<NUM_INTSRC;++i) {
        
        if (intstatus & (1<<i)) {
            cprintf (">: %x %x %x\n", intstatus, (1<<i), intstatus & (1<<i) );
            isrs[i](tf, i);
        }
    }
    */
    
    
  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  //
  // on xv6 x86
  //if(proc && proc->killed && (tf->cs&3) == DPL_USER)
  // 
  // on arm: (r14_svc == pc if SWI) 
  // - the proc in swi is in kernel space
  // - the proc not in swi is in user space
  // so: we need to compare tp->r14_svc with tp->pc
  // they need to be diferent to proc be in user space
  /*
  if(proc && proc->killed && (tf->r14_svc) != (tf->pc)) {
    exit();
  }
  */
  
  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(proc && proc->state == RUNNING && istimer) {
    yield();
  }
  
  // Check if the process has been killed since we yielded
  if(proc && proc->killed && (tf->r14_svc) != (tf->pc)) {
    exit();
  }
  
}

/*
 * dispatch the interrupt
 */
 /*
void pic_dispatch (struct trapframe *tp)
{
	int intid, intn;
	intid = gic_getack(); // iack 
	intn = intid - 32;
	
	isrs[intn](tp, intn);
	gic_eoi(intn);

  
}

*/