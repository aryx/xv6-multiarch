#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#include <stdarg.h>
//
// HOW THE VARARGS ARE READ, AND WHY THIS FILE USES <stdarg.h>
//
// claude: seven of the fourteen ports used to do it by hand instead, and it
// is worth recording what that looked like, because it explains why the
// portable version cannot be written that way:
//
//     static void
//     vprintf(int fd, const char *fmt, uint *ap)   // <- caller passed:
//     {                                            //    (uint*)(void*)&fmt + 1
//       ...
//         case 'd':
//           printint(fd, *ap, 10, 1);
//           ap++;                                  // step one 4-byte slot
//     }
//
// It takes the address of the last named parameter, steps one word past it,
// and then walks forward one uint at a time. That is simple, needs no
// compiler support at all, and is exactly correct - on a 32-bit ABI that
// passes every argument on the stack, in order, in 4-byte slots. i386, ARM32
// and MIPS32 are all close enough to that for it to have worked for years.
//
// It is simply wrong everywhere else. x86-64, AArch64 and RISC-V pass the
// first several arguments in REGISTERS; nothing is on the stack for &fmt+1 to
// point at, and the slots it walks are not the arguments. Even on 32-bit ARM
// the ABI aligns a 64-bit argument to an even register/slot pair, which the
// naive ++ does not account for.
//
// <stdarg.h> exists precisely to hide all of that: va_list/va_arg expand to
// whatever the target ABI actually needs - a register-save area plus an
// overflow pointer on x86-64, a plain pointer on i386. It is a COMPILER
// header, not a libc one, so it is available freestanding on every port here.
//
// The one thing to keep in mind writing against it: va_arg's type must match
// what the caller actually passed after default promotions. That is why "%d"
// below reads an int and not a uint64 - reading uint64 would consume eight
// bytes for a four-byte argument and desynchronise everything after it. One
// fork (riscv64) does exactly that, which is why its copy is the least
// portable of the fourteen rather than the most.


static char digits[] = "0123456789ABCDEF";

static void
putc(int fd, char c)
{
  write(fd, &c, 1);
}

static void
printint(int fd, int xx, int base, int sgn)
{
  char buf[16];
  int i, neg;
  uint x;

  neg = 0;
  if(sgn && xx < 0){
    neg = 1;
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);
  if(neg)
    buf[i++] = '-';

  while(--i >= 0)
    putc(fd, buf[i]);
}

static void
printptr(int fd, uint64 x) {
  int i;
  putc(fd, '0');
  putc(fd, 'x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    putc(fd, digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the given fd. Only understands %d, %x, %p, %s.
void
vprintf(int fd, const char *fmt, va_list ap)
{
  char *s;
  int c, i, state;

  state = 0;
  for(i = 0; fmt[i]; i++){
    c = fmt[i] & 0xff;
    if(state == 0){
      if(c == '%'){
        state = '%';
      } else {
        putc(fd, c);
      }
    } else if(state == '%'){
      if(c == 'd'){
        printint(fd, va_arg(ap, int), 10, 1);
      } else if(c == 'l') {
        printint(fd, va_arg(ap, uint64), 10, 0);
      } else if(c == 'x') {
        printint(fd, va_arg(ap, int), 16, 0);
      } else if(c == 'p') {
        printptr(fd, va_arg(ap, uint64));
      } else if(c == 's'){
        s = va_arg(ap, char*);
        if(s == 0)
          s = "(null)";
        while(*s != 0){
          putc(fd, *s);
          s++;
        }
      } else if(c == 'c'){
        putc(fd, va_arg(ap, uint));
      } else if(c == '%'){
        putc(fd, c);
      } else {
        // Unknown % sequence.  Print it to draw attention.
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
  va_list ap;

  va_start(ap, fmt);
  vprintf(fd, fmt, ap);
}

void
printf(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintf(1, fmt, ap);
}
