// claude: loongarch's own general, machine-word-dependent types - this
// port's equivalent of Plan9's u.h (see include/core/types.h's own
// header comment).

// this port's own "long" is already 8 bytes (real 64-bit word).
typedef unsigned long uint64;

// pointer-sized integer, for the shared kernel/sysproc.c's own
// sys_wait()/sys_exit()/etc. return type.
typedef uint64 uintp;
