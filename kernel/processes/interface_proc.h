#ifndef INTERFACE_PROC_H
#define INTERFACE_PROC_H

// claude: documentation only - never #included by any build. C has no
// way to declare or enforce an interface, so this file exists purely
// so a reader can find, in one place, the contract every fork's own
// kernel/processes/<arch>/arch_proc.h is expected to satisfy. See
// docs/claude_notes/notes_new_kernel_organization.md for the
// "interface, not permanent fork" method this documents, and any one
// real arch_proc.h (e.g. kernel/processes/riscv64/arch_proc.h) for a
// worked example, including the one real case where two forks in the
// same family need genuinely different bodies.
//
// Adopted by 8 of 14 forks so far (amd64-jserv, amd64, arm64, i386,
// loongarch, mips, riscv32, riscv64); the six ARM ports don't have
// this interface yet.

// Atomically register the caller as waiting on `chan`, release `lk`,
// block until woken, then reacquire `lk` - the same operation every
// port's own sleep(chan, lk) performs internally. Exists as its own
// interface point (rather than every caller just calling sleep()
// directly) because riscv64's own sleep() is split into two steps -
// sleep_prepare(chan) then a zero-arg sleep() - to close a real
// lost-wakeup race between releasing the caller's lock and the
// process actually blocking, a fix its own proc.c/pipe.c/console.c
// call directly and unaffected code never needs to know about. Every
// other fork's own arch_proc.h is a one-line pass-through to its
// ordinary sleep(chan, lk). Called from: kernel/log.c.
void arch_sleep_release(void *chan, struct spinlock *lk);

#endif /* INTERFACE_PROC_H */
