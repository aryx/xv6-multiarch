// ARM dual-timer module support (SP804)
#include "types.h"
#include "param.h"
#include "arm.h"
#include "mmu.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"

#include "timer.h"

#define TIMER_LOAD    0
#define TIMER_CURVAL  1
#define TIMER_CONTROL 2
#define TIMER_INTCLR  3
#define TIMER_RAWIRQ  4
#define TIMER_MIS     5

void isr_timer (struct trapframe *tp, int irq_idx);

struct spinlock tickslock;
uint ticks;

//#define TIMER0 (0x3F00B400)

// acknowledge the timer, write any value to TIMER_INTCLR should do
static void ack_timer() {
    volatile uint *timer0 = P2V(TIMER0);
    timer0[TIMER_INTCLR] = 1;
    //cprintf ("clear %s", "\n" );
}

// initialize the timer: perodical and interrupt based
void timer_init() {
    volatile uint *timer0 = P2V(TIMER0);

    initlock(&tickslock, "time");

    // Setup the system timer interrupt 
    // Timer frequency = Clk/256 // 0x400 
    //timer0[TIMER_LOAD] = 0x2000;
    timer0[TIMER_LOAD] = 0x1000;
    timer0[TIMER_CONTROL] =
        RPI_ARMTIMER_CTRL_23BIT |
        RPI_ARMTIMER_CTRL_ENABLE |
        RPI_ARMTIMER_CTRL_INT_ENABLE |
        RPI_ARMTIMER_CTRL_PRESCALE_256;
    pic_enable(PIC_TIMER0, isr_timer);
    
    //icount=0; // xsi line
}

// ---------------------------------------------------------------------------
// claude: System Timer (channel 3) - the actual tick source for this port.
//
// Everything above drives the SP804-style "ARM timer" at TIMER0
// (0x3F00B400). That peripheral is real on a Raspberry Pi, but it cannot
// be the tick source here, for two independent reasons:
//
//  1. QEMU does not implement it. hw/arm/bcm2835_peripherals.c registers
//     it with create_unimp(s, &s->armtmr, "bcm2835-sp804", ...) - an
//     unimplemented-device stub that accepts reads and writes and never
//     raises an interrupt. Measured on "-M raspi2b" with "-d int": across
//     a full boot plus 150s of usertests this kernel took exactly ONE IRQ
//     and 7214 SVCs, i.e. it had no preemption at all and was running
//     purely on cooperative scheduling through syscalls. That is why
//     usertests' preempt() hung indefinitely: a CPU-bound child never
//     traps, so pic_dispatch()'s own "istimer -> yield()" never runs.
//  2. Even on real hardware the ARM timer is clocked off the core clock,
//     so its rate moves with frequency scaling. It is the reason Linux on
//     the Pi uses the System Timer instead.
//
// The System Timer is equally real hardware and works in both places, so
// this is not a QEMU-only workaround: it is the same choice the sibling
// forks/arm-pi2 port makes for this identical board (Zhiyi Huang's own
// source/timer.c, timer3init()/timer3intr()), which is why arm-pi2 runs
// preempt() successfully and this port did not. The SP804 code above is
// left in place, and correct, but is no longer registered as an interrupt
// source - see main.c's own call site.
// ---------------------------------------------------------------------------

// register indices into the System Timer block, as uint words
#define ST_CS   0   // control/status: write 1<<n to ack channel n
#define ST_CLO  1   // free-running counter, low 32 bits (1MHz)
#define ST_CHI  2   // free-running counter, high 32 bits
#define ST_C0   3   // compare 0 (GPU firmware)
#define ST_C1   4   // compare 1
#define ST_C2   5   // compare 2 (GPU firmware)
#define ST_C3   6   // compare 3 - ours

// 1MHz counter, so 10000 ticks = 100 interrupts/sec, matching
// forks/arm-pi2's own TIMER_FREQ.
#define SYSTIMER_INTERVAL 10000

void isr_timer3 (struct trapframe *tp, int irq_idx);

// arm compare-3 to fire SYSTIMER_INTERVAL microseconds from now
static void arm_timer3 (void)
{
    volatile uint *st = P2V(SYSTIMER);
    st[ST_C3] = st[ST_CLO] + SYSTIMER_INTERVAL;
}

void timer3_init (void)
{
    initlock(&tickslock, "time");
    arm_timer3();
    pic_enable(PIC_TIMER3, isr_timer3);
}

void isr_timer3 (struct trapframe *tp, int irq_idx)
{
    volatile uint *st = P2V(SYSTIMER);

    // ack channel 3 FIRST: the System Timer holds the match line asserted
    // until the matching CS bit is written back, so acking after
    // re-arming would leave a stale pending bit behind.
    st[ST_CS] = (1 << PIC_TIMER3);

    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);

    arm_timer3();
}

// interrupt service routine for the timer
void isr_timer (struct trapframe *tp, int irq_idx)
{
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
    ack_timer();
}
