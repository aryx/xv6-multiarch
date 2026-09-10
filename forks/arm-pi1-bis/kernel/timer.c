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
// KeyboardUpdate() from a main loop. This kernel has no such loop, so
// the periodic timer interrupt - already firing 100 times/sec for the
// scheduler tick - is the natural place instead. usbkbdgetc() adapts
// KeyboardGetChar()'s "0 means nothing typed" convention to
// consoleintr()'s own "-1 means nothing available" one. Identical fix
// to forks/arm-pi1's own timer.c (notes_arch_arm_pi1.txt bug 10):
//
//  - the "lastusbkey" debounce is load-bearing, not cosmetic: CSUD's
//    own KeyboardGetChar()/KeyWasDown() repeat-suppression does not
//    actually suppress a held key in practice (confirmed for arm-pi1
//    via a raw HID report dump showing a clean single press-then-
//    release underneath a flood of repeated characters) - never emit
//    the same character on two consecutive polls in a row.
//  - the '\n'->'\r' translation: keyboard.s's own KeysNormal/KeysShift
//    tables map Enter to '\n', but this kernel's consoleintr() only
//    treats '\r' as "submit line" and discards a raw '\n' outright.
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
	while(t != getsystemtime());

	return;
}
