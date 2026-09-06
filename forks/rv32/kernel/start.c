#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

void main();
void timerinit();

// entry.S needs one stack per CPU.
__attribute__ ((aligned (16))) char stack0[4096 * NCPU];

// scratch area for timer interrupt, one per CPU.
uint32 mscratch0[NCPU * 32];

// assembly code in kernelvec.S for machine-mode timer interrupt.
extern void timervec();

// entry.S jumps here in machine mode on stack0.
void
start()
{
  // set M Previous Privilege mode to Supervisor, for mret.
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  w_mstatus(x);

  // set M Exception Program Counter to main, for mret.
  // requires gcc -mcmodel=medany
  w_mepc((uint32)main);

  // disable paging for now.
  w_satp(0);

  // delegate all interrupts and exceptions to supervisor mode.
  w_medeleg(0xffff);
  w_mideleg(0xffff);

  // claude: this fork's own start() never configured PMP at all, unlike
  // forks/riscv/kernel/start.c (MIT's current xv6-riscv, forked years
  // after this rv32 port), which sets exactly this pmpaddr0/pmpcfg0 pair
  // right here. Without any PMP entry configured, an unmatched physical
  // memory access from M-mode is allowed by the RISC-V privileged spec's
  // default, but an unmatched access from S/U-mode is DENIED once PMP is
  // implemented at all - so the mret below (M -> S) faulted on its very
  // first post-transition instruction fetch with "Instruction access
  // fault" (confirmed via "qemu-system-riscv32 -d int": cause:1,
  // desc=fault_fetch, looping forever since timervec's own mret keeps
  // returning to the same faulting mepc). README.md's "tested with
  // qemu-5.0.0" suggests that QEMU version's rv32 CPU model either didn't
  // implement PMP or didn't enforce this default-deny - qemu 8.2.2 (this
  // repo's own pinned version) does. TOR (top-of-range, matching
  // forks/riscv's own pmpcfg=0xf choice) from address 0 up to
  // pmpaddr0<<2: 0xffffffff<<2 covers the entire rv32 physical address
  // range (up to Sv32's 34-bit PA width), i.e. "allow everything".
  w_pmpaddr0(0xffffffff);
  w_pmpcfg0(0xf);

  // ask for clock interrupts.
  timerinit();

  // keep each CPU's hartid in its tp register, for cpuid().
  int id = r_mhartid();
  w_tp(id);

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
}

// set up to receive timer interrupts in machine mode,
// which arrive at timervec in kernelvec.S,
// which turns them into software interrupts for
// devintr() in trap.c.
void
timerinit()
{
  // each CPU has a separate source of timer interrupts.
  int id = r_mhartid();

  // ask the CLINT for a timer interrupt.
  uint32 interval = 1000000; // cycles; about 1/10th second in qemu.
  *(uint64*)CLINT_MTIMECMP(id) = *(uint64*)CLINT_MTIME + interval;

  // prepare information in scratch[] for timervec.
  // scratch[0..3] : space for timervec to save registers.
  // scratch[4] : address of CLINT MTIMECMP register.
  // scratch[5] : desired interval (in cycles) between timer interrupts.
  uint32 *scratch = &mscratch0[32 * id];
  scratch[4] = CLINT_MTIMECMP(id);
  scratch[5] = interval;
  w_mscratch((uint32)scratch);

  // set the machine-mode trap handler.
  w_mtvec((uint32)timervec);

  // enable machine-mode interrupts.
  w_mstatus(r_mstatus() | MSTATUS_MIE);

  // enable machine-mode timer interrupts.
  w_mie(r_mie() | MIE_MTIE);
}
