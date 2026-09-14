#ifndef ARCH_PROC_H
#define ARCH_PROC_H

// claude: arch_sleep_release() below needs these forward-declared -
// can't rely on whichever .c file happens to include "defs.h" before
// "proc.h" (which now pulls this file in) - some don't.
struct spinlock;
void sleep_prepare(void*);
void sleep(void);
void acquire(struct spinlock*);
void release(struct spinlock*);

// claude: struct cpu/struct context - real CPU register layout,
// hand-matched to swtch.S.

// Saved registers for kernel context switches.
struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

// Per-CPU state
struct cpu {
  struct proc *proc;      // The process running on this cpu, or null.
  struct context context; // swtch() here to enter scheduler().
  int noff;               // Depth of push_off() nesting.
  int intena;              // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

// per-process trap-handling state (trampoline.S/uservec) - real
// per-ISA register save area.
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};


// claude: portable "sleep on chan, releasing lk across the sleep"
// interface, needed by the shared kernel/log.c. Every other fork in
// its own cluster (arm64/arm64-pi4/loongarch/riscv32) has a single
// sleep(chan, lk) that does register+release+block+reacquire
// atomically - this port's own sleep() is split into sleep_prepare()
// (register the channel) and a separate zero-arg sleep() (actually
// block, but only if the channel wasn't already woken up in the
// meantime) instead, a real fix for a narrow lost-wakeup race in the
// gap between releasing the caller's lock and the process actually
// going to sleep. This port's own proc.c/pipe.c/console.c/etc. all
// call the two-step form directly and are unaffected; this wrapper
// exists only so kernel/log.c can call the same arch_sleep_release(chan, lk)
// either way.
static inline void
arch_sleep_release(void *chan, struct spinlock *lk)
{
  sleep_prepare(chan);
  release(lk);
  sleep();
  acquire(lk);
}

#endif /* ARCH_PROC_H */
