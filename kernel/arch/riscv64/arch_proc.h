#ifndef ARCH_PROC_H
#define ARCH_PROC_H

// claude: portable "sleep on chan, releasing lk across the sleep"
// interface, needed by the shared kernel/log.c. Every other fork in
// its own cluster (arm64/arm64-pi4/loongarch/riscv32) has a single
// sleep(chan, lk) that does register+release+block+reacquire
// atomically - this port's own sleep() is split into sleep_prepare()
// (register the channel) and a separate zero-arg sleep() (actually
// block, but only if the channel wasn't already woken up in the
// meantime) instead, a real fix for a narrow lost-wakeup race in the
// gap between releasing the caller's lock and the process actually
// going to sleep. This port's own proc.c/pipe.c/console.c/etc. all
// call the two-step form directly and are unaffected; this wrapper
// exists only so kernel/log.c can call the same sleep_release(chan, lk)
// either way.
static inline void
sleep_release(void *chan, struct spinlock *lk)
{
  sleep_prepare(chan);
  release(lk);
  sleep();
  acquire(lk);
}

#endif /* ARCH_PROC_H */
