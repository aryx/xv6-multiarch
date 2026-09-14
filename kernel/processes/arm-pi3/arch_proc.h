#ifndef ARCH_PROC_H
#define ARCH_PROC_H

// claude: struct cpu/struct context - real CPU register layout,
// hand-matched to swtch.S. This is the one multi-core board in its own
// family: curr_cpu/curr_proc do a real cpu_id() lookup instead of
// hardcoding cpus[0], and struct cpu carries first_sched/kpgdir, which
// a single-core board has no use for (see notes_arch_arm_pi3.txt).

// Per-CPU state
struct cpu {
  uchar id;                    // Local APIC ID; index into cpus[] below
  struct context *scheduler;   // swtch() here to enter scheduler
  int first_sched;             // Has the CPU entered the scheduler before.
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  pde_t *kpgdir;                // The page table for the CPU.

  // Cpu-local storage variables; see below
  struct cpu *cpu;
  struct proc *proc;           // The currently-running process.
};

extern struct cpu cpus[NCPU];

#define curr_cpu    (&cpus[cpu_id()])
#define curr_proc   (cpus[cpu_id()].proc)

#define myproc() (curr_proc)

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
struct context {
  uint r4;
  uint r5;
  uint r6;
  uint r7;
  uint r8;
  uint r9;
  uint r10;
  uint r11;
  uint r12;
  uint lr;
  uint pc;
};

enum cpustate { UNSTARTED=1, STARTED=1,CENTRY=2, SCHED=3, ERROR=0xff };

#endif /* ARCH_PROC_H */
