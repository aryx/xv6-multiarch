// claude: arm64's own general, machine-word-dependent types - this
// port's equivalent of Plan9's u.h (see include/core/types.h's own
// header comment). Shared by arm64 and arm64-pi4 - same ISA, different
// boards.

// this port's own "long" is already 8 bytes (real 64-bit word).
typedef unsigned long uint64;

// pointer-sized integer, for the shared kernel/kalloc.c's own
// PGROUNDUP((uintp)pa) casts.
typedef uint64 uintp;
