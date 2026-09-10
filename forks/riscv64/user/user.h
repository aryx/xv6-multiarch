// claude: this port's additions to the shared API - a two-argument sys_sbrk
// so memory can be mapped eagerly or lazily, the wrappers over it, and two
// syscalls no other port has. Everything else lives in include/user/user.h,
// which this pulls in by relative path; this file is found first because the
// fork root comes earlier on the include path.
//
// Note this port calls its sleep syscall pause(); the shared header still
// declares sleep(), which is harmless here - nothing in this port calls it.
#include "../../../include/user/user.h"

// claude: this port's own sentinel, returned by its sbrk wrappers on failure.
#define SBRK_ERROR ((char *)-1)

char *sys_sbrk(int, int);
char *sbrklazy(int);
int pause(int);
int sync(void);
