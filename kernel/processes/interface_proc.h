#ifndef INTERFACE_PROC_H
#define INTERFACE_PROC_H

// claude: unlike the rest of this file (and every other interface_*.h
// in this tree), the declaration right below is genuinely live -
// #included by every fork's own arch_proc.h that implements
// arch_sleep_release(), so a signature typo in a newly-copied
// arch_proc.h fails to compile instead of silently mismatching. Safe
// to do here (not for most of this file's own content) because the
// signature only uses void* and a forward-declared struct pointer -
// no per-fork type width or layout to get wrong.
struct spinlock;
static inline void arch_sleep_release(void *chan, struct spinlock *lk);

// claude: everything below IS documentation only - never #included by
// any build. C has no way to declare or enforce an interface, so this
// file exists purely so a reader can find, in one place, the contract
// every fork's own kernel/processes/<arch>/arch_proc.h is expected to
// satisfy. See docs/claude_notes/notes_new_kernel_organization.md for
// the "interface, not permanent fork" method this documents, and any
// one real arch_proc.h (e.g. kernel/processes/riscv64/arch_proc.h) for
// a worked example, including the one real case where two forks in the
// same family need genuinely different bodies.
//
// arch_sleep_release() above was arch_proc.h's original, narrow job.
// Its real scope is broader: it's also where struct cpu, struct
// context, and (where struct proc embeds it by value rather than via
// a pointer) struct trapframe live - genuine per-ISA register layout,
// hand-matched to that fork's own swtch.S, that a proc.h shared across
// forks can no longer hold. Two lessons worth keeping in mind before
// ever using one fork's arch_proc.h as a template for another:
//   - struct proc being identical between two forks does not mean
//     struct cpu/context are too - the mechanism for reaching "the
//     current cpu" and "the current process" is a separate design
//     choice (an array-indexed cpus[] macro vs. plain module-level
//     "cpu"/"proc" globals vs. something else) that can differ even
//     between forks sharing everything else.
//   - a struct context copied from a sibling fork can silently gain or
//     lose a field its own swtch.S was never written to save/restore.
//     The struct "looking right" proves nothing; only the actual
//     register list swtch.S (or its equivalent) pushes and pops does.
//     A context struct wider than what the assembly actually saves
//     corrupts the kernel stack silently - it just shifts everything
//     built on top of it - and only shows up as a crash on first use.
// What belongs here, per fork: struct cpu, struct context, struct
// trapframe (where applicable), extern struct cpu cpus[NCPU], and
// whatever mechanism this fork uses to reach the current cpu/proc.
// There's no single shape to document as "the" contract for that last
// part - see each fork's own arch_proc.h.

// arch_sleep_release(void *chan, struct spinlock *lk) (declared live,
// above): atomically register the caller as waiting on `chan`, release
// `lk`, block until woken, then reacquire `lk` - the same operation
// every port's own sleep(chan, lk) performs internally. Exists as its
// own interface point (rather than every caller just calling sleep()
// directly) because riscv64's own sleep() is split into two steps -
// sleep_prepare(chan) then a zero-arg sleep() - to close a real
// lost-wakeup race between releasing the caller's lock and the
// process actually blocking, a fix its own proc.c/pipe.c/console.c
// call directly and unaffected code never needs to know about. Every
// other fork's own arch_proc.h is a one-line pass-through to its
// ordinary sleep(chan, lk). Called from: kernel/filesystems/log.c.

// claude: wrapped in #if 0 - unlike arch_sleep_release() above, these
// declarations genuinely conflict with each other (the legacy and
// modern families below use the same names with different signatures)
// and would break the build if ever actually compiled. Kept as text a
// reader can find, not as live code - see interface_console.h/
// interface_vm.h/interface_syscall.h for the same pattern.
#if 0

// claude: everything below is a different kind of documentation than
// the arch_proc.h contract above - proc.c has no arch_*.h dispatch
// family of its own (it's Tier-4 material), but the per-fork
// implementations converged on the same function names for the same
// roles anyway, same "emergent, not deliberate" story as
// interface_console.h/interface_trap.h/interface_vm.h's own added
// section. Completes the set: proc.c, vm.c, trap.c and syscall.c are
// this tree's classic Tier-4 four, all documented this way now.
//
// arm is excluded (no proc.c the same shape as the other 13 - its own
// process/scheduling code is organized differently throughout).

// void scheduler(void)
//   Each CPU's own idle loop: find a RUNNABLE proc, switch to it via
//   swtch(), and come back here when it yields or blocks. Never
//   returns. Identical name and signature on all 13 forks that have
//   one - the single most universal name in this file, alongside
//   yield/sleep/wakeup/allocproc/userinit/growproc/procdump below.
void scheduler(void);

// void yield(void)
//   Give up the CPU for one scheduling round - mark the caller
//   RUNNABLE and call into the scheduler. Universal, all 13 forks.
void yield(void);

// void sleep(void *chan, struct spinlock *lk)
//   Atomically register on `chan`, release `lk`, and block - see
//   arch_sleep_release() above for the one fork (riscv64) that splits
//   this into sleep_prepare(chan) + a separate zero-arg sleep(void)
//   instead, to close a real lost-wakeup race. Both shapes documented
//   here since sleep() itself, unlike arch_sleep_release(), is called
//   directly all over each fork's own code, not just from one shared
//   caller.
void sleep(void *chan, struct spinlock *lk);

// void wakeup(void *chan)
//   Wake every process sleeping on `chan`. Universal, all 13 forks -
//   the "outer" function most code calls.
void wakeup(void *chan);

// static void wakeup1(void *chan)
//   The actual "scan every proc, wake the ones sleeping on chan"
//   loop, called (with the process-table lock already held) both by
//   wakeup() and, on the legacy family, by exit()/kill() directly.
//   Present on 9 forks - the 8-fork legacy family (see the other
//   interface_*.h files' own split) plus one more; the remaining
//   forks fold the same loop into wakeup() itself instead, with no
//   separately-named inner function - not a missing feature, just no
//   name to document.
static void wakeup1(void *chan);

// struct proc *allocproc(void)
//   Find an UNUSED slot in the process table, initialize it (PID,
//   kernel stack, trapframe/context), and return it still marked
//   ALLOCATING/EMBRYO - the shared first half of both fork() and
//   userinit(). Universal, all 13 forks.
void *allocproc(void);

// void userinit(void)
//   Create the very first process (PID 1, running initcode) - called
//   once from main(). Universal, all 13 forks.
void userinit(void);

// int growproc(int n)
//   Grow or shrink the calling process's own memory by n bytes (n may
//   be negative) - sbrk()'s own implementation, built on
//   uvmalloc()/uvmdealloc() or allocuvm()/deallocuvm()
//   (kernel/memory/interface_vm.h). Universal, all 13 forks, same
//   signature throughout.
int growproc(int n);

// void procdump(void)
//   Print every process's PID, state and name to the console - the
//   Ctrl-P debug dump. Universal, all 13 forks.
void procdump(void);

// --- legacy family (8 of the 13, matching every other split in this
//     tree) ---
//
// int fork(void)
// void exit(void)
// int wait(void)
// int kill(int pid)
//   The classic MIT xv6 process syscalls' own kernel-side
//   implementations - no exit status, no wait4-style output pointer,
//   just PID and RUNNABLE-vs-ZOMBIE bookkeeping.
int fork(void);
void exit(void);
int wait(void);
int kill(int pid);

// --- modern family (5: arm64, arm64-pi4, loongarch, riscv32, plus
//     riscv64 under its own name below) ---
//
// int fork(void)
// void exit(int status)
// int wait(uint64 addr)
// int kill(int pid)
//   Same fork()/kill() as the legacy family, but a real exit(status)/
//   wait(*status) pair - a process now has a real exit code, and a
//   parent can retrieve it (`addr` is a user-space pointer arch_
//   copyout() writes the status through, or 0 to not care). riscv64
//   itself spells exit() as kexit(int status) instead - a real
//   1-fork naming exception on top of the signature split, found by
//   checking rather than assumed present under the common name.
int fork(void);
void exit(int status);
int wait(uintp addr);
int kill(int pid);
void kexit(int status); /* riscv64 only, in place of exit() */

// --- modern-family-only additions, no legacy-family equivalent name
//     (the same job is done inline, unnamed, in exit()/wait() there)
//     ---
//
// void reparent(struct proc *p)
//   Give every child of `p` to the first still-running process
//   (usually init) - called from exit(), before the exiting process
//   itself becomes a zombie. 5 of 5 modern forks.
void reparent(void *p);

// void freeproc(struct proc *p)
//   Release a ZOMBIE process's own resources (kernel stack, page
//   table, trapframe) and mark its slot UNUSED again - called from
//   wait() once a zombie child has been reaped. 5 of 5 modern forks.
void freeproc(void *p);

#endif /* #if 0 */

#endif /* INTERFACE_PROC_H */
