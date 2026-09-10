// The ARM UART, a memory mapped device
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"

#include "memlayout.h" // raspi

// claude: the check xv6 x86 makes at the bottom of its own trap() and this
// port never made anywhere:
//
//     if(proc && proc->killed && (tf->cs&3) == DPL_USER)
//       exit();
//
// kill() sets p->killed and wakes the target if it is SLEEPING (see
// proc.c), but SETTING the flag is only half of it - something has to act
// on it when the process is about to go back to user space, or a killed
// process simply keeps running. Nothing here did: "killed" appeared in
// this file only as an assignment, never as a test.
//
// The visible consequence was that kill() did not kill. usertests'
// sbrktest() ends by killing ten children that sit in "for(;;)
// sleep(1000);" and wait()ing for each. sys_sleep() does check
// proc->killed and returns -1 - so the child woke, returned -1, and the
// for(;;) called sleep() again, spinning forever instead of exiting.
// wait() therefore blocked forever, usertests never finished, and the
// harness sat until its 600s timeout - which is what turned CI's
// "build (arm)" job from 77s into a ten-minute failure once sbrktest()
// stopped being skipped. The children also never released their memory,
// so the following exec test ran into "allocuvm out of memory" and died,
// and init printed the "zombie!" lines as it reaped the orphans.
//
// "(r->spsr & MODE_MASK) == USR_MODE" is this architecture's spelling of
// x86's "(tf->cs & 3) == DPL_USER": the SPSR saved on exception entry
// holds the pre-exception processor mode. It matters that we exit ONLY
// when heading back to user space - calling exit() while the trap
// interrupted kernel code would tear down a process midway through a
// kernel path, holding whatever locks it held. Same test dabort_handler
// below already uses, for the same reason.
static void exit_if_killed (struct trapframe *r)
{
    if (proc && proc->killed && (r->spsr & MODE_MASK) == USR_MODE) {
        exit();
    }
}

// trap routine
void swi_handler (struct trapframe *r)
{
    proc->tf = r;
    syscall ();

    // claude: a process killed while blocked in a syscall (sleep(), read(),
    // wait()) resumes here, with the syscall having returned an error.
    exit_if_killed (r);
}

// trap routine
void irq_handler (struct trapframe *r)
{
    
    // proc points to the current process. If the kernel is
    // running scheduler, proc is NULL.
    if (proc != NULL) {
        proc->tf = r;
    }

    pic_dispatch (r);

    // claude: and a process killed while merely running in user space is
    // noticed on the next interrupt - the timer tick guarantees one
    // arrives. pic_dispatch() may yield() on that tick, so this also
    // covers being killed while descheduled.
    exit_if_killed (r);
}

// trap routine
void reset_handler (struct trapframe *r)
{
    cli();
    cprintf ("reset at: 0x%x \n", r->pc);
}

// trap routine
void und_handler (struct trapframe *r)
{
    cli();
    cprintf ("und at: 0x%x \n", r->pc);
}

void sleepwrap(void* wproc);

// trap routine
void dabort_handler (struct trapframe *r)
{
    /*
    // xv6 x86 code
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      panic("trap");
    }
    // In user space, assume process misbehaved.
    myproc()->killed = 1;
    */
    
    // claude: was "proc == 0 && r->r14_svc == r->pc". That second test was
    // vacuous: the old trap_dabort pushed r14 TWICE, once as pc and once
    // as r14_svc, so the two were always equal by construction and the
    // condition reduced to "proc == 0". It cannot be used at all now -
    // trap_asm.S's trap_dabort switches to SVC mode and stores the real
    // r14_svc there, exactly as trap_irq already did.
    //
    // The correct test is the one the x86 original was making with
    // "(tf->cs & 3) == 0": did this fault come from user mode? The SPSR
    // saved on entry holds the pre-exception processor mode, so ask it
    // directly. Keeping "proc == 0" as well, since a fault with no
    // current process is a kernel bug however it is reached.
    
    uint dfs, fa;
    extern void show_callstk (char *s);

    if(proc == 0 || (r->spsr & MODE_MASK) != USR_MODE) {
      // In kernel, it must be our mistake.
      // put second part of original code
      cli();

      // read data fault status register
      asm("MRC p15, 0, %[r], c5, c0, 0": [r]"=r" (dfs)::);

      // read the fault address register
      asm("MRC p15, 0, %[r], c6, c0, 0": [r]"=r" (fa)::);
      
      cprintf ("data abort: instruction 0x%x, fault addr 0x%x, reason 0x%x \n",
               r->pc, fa, dfs);
      
      dump_trapframe (r);
      show_callstk("Stack dump for data exception.");
    }
    
    // In user space, assume process misbehaved.
    
    // get data to show on msg
    // read data fault status register 
    asm("MRC p15, 0, %[r], c5, c0, 0": [r]"=r" (dfs)::);
    // read the fault address register
    asm("MRC p15, 0, %[r], c6, c0, 0": [r]"=r" (fa)::);

    cprintf ("data abort: pid %d %s instruction 0x%x, fault addr 0x%x, reason 0x%x \n",
             proc->pid, proc->name, r->pc, fa, dfs);
    
    // kill the misbeaved process
    proc->killed = 1;
    
    // workaround to avoid crash (this kill the process and its parent?)
    // it seems to raise the 'zombie!' message on userinit.
    exit();
}

// trap routine
void iabort_handler (struct trapframe *r)
{
    uint ifs;
    
    // read fault status register
    asm("MRC p15, 0, %[r], c5, c0, 0": [r]"=r" (ifs)::);

    cli();
    cprintf ("prefetch abort at: 0x%x (reason: 0x%x)\n", r->pc, ifs);
    dump_trapframe (r);
}

// trap routine
void na_handler (struct trapframe *r)
{
    cli();
    cprintf ("n/a at: 0x%x \n", r->pc);
}

// trap routine
void fiq_handler (struct trapframe *r)
{
    cli();
    cprintf ("fiq at: 0x%x \n", r->pc);
}

// low-level init code: in real hardware, lower memory is usually mapped
// to flash during startup, we need to remap it to SDRAM
void trap_init ( )
{
    volatile uint32 *ram_start;
    char *stk;
    int i;
    uint modes[] = {FIQ_MODE, IRQ_MODE, ABT_MODE, UND_MODE};

    // the opcode of PC relative load (to PC) instruction LDR pc, [pc,...]
    static uint32 const LDR_PCPC = 0xE59FF000U;

    // create the excpetion vectors
    ram_start = (uint32*)VEC_TBL;

    ram_start[0] = LDR_PCPC | 0x18; // Reset (SVC)
    ram_start[1] = LDR_PCPC | 0x18; // Undefine Instruction (UND)
    ram_start[2] = LDR_PCPC | 0x18; // Software interrupt (SVC)
    ram_start[3] = LDR_PCPC | 0x18; // Prefetch abort (ABT)
    ram_start[4] = LDR_PCPC | 0x18; // Data abort (ABT)
    ram_start[5] = LDR_PCPC | 0x18; // Not assigned (-)
    ram_start[6] = LDR_PCPC | 0x18; // IRQ (IRQ)
    ram_start[7] = LDR_PCPC | 0x18; // FIQ (FIQ)

    ram_start[8]  = (uint32)trap_reset;
    ram_start[9]  = (uint32)trap_und;
    ram_start[10] = (uint32)trap_swi;
    ram_start[11] = (uint32)trap_iabort;
    ram_start[12] = (uint32)trap_dabort;
    ram_start[13] = (uint32)trap_na;
    ram_start[14] = (uint32)trap_irq;
    ram_start[15] = (uint32)trap_fiq;

    // initialize the stacks for different mode
    for (i = 0; i < sizeof(modes)/sizeof(uint); i++) {
        stk = alloc_page ();

        if (stk == NULL) {
            panic("failed to alloc memory for irq stack");
        }

        set_stk (modes[i], (uint)stk);
    }
}

void dump_trapframe (struct trapframe *tf)
{
    cprintf ("r14_svc: 0x%x\n", tf->r14_svc);
    cprintf ("   spsr: 0x%x\n", tf->spsr);
    cprintf ("     r0: 0x%x\n", tf->r0);
    cprintf ("     r1: 0x%x\n", tf->r1);
    cprintf ("     r2: 0x%x\n", tf->r2);
    cprintf ("     r3: 0x%x\n", tf->r3);
    cprintf ("     r4: 0x%x\n", tf->r4);
    cprintf ("     r5: 0x%x\n", tf->r5);
    cprintf ("     r6: 0x%x\n", tf->r6);
    cprintf ("     r7: 0x%x\n", tf->r7);
    cprintf ("     r8: 0x%x\n", tf->r8);
    cprintf ("     r9: 0x%x\n", tf->r9);
    cprintf ("    r10: 0x%x\n", tf->r10);
    cprintf ("    r11: 0x%x\n", tf->r11);
    cprintf ("    r12: 0x%x\n", tf->r12);
    cprintf ("     pc: 0x%x\n", tf->pc);
}
