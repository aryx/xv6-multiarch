#include "core/types.h"   // claude: portable typedefs, shared by every port
#ifndef TYPES_H
#define TYPES_H

#ifndef NULL
#define NULL ((void*)0)
#endif

/* ARM interrupt control registers */
/*
typedef struct intctrlregs {
        uint  armpending;
        uint  gpupending[2];
        uint  fiqctrl;
        uint  gpuenable[2];
        uint  armenable;
        uint  gpudisable[2];
        uint  armdisable;
} intctrlregs;
*/
#endif
typedef unsigned long long uint64;  // claude: needed by the shared
                                    // printf.c's printptr(); this port is
                                    // 32-bit, so it has no native 64-bit
                                    // word, but the C type is still fine.
