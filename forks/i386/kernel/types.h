#include "core/types.h"   // claude: portable typedefs, shared by every port


typedef uint pde_t;
typedef unsigned long long uint64;  // claude: needed by the shared
                                    // printf.c's printptr(); this port is
                                    // 32-bit, so it has no native 64-bit
                                    // word, but the C type is still fine.
