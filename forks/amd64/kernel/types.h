#include "core/types.h"   // claude: portable typedefs, shared by every port


typedef unsigned long uint64;

typedef uint64 pde_t;

// claude: pointer-sized integer, for tests/usertests-x86.c's own pointer
// <-> integer casts (shared with forks/i386, whose own uint64 is 8 bytes
// despite 4-byte pointers there - see that fork's own types.h). Same
// name and purpose as forks/amd64-jserv's own uintp.
typedef uint64 uintp;
