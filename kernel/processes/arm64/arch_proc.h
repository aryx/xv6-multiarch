#ifndef ARCH_PROC_H
#define ARCH_PROC_H

#include "processes/interface_proc.h"

// claude: arch_sleep_release() below needs these forward-declared -
// can't rely on whichever .c file happens to include "defs.h" before
// "proc.h"/"proc-legacy.h" (which now pull this file in) - some don't.
struct spinlock;
void sleep(void*, struct spinlock*);

// claude: struct cpu/struct context - real CPU register layout,
// hand-matched to swtch.S. Shared by arm64 and arm64-pi4 - same ISA,
// same register set.

// Saved registers for kernel context switches.
struct context {
  uint64 sp;

  /* callee register */
  uint64 x18;
  uint64 x19;
  uint64 x20;
  uint64 x21;
  uint64 x22;
  uint64 x23;
  uint64 x24;
  uint64 x25;
  uint64 x26;
  uint64 x27;
  uint64 x28;
  uint64 x29;
  uint64 x30;
};

// Per-CPU state
struct cpu {
  struct proc *proc;          // The process running on this cpu, or null.
  struct context context;     // swtch() here to enter scheduler().
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// the sscratch register points here.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.
struct trapframe {
  uint64 x0;
  uint64 x1;
  uint64 x2;
  uint64 x3;
  uint64 x4;
  uint64 x5;
  uint64 x6;
  uint64 x7;
  uint64 x8;
  uint64 x9;
  uint64 x10;
  uint64 x11;
  uint64 x12;
  uint64 x13;
  uint64 x14;
  uint64 x15;
  uint64 x16;
  uint64 x17;
  uint64 x18;
  uint64 x19;
  uint64 x20;
  uint64 x21;
  uint64 x22;
  uint64 x23;
  uint64 x24;
  uint64 x25;
  uint64 x26;
  uint64 x27;
  uint64 x28;
  uint64 x29;
  uint64 x30;
  uint64 elr;
  uint64 spsr;
  uint64 sp;     
};


// claude: trivial backend for the arch_sleep_release() interface - see
// kernel/arch/riscv64/arch_proc.h's own comment for why this exists at
// all (that port's own sleep() is split in two, to close a real
// lost-wakeup race this port doesn't fix). This port's own sleep(chan,
// lk) already does register+release+block+reacquire atomically, so
// arch_sleep_release() is a plain pass-through. Shared by arm64 and
// arm64-pi4 - same ISA, different boards, same include/arch/arm64
// pairing as this directory's own arch_vm.h.
static inline void
arch_sleep_release(void *chan, struct spinlock *lk)
{
  sleep(chan, lk);
}

#endif /* ARCH_PROC_H */
