// claude: struct cpu and struct context used to live here too, but
// they're real CPU register layout (hand-matched to each fork's own
// swtch.S) - genuinely per-arch, not shared, even within this family.
// Moved to each fork's own kernel/processes/<arch>/arch_proc.h
// (matching the split kernel/processes/proc.h's own arch_proc.h use
// for the modern family), #included below since some of them (mips's
// HAS_ASID) parameterize struct proc itself.
#include "arch_proc.h"

enum procstate { UNUSED=0, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state. amd64 is the one legacy-family fork that stays
// standalone rather than sharing this: its own struct sysframe *sf
// instead of trapframe, and a real "kstack must be first entry" layout
// requirement its own sysentry asm depends on.
struct proc {
  uintp sz;                    // Size of process memory (bytes)
  pde_t* pagetable;             // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  volatile int pid;            // Process ID
#ifdef HAS_ASID
  int asid;                    // ASID (mips only - software-managed TLB)
#endif
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
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
