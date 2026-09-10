// Runtime support the compiler expects but a freestanding xv6 userland does
// not otherwise provide.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// claude: hoisted from forks/arm's own ulib.c, which is where this fix was
// first written; it is now shared so that EVERY port can use C division
// normally in shared code.
//
// A target without a hardware divide instruction - ARMv6 and ARMv7 here -
// turns a C '/' or '%' into a call into libgcc (__aeabi_uidivmod and
// friends). Those live in libgcc, but libgcc's divide-by-zero path,
// __aeabi_idiv0, calls raise(), which is a hosted-C-library function xv6 has
// no notion of. Without this, linking any user program that divides fails
// with "undefined reference to `raise'" - and the tempting workaround, of
// rewriting the arithmetic to avoid dividing, is the wrong trade: division
// is far too useful an operation to give up in shared code.
//
// xv6 has no signals, so there is nothing to deliver. A real division by
// zero should never happen; if it somehow does, exiting is the reasonable
// fallback, matching what forks/arm already did.
int
raise(int sig)
{
  (void)sig;
  exit(0);
  return 0; // not reached
}
