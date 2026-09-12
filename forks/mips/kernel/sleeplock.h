// claude: this port's own inode locking uses a plain I_BUSY flag, not
// a sleeplock - this struct is unused by anything in forks/mips/ - but
// the now-shared kernel/file.c/kernel/pipe.c #include "sleeplock.h"
// (needed by the 5 other forks sharing them, whose own struct inode
// really does have a sleeplock field), so the include line itself
// needs somewhere to resolve to. Matches the content amd64/i386/arm64/
// arm64-pi4/loongarch already carry, byte-for-byte.
// Long-term locks for processes
struct sleeplock {
  uint locked;       // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock

  // For debugging:
  char *name;        // Name of lock.
  int pid;           // Process holding lock
};
