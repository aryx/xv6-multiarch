#include "core/types.h"   // claude: portable typedefs, shared by every port


typedef uint pde_t;
typedef unsigned long long uint64;  // claude: needed by the shared
                                    // printf.c's printptr(); this port is
                                    // 32-bit, so it has no native 64-bit
                                    // word, but the C type is still fine.

// claude: pointer-sized integer (uint, NOT uint64 - this port's pointers
// are 4 bytes, but uint64 above is 8 regardless of the underlying word
// size) - for tests/usertests-x86.c's own pointer <-> integer casts,
// shared with forks/amd64. Same name and purpose as forks/amd64-jserv's
// own uintp.
typedef uint uintp;
