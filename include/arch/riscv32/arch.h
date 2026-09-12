// claude: riscv32's own general, machine-word-dependent types - this
// port's equivalent of Plan9's u.h (see include/core/types.h's own
// header comment).

// this port's own "long" is 4 bytes; needed by the shared printf.c's
// printptr().
typedef unsigned long long uint64;

// pointer-sized integer (uint32, NOT uint64 above - this port's pointers
// are 4 bytes), for the shared kernel/kalloc.c's own PGROUNDUP((uintp)pa)
// casts.
typedef uint32 uintp;
