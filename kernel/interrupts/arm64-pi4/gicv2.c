#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "aarch64.h"
#include "defs.h"

//
// aarch64 Global Interrupt Controller (GICv2).
//

// GICD base
#define RegD(reg) ((volatile uint32 *)(GICDBASE + (reg)))
// GICC base
#define RegC(reg) ((volatile uint32 *)(GICCBASE + (reg)))

#define D_CTLR          0x0
#define D_TYPER         0x4
#define D_ISENABLER(n)  (0x100 + (uint64)(n) * 4)
#define D_ICENABLER(n)  (0x180 + (uint64)(n) * 4)
#define D_ISPENDR(n)    (0x200 + (uint64)(n) * 4)
#define D_ICPENDR(n)    (0x280 + (uint64)(n) * 4)
#define D_IPRIORITYR(n) (0x400 + (uint64)(n) * 4)
#define D_ITARGETSR(n)  (0x800 + (uint64)(n) * 4)
#define D_ICFGR(n)      (0xc00 + (uint64)(n) * 4)

#define C_CTLR  0x0 
#define C_PMR   0x4
#define C_IAR   0xc
#define C_EOIR  0x10
#define C_HPPIR 0x18
#define C_AIAR  0x20
#define C_AEOIR 0x24

static void gic_setup_ppi(uint32 intid);
static void gic_setup_spi(uint32 intid);

static void
giccinit()
{
  *RegC(C_CTLR) = 0;
  *RegC(C_PMR) = 0xff;
}

static void
gicdinit()
{
  *RegD(D_CTLR) = 0;
}

// claude: global, once, by CPU0 only - SPIs (INTID >= 32) live in
// distributor registers that really are shared, and this one is
// explicitly targeted at CPU0 by gic_setup_spi().
void
gicv2init()
{
  gic_setup_spi(UART0_IRQ);
}

// claude: per-CPU, by every hart - and gic_setup_ppi(TIMER0_IRQ) MUST be
// here rather than in gicv2init() above, where it used to be.
//
// TIMER0_IRQ is 27, the ARM generic virtual timer's PPI. For INTIDs 0-31
// the GICv2 distributor registers gic_setup_ppi() touches - ISENABLER0,
// ICPENDR0, IPRIORITYR0-7 - are BANKED per CPU interface: each core sees
// its own private copy. So CPU0 running gic_setup_ppi() enabled the timer
// PPI for CPU0 and nobody else, and cores 1-3 came up with their virtual
// timer masked at the GIC, permanently.
//
// The visible consequence was a hang, not a lost tick: with no timer
// interrupt, a core running a user process that spins in a tight loop
// never re-enters the kernel, so trap.c's "which_dev == 2 -> yield()"
// never fires and the scheduler never gets that core back. usertests'
// preempt() deadlocks on exactly this - it forks three spinning
// children, kills them, then wait()s. kill() only sets p->killed; the
// victim exits when it NEXT traps into the kernel, which for a spinning
// process is only ever a timer interrupt. On cores 1-3 that never
// arrives, so the children never die and the three wait(0) calls block
// forever.
//
// Not a QEMU artifact: PPI register banking is architectural GICv2, and
// a real Pi 4's GIC-400 behaves identically.
void
gicv2inithart()
{
  giccinit();
  gicdinit();

  gic_setup_ppi(TIMER0_IRQ);

  *RegC(C_CTLR) |= 0x1;
  *RegD(D_CTLR) |= 0x1;
}

static void
gic_enable_int(uint32 intid)
{
  *RegD(D_ISENABLER(intid / 32)) |= 1 << (intid % 32);
}

static void
gic_clear_pending(uint32 intid)
{
  *RegD(D_ICPENDR(intid / 32)) |= 1 << (intid % 32);
}

static void
gic_set_prio0(uint32 intid)
{
  // set priority to 0
  *RegD(D_IPRIORITYR(intid / 4)) &= ~((uint32)0xff << (intid % 4 * 8));
}

static void
gic_set_target(uint32 intid, uint32 cpuid)
{
  uint32 i = *RegD(D_ITARGETSR(intid / 4));
  i &= ~((uint32)0xff << (intid % 4 * 8));
  *RegD(D_ITARGETSR(intid / 4)) = i | ((uint32)(1 << cpuid) << (intid % 4 * 8));
}

static void
gic_setup_ppi(uint32 intid)
{
  gic_set_prio0(intid);
  gic_clear_pending(intid);
  gic_enable_int(intid);
}

static void
gic_setup_spi(uint32 intid)
{
  gic_set_prio0(intid);

  // all interrupts are handled by cpu0　
  gic_set_target(intid, 0);

  gic_clear_pending(intid);
  gic_enable_int(intid);
}

// irq from iar
int
gic_iar_irq(uint32 iar)
{
  return iar & 0x3ff;
}

// interrupt acknowledge register:
// ask GIC what interrupt we should serve.
uint32
gic_iar()
{
  return *RegC(C_IAR);
}

// tell GIC we've served this IRQ.
void
gic_eoi(uint32 iar)
{
  *RegC(C_EOIR) = iar;
}

