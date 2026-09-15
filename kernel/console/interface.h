#ifndef INTERFACE_CONSOLE_H
#define INTERFACE_CONSOLE_H

// claude: console.c/uart.c have no arch_*.h family (too divergent to
// share a single file - see plan_factorization.md's own "checked
// console.c/timer.c - not viable" finding), but every fork's own
// kernel/console/<arch>/{console.c,uart.c} independently settled on
// the same function names for the same roles. Each fork's own
// <arch>_defs.h #includes this file right before those definitions,
// so the compiler checks the real definition against the declaration
// here - see kernel/processes/interface.h for the same technique
// applied to arch_proc.h.
//
// Reached via "console/interface.h" (a path, not a bare name)
// plus a plain -I../../kernel in each fork's own Makefile, rather than
// the usual forks/<arch>/kernel/ symlink - a pilot for cutting down
// the symlink count; kernel/processes/interface.h and its
// siblings still use the symlink form for now.

// void consoleinit(void)
//   Set up the console device (device-switch table entry, UART init,
//   lock init). Called once from main(). Same name and signature on
//   every fork.
void consoleinit(void);

// void consputc(int c)
//   Write one character to whatever this fork's console actually is
//   (UART, and on some forks also a framebuffer/CGA text screen) -
//   the single point every higher-level print path funnels through.
//   Same name and signature on every fork.
void consputc(int c);

// void uartinit(void)
//   Initialize this board's UART hardware (baud rate, FIFO, interrupt
//   enable). Called once from consoleinit(). arm64-pi4 alone takes a
//   UART index (its board exposes more than one UART); arm spells
//   this uart_init() instead, which just coexists harmlessly here.
#ifdef ARM64_PI4_UARTINIT_N
void uartinit(int n);
#else
void uartinit(void);
#endif

// void uartputc(int c)
//   Write one byte to the UART, blocking until the transmit FIFO has
//   room. The arm-pi board family takes uint instead of int (same
//   meaning); riscv64 has no uartputc at all (uartputc_sync instead),
//   which just coexists harmlessly here.
#ifdef ARM_PI_UART_UINT
void uartputc(uint c);
#else
void uartputc(int c);
#endif

// void uartintr(void)
//   UART receive-interrupt handler: drain available bytes via
//   uartgetc() and hand each to consoleintr(). Called from this
//   fork's own trap/interrupt dispatch, not from portable code. Not
//   present under this name on arm or the arm-pi board family (a
//   different interrupt-entry design there) - harmless to declare
//   unconditionally since nothing on those forks calls it either.
void uartintr(void);

// int uartgetc(void)
//   Read one byte from the UART if one is waiting, else -1.
//   Non-blocking; called from uartintr() and, on the callback-style
//   consoleintr() family below, passed BY NAME as the callback itself.
//   Left undeclared here on purpose: 9 forks give it internal linkage
//   (static) since nothing outside their own uart.c ever calls it,
//   while 4 forks (plus arm) leave it external - a live extern
//   declaration would conflict with the static definitions.

// void consoleintr(...)
//   Handle one incoming character from the console's own input
//   device (echo it, buffer it, wake a waiting consoleread() on
//   newline). Two real, incompatible designs, split along the same
//   legacy/modern boundary as bio.c/buf.h/conf.h elsewhere in this
//   tree:
//     - legacy: consoleintr(int (*getc)(void)) - takes a callback and
//       calls it itself, once per available byte.
//     - modern: consoleintr(int c) - the caller (uartintr()) has
//       already fetched the byte and passes it directly.
//   Not a prototype either family can literally share; documented as
//   two variants rather than one.
#if 0
void consoleintr(int (*getc)(void));  /* legacy */
void consoleintr(int c);              /* modern */
#endif

// int consoleread(...), int consolewrite(...)
//   Read from / write to the console as a file (fd 0/1/2 before a
//   process opens anything else). Three signature generations found,
//   not two - a real third family, not just legacy-vs-modern:
//     - modern, pointer-width-correct: (int user_dst, uintp dst, int n)
//     - modern, but hardcoded 32-bit: (int user_dst, uint32 dst, int n) -
//       same shape as above on a fork that never needed a wider
//       pointer, not yet expressed as uintp.
//     - legacy, inode-based: (struct inode *ip, char *dst, int n) -
//       predates the user_dst/uint64 generic read/write API entirely,
//       operates directly on the inode instead.
#if 0
int consoleread(int user_dst, uintp dst, int n);
int consolewrite(int user_src, uintp src, int n);
#endif

#endif /* INTERFACE_CONSOLE_H */
