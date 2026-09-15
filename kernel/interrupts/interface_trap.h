#ifndef INTERFACE_TRAP_H
#define INTERFACE_TRAP_H

// claude: trap.c has no arch_*.h dispatch family (it's core Tier-4
// material - the whole point of naming an arch_* interface is to let
// a *portable* file call it, and nothing portable calls into trap
// dispatch directly), but the per-fork implementations independently
// converged on the same function names for the same roles anyway,
// split along this tree's usual legacy/modern boundary. Each fork's
// own trap.c #includes this file right before defining these
// functions, so the compiler checks the real definition against the
// declaration here - see kernel/processes/interface_proc.h for the
// same technique applied to arch_proc.h.
//
// arm is excluded entirely: it dispatches through the ARM exception
// vector table directly (swi_handler/irq_handler/reset_handler/
// und_handler/dabort_handler/iabort_handler/fiq_handler, each handling
// one CPU exception type, no single trap-dispatch function at all) -
// a third design, not a variant of either family below.

// --- legacy family (amd64, amd64-jserv, arm-pi1, arm-pi1-bis,
//     arm-pi2, arm-pi3, i386, mips) ---
//
// void trap(struct trapframe *tf)
//   The single entry point for every trap/interrupt/exception,
//   user or kernel, dispatched by cause (syscall, page fault, IRQ
//   number, ...) all in one function and one switch/if-chain.
void trap(struct trapframe *tf);

// --- modern family (arm64, arm64-pi4, loongarch, riscv32, riscv64 -
//     MIT's 2019 redesign, same family as bio.c/buf.h/conf.h/
//     kalloc.c elsewhere in this tree) ---
//
// void trapinit(void)
//   One-time trap setup - installing the trap vector, mostly.
void trapinit(void);

// void kerneltrap(void)
//   Entry point for a trap taken while already running kernel code
//   (an interrupt during a syscall handler, say) - must not clobber
//   the interrupted context the way usertrap() can assume a fresh
//   trapframe.
void kerneltrap(void);

// void clockintr(void)
//   Handle a timer-tick interrupt: bump ticks, wake anyone sleeping on
//   &ticks, ack/rearm the timer. Called from both usertrap() and
//   kerneltrap() (via devintr() below) wherever the tick fires.
void clockintr(void);

// int devintr(void)
//   Recognize and handle one pending device interrupt (external IRQ
//   via the platform's interrupt controller, or the timer). Return
//   value is a real three-way status, consistent across this family:
//     0 - not a recognized device interrupt (caller should treat the
//         trap as something else, e.g. a fault)
//     1 - external device IRQ, handled
//     2 - timer interrupt, handled (clockintr() already called)
int devintr(void);

// The rest of this family has real per-fork exceptions and stays
// documentation-only (never #included), same spirit as
// kernel/memory/interface_vm.h's own legacy/modern split:
#if 0
// void trapinithart(void)
//   Per-hart/per-core trap setup. Missing on loongarch (folded into
//   trapinit() instead - checked: no separate function there).
void trapinithart(void);

// void usertrap(void)
//   Entry point for a trap taken while running user code. arm64-pi4's
//   own copy takes an explicit struct trapframe *tf argument instead
//   of reading it off the process; every other fork in this family
//   reads it via myproc()->trapframe internally.
void usertrap(void);

// void usertrapret(void)
//   Prepare to return to user space after usertrap() finishes. Present
//   under this exact name on loongarch and riscv32 only; arm64/
//   arm64-pi4 fold the equivalent work back into usertrap()'s own
//   tail, and riscv64 calls it prepare_return() instead.
void usertrapret(void);

// void userirq(void), void kernelirq(void)
//   arm64/arm64-pi4 only - a GICv3-specific split of devintr()'s own
//   job into the user-mode and kernel-mode IRQ paths. Not present on
//   loongarch/riscv32/riscv64, which handle both cases through the
//   one shared devintr() above instead.
void userirq(void);
void kernelirq(void);
#endif

#endif /* INTERFACE_TRAP_H */
