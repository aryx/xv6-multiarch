#include <core/types.h>   // claude: portable typedefs, shared by every port
#include <arch.h>   // claude: this port's own general, arch-specific types (uint64, uintp)
typedef unsigned long long int   ulonglong;

typedef uint pde_t;
typedef ulonglong pte_t;
