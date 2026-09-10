/*****************************************************************
*       timer.c
*       by Zhiyi Huang, hzy@cs.otago.ac.nz
*       University of Otago
*
********************************************************************/

// The System Timer peripheral

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "proc.h"
#include "traps.h"
#include "arm.h"
#include "spinlock.h"

#define TIMER_REGS_BASE		0xFE003000
#define CONTROL_STATUS		0x0 // control/status
#define COUNTER_LO		0x4 // the time-stamp lower 32 bits
#define COUNTER_HI		0x8 // the time-stamp higher 32 bits
#define COMPARE0		0xc  // compare 0
#define COMPARE1                0x10 // compare 1
#define COMPARE2                0x14 // compare 2
#define COMPARE3                0x18 // compare 3

#define TIMER_FREQ		10000  // interrupt 100 times/sec.

void 
enabletimer3irq(void)
{
        intctrlregs *ip;

        ip = (intctrlregs *)INT_REGS_BASE;
        ip->gpuenable[0] |= 1 << IRQ_TIMER3; // enable the system timer3 irq
}


void 
timer3init(void)
{
uint v;

	enabletimer3irq();

	v = inw(TIMER_REGS_BASE+COUNTER_LO);
	v += TIMER_FREQ;

	outw(TIMER_REGS_BASE+COMPARE3, v);
	ticks = 0;
}

// claude: CSUD (csud/) is a polling driver, not interrupt-driven - the
// Baking Pi tutorial code it comes from expects its own caller to poll
// KeyboardUpdate() from a main loop. This kernel has no such loop (it
// goes straight into the scheduler after boot), so the periodic timer
// interrupt - already firing 100 times/sec for the scheduler tick - is
// the natural place instead. usbkbdgetc() adapts KeyboardGetChar()'s
// "0 means nothing typed" convention to consoleintr()'s own "-1 means
// nothing available" one (see uart.c's own uartgetc() for the same
// shape), so keyboard input is fed into the exact same input-buffer
// path as UART input, indistinguishably from the shell's point of view.
//
// claude: the extra "lastusbkey" debounce below is load-bearing, not
// cosmetic. Confirmed directly (temporarily logging the raw 8-byte USB
// HID report csud/'s own HidReadDevice() reads every poll -
// notes_arch_arm_pi1.txt has the full writeup): the UNDERLYING report
// genuinely transitions cleanly, exactly once, from "no keys down" to
// "'l' down" and back - the USB layer is not the problem. But
// KeyboardGetChar()'s own repeat-suppression (csud/source/device/hid/
// keyboard.c's KeyWasDown()/KeyboardOldDown, keyed by matching the raw
// USB usage VALUE against a small fixed-size history) does not
// actually suppress it in practice here - a single held key still
// produces a long run of identical characters. Rather than debug
// CSUD's own hand-written state machine further, this keeps the same
// safety property directly: never emit the SAME character on two
// consecutive polls in a row. A key genuinely typed twice in a row
// (needs a release cycle first, i.e. KeyboardGetChar() returning 0 -
// "no key" - at least once between the two presses) is unaffected;
// only a literal same-character-every-single-10ms-tick run is
// collapsed to one.
static uint lastusbkey;

static int
usbkbdgetc(void)
{
  char c = KeyboardGetChar();

  if(c == 0){
    lastusbkey = 0;
    return -1;
  }
  if((uchar)c == lastusbkey)
    return -1;
  lastusbkey = (uchar)c;
  // claude: source/keyboard.s's own KeysNormal/KeysShift tables map
  // the Enter key to '\n' (0x0a) - a reasonable, standard choice for a
  // USB keyboard driver in isolation, but this kernel's own
  // consoleintr() (source/console.c) explicitly DISCARDS a raw '\n'
  // and only treats '\r' (0x0d) as "submit line" (a real
  // serial-terminal convention documented in notes_arch_arm_pi1.txt -
  // the UART path never sends '\n' for Enter either). Translated here,
  // at the USB-specific glue layer, rather than changing keyboard.s's
  // own table (which would be a wrong fix for any OTHER hypothetical
  // caller expecting standard ASCII) or consoleintr() itself (shared
  // with the UART path, which is correct as-is).
  if((uchar)c == '\n')
    return '\r';
  return (int)(uchar)c;
}

void
timer3intr(void)
{
uint v;
//cprintf("timer3 interrupt: %x\n", inw(TIMER_REGS_BASE+CONTROL_STATUS));
	outw(TIMER_REGS_BASE+CONTROL_STATUS, (1 << IRQ_TIMER3)); // clear timer3 irq

	ticks++;
	wakeup(&ticks);

	KeyboardUpdate();
	consoleintr(usbkbdgetc);

	// reset the value of compare3
	v=inw(TIMER_REGS_BASE+COUNTER_LO);
	v += TIMER_FREQ;
	outw(TIMER_REGS_BASE+COMPARE3, v);
}

void
delay(uint m)
{
	unsigned long long t;

	if(m == 0) return;

	t = getsystemtime() + m;
	// claude: was "while(t != getsystemtime());" - an exact-equality
	// busy-wait. Confirmed as a real hang under QEMU's "-M raspi1ap"
	// (traced via "-d in_asm": stuck calling delay()/getsystemtime() in
	// a loop during uartinit()'s own GPIO settle delay, never
	// progressing) - the emulated timer counter can advance by more
	// than 1 between two reads and skip straight past the exact target,
	// so "!=" never becomes false and this spins for the remaining
	// ~2^64 range of a 64-bit counter. Same root cause, and the same
	// fix (a signed "has the target been reached or passed" compare
	// instead of exact match), as ~/principia/kernel/COMPILE/9/bcm/
	// clock.c's own "claude:"-tagged comment on this exact class of bug
	// for the same real hardware family - a pure robustness fix, not a
	// QEMU-only workaround: real hardware's timer normally never skips
	// past the target at all, so this changes nothing there, but is
	// strictly safer if it ever did.
	while((long long)(getsystemtime() - t) < 0);

	return;
}
