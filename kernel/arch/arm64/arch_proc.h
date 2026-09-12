#ifndef ARCH_PROC_H
#define ARCH_PROC_H

// claude: trivial backend for the sleep_release() interface - see
// kernel/arch/riscv64/arch_proc.h's own comment for why this exists at
// all (that port's own sleep() is split in two, to close a real
// lost-wakeup race this port doesn't fix). This port's own sleep(chan,
// lk) already does register+release+block+reacquire atomically, so
// sleep_release() is a plain pass-through. Shared by arm64 and
// arm64-pi4 - same ISA, different boards, same include/arch/arm64
// pairing as this directory's own arch_vm.h.
static inline void
sleep_release(void *chan, struct spinlock *lk)
{
  sleep(chan, lk);
}

#endif /* ARCH_PROC_H */
