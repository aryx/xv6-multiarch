// claude: guard added proactively, matching kernel/nopcs/spinlock.h's
// own fix - a fork whose proc.h self-includes this (not needed by any
// fork yet, but the nopcs sibling already hit this exact double-
// definition bug) would otherwise double-define struct spinlock for
// any .c file that also includes it directly.
#ifndef SPINLOCK_H
#define SPINLOCK_H

// Mutual exclusion lock.
struct spinlock {
  uint locked __attribute((aligned (4)));  // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
  uintp pcs[10];     // The call stack (an array of program counters)
                     // that locked the lock.
};

#endif /* SPINLOCK_H */
