#ifndef ARCH_PROC_H
#define ARCH_PROC_H

// claude: struct cpu/struct context - real CPU register layout,
// hand-matched to swtch.S. Unlike the rest of this family, which
// indexes cpus[] via a curr_cpu/curr_proc macro, this fork accesses the
// current CPU/process through bare "cpu"/"proc" globals instead (same
// shape as mips's/amd64-jserv's own) - a real, independent design
// choice, not something struct proc's own shape predicts.

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

extern struct cpu* cpu;
extern struct proc* proc;

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
};

#endif /* ARCH_PROC_H */
