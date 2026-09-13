#ifndef INTERFACE_TRAP_H
#define INTERFACE_TRAP_H

// claude: documentation only - never #included by any build. Same
// kind as kernel/console/interface_console.h: trap.c has no
// arch_*.h dispatch family (it's core Tier-4 material - the whole
// point of naming an arch_* interface is to let a *portable* file
// call it, and nothing portable calls into trap dispatch directly),
// but the per-fork implementations independently converged on the
// same function names for the same roles anyway, split cleanly along
// this tree's usual legacy/modern boundary. Checked actual
// definitions (not just names/counts) before writing each prototype
// below.
//
// arm is excluded entirely: it dispatches through the ARM exception
// vector table directly (swi_handler/irq_handler/reset_handler/
// und_handler/dabort_handler/iabort_handler/fiq_handler, each handling
// one CPU exception type, no single trap-dispatch function at all) -
// a third design, not a variant of either family below.

// --- legacy family (8 forks: amd64, amd64-jserv, arm-pi1, arm-pi1-bis,
//     arm-pi2, arm-pi3, i386, mips) ---
//
// void trap(struct trapframe *tf)
//   The single entry point for every trap/interrupt/exception,
//   user or kernel, dispatched by cause (syscall, page fault, IRQ
//   number, ...) all in one function and one switch/if-chain.
void trap(struct trapframe *tf);

// --- modern family (5 forks: arm64, arm64-pi4, loongarch, riscv32,
//     riscv64 - MIT's 2019 redesign, same family as bio.c/buf.h/
//     conf.h/kalloc.c elsewhere in this tree) ---
//
// void trapinit(void), void trapinithart(void)
//   One-time (trapinit) and per-hart/per-core (trapinithart) trap
//   setup - installing the trap vector, mostly. loongarch's own
//   trapinithart is folded into trapinit instead (checked: no
//   separate function there) - noted, not treated as missing.
void trapinit(void);
void trapinithart(void);

// void usertrap(void)
//   Entry point for a trap taken while running user code - the
//   common case (syscalls, user page faults). arm64-pi4's own copy
//   takes an explicit struct trapframe *tf argument instead of
//   reading it off the process; every other fork in this family reads
//   it via myproc()->trapframe internally.
void usertrap(void);

// void usertrapret(void)
//   Prepare to return to user space after usertrap() finishes (restore
//   the trapframe, switch page tables, set up the trampoline). Present
//   under this exact name on loongarch and riscv32 only; arm64/
//   arm64-pi4 fold the equivalent work back into usertrap()'s own tail,
//   and riscv64 calls it prepare_return() instead - a real naming
//   exception, not just an omission.
void usertrapret(void);

// void kerneltrap(void)
//   Entry point for a trap taken while already running kernel code
//   (an interrupt during a syscall handler, say) - must not clobber
//   the interrupted context the way usertrap() can assume a fresh
//   trapframe. Consistent name and zero-arg signature on all 5 forks
//   in this family, including riscv64 (spelled kerneltrap() with an
//   explicit empty arg list rather than kerneltrap(void), same thing).
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

// void userirq(void), void kernelirq(void)
//   arm64/arm64-pi4 only (2 of the 5-fork family) - a GICv3-specific
//   split of devintr()'s own job into the user-mode and kernel-mode
//   IRQ paths. Not present on loongarch/riscv32/riscv64, which handle
//   both cases through the one shared devintr() above instead - a
//   real narrower interface, not a 5-fork one.
void userirq(void);
void kernelirq(void);

#endif /* INTERFACE_TRAP_H */
