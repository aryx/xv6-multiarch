// claude: LOCAL OVERRIDE, identical to forks/arm-pi1's own copy (same
// CPU/float-ABI pair: arm-linux-gnueabihf-gcc 13,
// "-mcpu=arm1176jzf-s -marm -mfloat-abi=soft") - see that file's own
// header comment for the full story of the bug this works around: the
// shared lib_core/libc/ulib/printf.c's <stdarg.h>-based va_start/va_arg
// is miscompiled for a variadic function whose only named parameter is
// a pointer, corrupting every "%d" while "%s" still prints fine.
//
// On THIS fork specifically, this override is confirmed to fix that
// corruption (verified: a debug "printf(...%d...)" trace that used to
// come out as garbage prints correctly with this file in place, same
// as arm-pi1's own ls/wc), but it does NOT fix arm-pi1-bis's separate,
// still-open "ls"/"usertests" crash ("unknown sys call 4096", trap 2) -
// that one happens inside the child's very first malloc() (a fresh
// sbrk() call from shells/sh.c's parsecmd()/execcmd(), before growproc()
// is ever reached), unrelated to printf. Needs its own session - see
// notes_arch_arm_pi1_bis.txt and notes_arch_arm_pi1.txt.
//
// This technique is exactly correct here (see the shared printf.c's own
// header comment for why): ARM32 EABI passes every argument in
// registers/on the stack in 4-byte slots, in order, and GCC's variadic
// prologue for "printf(const char *fmt, ...)" spills r0-r3 to the stack
// contiguously right after the incoming stack-passed arguments, so
// "&fmt + 1" reliably lands on the first vararg's slot - it is simply
// the wrong choice on x86-64/AArch64/RISC-V, which is why those need
// <stdarg.h> and this arm1176jzf-s pair does not tolerate it.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
putc(int fd, char c)
{
  write(fd, &c, 1);
}

static uint
udiv(uint n, uint d)
{
  uint q = 0, r = 0;
  int i;
  for (i = 31; i >= 0; i--) {
    r = r << 1;
    r = r | ((n >> i) & 1);
    if (r >= d) {
      r = r - d;
      q = q | (1 << i);
    }
  }
  return q;
}

static void
printint(int fd, int xx, int base, int sgn)
{
  static char digits[] = "0123456789ABCDEF";
  char buf[16];
  int i, neg;
  uint x, y, b;

  neg = 0;
  if (sgn && xx < 0) {
    neg = 1;
    x = -xx;
  } else {
    x = xx;
  }

  b = base;
  i = 0;
  do {
    y = udiv(x, b);
    buf[i++] = digits[x - y * b];
  } while ((x = y) != 0);
  if (neg)
    buf[i++] = '-';

  while (--i >= 0)
    putc(fd, buf[i]);
}

static void
vprintf_old(int fd, const char *fmt, uint *ap)
{
  char *s;
  int c, i, state;

  state = 0;
  for (i = 0; fmt[i]; i++) {
    c = fmt[i] & 0xff;
    if (state == 0) {
      if (c == '%') {
        state = '%';
      } else {
        putc(fd, c);
      }
    } else if (state == '%') {
      if (c == 'd') {
        printint(fd, *ap, 10, 1);
        ap++;
      } else if (c == 'x' || c == 'p') {
        printint(fd, *ap, 16, 0);
        ap++;
      } else if (c == 's') {
        s = (char *)*ap;
        ap++;
        if (s == 0)
          s = "(null)";
        while (*s != 0) {
          putc(fd, *s);
          s++;
        }
      } else if (c == 'c') {
        putc(fd, *ap);
        ap++;
      } else if (c == '%') {
        putc(fd, c);
      } else {
        putc(fd, '%');
        putc(fd, c);
      }
      state = 0;
    }
  }
}

void
fprintf(int fd, const char *fmt, ...)
{
  vprintf_old(fd, fmt, (uint *)(void *)&fmt + 1);
}

void
printf(const char *fmt, ...)
{
  vprintf_old(1, fmt, (uint *)(void *)&fmt + 1);
}
