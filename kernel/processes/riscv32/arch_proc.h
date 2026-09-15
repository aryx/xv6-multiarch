#ifndef ARCH_PROC_H
#define ARCH_PROC_H

#include "processes/interface.h"

// claude: arch_sleep_release() below needs these forward-declared -
// can't rely on whichever .c file happens to include "defs.h" before
// "proc.h"/"proc-legacy.h" (which now pull this file in) - some don't.
struct spinlock;
void sleep(void*, struct spinlock*);

// claude: struct cpu/struct context - real CPU register layout,
// hand-matched to swtch.S.

// Saved registers for kernel context switches.
struct context {
  uint32 ra;
  uint32 sp;

  // callee-saved
  uint32 s0;
  uint32 s1;
  uint32 s2;
  uint32 s3;
  uint32 s4;
  uint32 s5;
  uint32 s6;
  uint32 s7;
  uint32 s8;
  uint32 s9;
  uint32 s10;
  uint32 s11;
};

// Per-CPU state
struct cpu {
  struct proc *proc;          // The process running on this cpu, or null.
  struct context context;     // swtch() here to enter scheduler().
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

// per-process trap-handling state (trampoline.S/uservec) - real
// per-ISA register save area.
struct trapframe {
  /*   0 */ uint32 kernel_satp;   // kernel page table
  /*   4 */ uint32 kernel_sp;     // top of process's kernel stack
  /*   8 */ uint32 kernel_trap;   // usertrap()
  /*  12 */ uint32 epc;           // saved user program counter
  /*  16 */ uint32 kernel_hartid; // saved kernel tp
  /*  20 */ uint32 ra;
  /*  24 */ uint32 sp;
  /*  28 */ uint32 gp;
  /*  32 */ uint32 tp;
  /*  36 */ uint32 t0;
  /*  40 */ uint32 t1;
  /*  44 */ uint32 t2;
  /*  48 */ uint32 s0;
  /*  52 */ uint32 s1;
  /*  56 */ uint32 a0;
  /*  60 */ uint32 a1;
  /*  64 */ uint32 a2;
  /*  68 */ uint32 a3;
  /*  72 */ uint32 a4;
  /*  76 */ uint32 a5;
  /*  80 */ uint32 a6;
  /*  84 */ uint32 a7;
  /*  88 */ uint32 s2;
  /*  92 */ uint32 s3;
  /*  96 */ uint32 s4;
  /* 100 */ uint32 s5;
  /* 104 */ uint32 s6;
  /* 108 */ uint32 s7;
  /* 112 */ uint32 s8;
  /* 116 */ uint32 s9;
  /* 120 */ uint32 s10;
  /* 124 */ uint32 s11;
  /* 128 */ uint32 t3;
  /* 132 */ uint32 t4;
  /* 136 */ uint32 t5;
  /* 140 */ uint32 t6;
};


// claude: trivial backend for the arch_sleep_release() interface - see
// kernel/arch/riscv64/arch_proc.h's own comment for why this exists at
// all (that port's own sleep() is split in two, to close a real
// lost-wakeup race this port doesn't fix). This port's own sleep(chan,
// lk) already does register+release+block+reacquire atomically, so
// arch_sleep_release() is a plain pass-through.
static inline void
arch_sleep_release(void *chan, struct spinlock *lk)
{
  sleep(chan, lk);
}

#endif /* ARCH_PROC_H */
