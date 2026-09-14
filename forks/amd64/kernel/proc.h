// claude: struct cpu/struct context (real CPU register layout,
// hand-matched to swtch.S) live in kernel/processes/amd64/arch_proc.h.
// struct proc itself stays standalone here rather than shared: its own
// struct sysframe *sf instead of trapframe, and a real "kstack must be
// first entry" layout requirement its own sysentry asm depends on.
#include "arch_proc.h"

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  char *kstack;                // Bottom of kernel stack for this process, must be first entry
  uint64 sz;                   // Size of process memory (bytes)
  pde_t* pagetable;             // Page table
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct sysframe *sf;         // Syscall frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
