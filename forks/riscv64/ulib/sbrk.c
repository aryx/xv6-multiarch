// claude: split out of this fork's ulib.c when the rest of that file moved to
// lib_core/libc/ulib/ulib.c. These two wrappers are specific to this port: its
// kernel takes a two-argument sys_sbrk(n, mode) so that memory can be mapped
// eagerly or lazily, which no other port here has. Everyone else gets sbrk()
// directly from usys.S as an ordinary one-argument syscall.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/vm.h"   // claude: SBRK_EAGER / SBRK_LAZY
#include "user/user.h"

char*
sbrk(int n)
{
  return sys_sbrk(n, SBRK_EAGER);
}

char*
sbrklazy(int n)
{
  return sys_sbrk(n, SBRK_LAZY);
}
