// claude: amd64's own general, machine-word-dependent types - this
// port's equivalent of Plan9's u.h (see include/core/types.h's own
// header comment).

// this port's own "long" is already 8 bytes (real 64-bit word).
typedef unsigned long uint64;

// pointer-sized integer, for kernel/spinlock.h's pcs[10] and
// tests/usertests-x86.c's own pointer<->integer casts.
typedef uint64 uintp;
