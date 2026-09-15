#ifndef ARCH_PROC_H
#define ARCH_PROC_H

#include "processes/interface.h"

// claude: arch_sleep_release() below needs these forward-declared -
// can't rely on whichever .c file happens to include "defs.h" before
// "proc.h"/"proc-legacy.h" (which now pull this file in) - some don't.
struct spinlock;
void sleep(void*, struct spinlock*);

// claude: struct proc has one real mips-only field (asid, this port's
// software-managed-TLB address-space ID) on top of the shared
// kernel/processes/proc-legacy.h shape - this flag lets that file add
// it conditionally instead of duplicating the whole struct here.
#define HAS_ASID 1

// Per-CPU state
struct cpu {
  uchar id;                    // Local APIC ID; index into cpus[] below
  struct context *scheduler;   // swtch() here to enter scheduler
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?

  // Cpu-local storage variables; see below
  struct cpu *cpu;
  struct proc *proc;           // The currently-running process.
};

extern struct cpu cpus[NCPU];
extern int ncpu;

// Per-CPU variables, holding pointers to the
// current cpu and to the current process. Plain globals (no real
// per-CPU register trick, unlike amd64/i386's %gs-based version of the
// same idea) - safe since this port is single-core (NCPU 1).
extern struct cpu *cpu;       // &cpus[cpunum()]
extern struct proc *proc;     // cpus[cpunum()].proc

#define myproc() (proc)

struct context {
  uint s0;
  uint s1;
  uint s2;
  uint s3;
  uint s4;
  uint s5;
  uint s6;
  uint s7;
  uint s8;
  uint ra;
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
