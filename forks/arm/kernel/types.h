#include <core/types.h>   // claude: portable typedefs, shared by every port
#include <arch.h>   // claude: this port's own general, arch-specific types (uint64, uintp)
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
