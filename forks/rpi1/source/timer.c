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

void 
timer3intr(void)
{
uint v;
//cprintf("timer3 interrupt: %x\n", inw(TIMER_REGS_BASE+CONTROL_STATUS));
	outw(TIMER_REGS_BASE+CONTROL_STATUS, (1 << IRQ_TIMER3)); // clear timer3 irq

	ticks++;
	wakeup(&ticks);

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
