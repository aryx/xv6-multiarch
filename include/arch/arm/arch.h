// claude: arm's own general, machine-word-dependent types - this port's
// equivalent of Plan9's u.h (see include/core/types.h's own header
// comment). Shared by all five ARM32 ports (arm, arm-pi1, arm-pi1-bis,
// arm-pi2, arm-pi3) - same ISA, different boards.

// this port's own "long" is 4 bytes; needed by the shared printf.c's
// printptr().
typedef unsigned long long uint64;

// pointer-sized integer, for kernel/spinlock.h's pcs[10].
typedef uint uintp;
