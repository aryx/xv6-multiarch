#include <core/types.h>   // claude: portable typedefs, shared by every port


typedef unsigned long uint64;

#if X64
typedef unsigned long uintp;
#else
typedef unsigned int  uintp;
#endif

typedef uintp pde_t;
