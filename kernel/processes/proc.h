// claude: was relying on whichever caller happened to already include
// spinlock.h/arch_vm.h before this file, for struct spinlock/
// pagetable_t just below (both embedded/used by value, needing full
// definitions, not just forward declarations) - kept self-contained
// instead, same reasoning kernel/sleeplock.c's own merge already
// established for this file.
#include "spinlock.h"
#include "arch_vm.h"

// claude: struct cpu and struct context are real CPU register layout
// (hand-matched to each fork's own swtch.S) - genuinely per-arch, not
// shared even within this family. Moved to each fork's own
// kernel/processes/<arch>/arch_proc.h; #included here since struct
// proc below embeds "struct context context" BY VALUE (unlike the
// legacy family's kernel/processes/proc-legacy.h, where it's a bare
// pointer) - the full struct context definition must already be
// visible at this point, not just forward-declared.
#include "arch_proc.h"

// claude: riscv32 alone lacks a separate USED state in its own
// original enum (UNUSED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE) - safe
// to use the 4-fork shape here anyway, since every comparison goes
// through the enum's names, never a raw integer value, so the shifted
// numeric values this gives riscv32 for SLEEPING/RUNNABLE/RUNNING/
// ZOMBIE don't matter.
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state. Covers the 5-fork modern family: arm64,
// arm64-pi4, loongarch, riscv32, riscv64.
struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state;        // Process state
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status to be returned to parent's wait
  int pid;                     // Process ID
#ifdef ARM64_PI4_EXTRA
  int ctxid;
  // claude: set when the kernel has written this process's user image
  // (fork's uvmcopy, userinit's initcode) and it has not yet been
  // I-cache-synced; cleared by scheduler() once it has. See scheduler().
  int cachesync;
#endif

  // wait_lock must be held when using this:
  struct proc *parent;         // Parent process

  // these are private to the process, so p->lock need not be held.
  uintp kstack;                // Virtual address of kernel stack
  uintp sz;                    // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
  struct trapframe *trapframe; // data page for trampoline/uservec
  struct context context;      // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
};
