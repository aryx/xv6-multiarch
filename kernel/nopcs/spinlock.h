// claude: guard added - kernel/proc.h now self-includes this (see its
// own comment) in the forks that need it, which would otherwise double-
// define struct spinlock for any .c file that also includes it
// directly (as most already do).
#ifndef SPINLOCK_H
#define SPINLOCK_H

// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
};

#endif /* SPINLOCK_H */
