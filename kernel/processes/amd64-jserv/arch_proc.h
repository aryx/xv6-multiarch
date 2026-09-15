#ifndef ARCH_PROC_H
#define ARCH_PROC_H

#include "processes/interface_proc.h"

// claude: arch_sleep_release() below needs these forward-declared -
// can't rely on whichever .c file happens to include "defs.h" before
// "proc.h"/"proc-legacy.h" (which now pull this file in) - some don't.
struct spinlock;
void sleep(void*, struct spinlock*);

#include "mmu.h"  // claude: struct taskstate, struct segdesc

// Segments in proc->gdt.
#define NSEGS     7

// Per-CPU state
struct cpu {
  uchar id;                    // index into cpus[] below
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?

  // Cpu-local storage variables; see below
#if X64
  void *local;
#else
  struct cpu *cpu;
  struct proc *proc;           // The currently-running process.
#endif
};

extern struct cpu cpus[NCPU];
extern int ncpu;

// Per-CPU variables, holding pointers to the
// current cpu and to the current process.
// The asm suffix tells gcc to use "%gs:0" to refer to cpu
// and "%gs:4" to refer to proc.  seginit sets up the
// %gs segment register so that %gs refers to the memory
// holding those two variables in the local cpu's struct cpu.
// This is similar to how thread-local variables are implemented
// in thread libraries such as Linux pthreads.
#if X64
extern __thread struct cpu *cpu;
extern __thread struct proc *proc;
#else
extern struct cpu *cpu asm("%gs:0");       // &cpus[cpunum()]
extern struct proc *proc asm("%gs:4");     // cpus[cpunum()].proc
#endif

// claude: this port has no real myproc() function, just this raw
// global - same shape as mips's own (see that fork's proc.h).
#define myproc() (proc)

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
#if X64
struct context {
  uintp r15;
  uintp r14;
  uintp r13;
  uintp r12;
  uintp r11;
  uintp rbx;
  uintp ebp; //rbp
  uintp eip; //rip;
};
#else
struct context {
  uintp edi;
  uintp esi;
  uintp ebx;
  uintp ebp;
  uintp eip;
};
#endif

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
