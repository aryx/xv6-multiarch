#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "mips.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  // Set vector base address.
  write_cop0_status(read_cop0_status() & ~STATUS_BEV);

  // TLB refill
  memmove((void *)EV_TLBREFILL, tlbrefill, ((uint)tlbrefill_end - (uint)tlbrefill));

  // general traps
  memmove((void *)EV_OTHERS, gentraps, ((uint)gentraps_end - (uint)gentraps));
  
  initlock(&tickslock, "time");
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  uint exccode = tf->cause & CAUSE_EXC;
  // claude: see the yield() below - this has to be decided inside the
  // EXC_INT switch, where the IRQ number is actually known.
  int istimer = 0;
  if(exccode == EXC_SYSCALL){
    if(proc->killed)
      exit();
    proc->tf = tf;
    proc->tf->epc += 4; // Just increase epc. Assume the syscall NOT to be in a delay slot.
    syscall();
    if(proc->killed)
      exit();
    return;
  }

  if(exccode == EXC_INT){
    uint irq = picgetirq();
    switch(irq){
    case IRQ_TIMER:
      istimer = 1;
      if(cpu->id == 0){
        acquire(&tickslock);
        ticks++;
        wakeup(&ticks);
        release(&tickslock);
      }
      break;
    case IRQ_IDE:
      ideintr();
      break;
    case IRQ_IDE+1:
      // Bochs generates spurious IDE1 interrupts.
      break;
    case IRQ_KBD:
      kbdintr();
      break;
    case IRQ_COM1:
      uartintr();
      break;
    case IRQ_SPURIOUS_MS:
    case IRQ_SPURIOUS_SL:
      cprintf("cpu%d: spurious interrupt at %x\n",
          cpu->id, tf->epc);
      break;
    }
    picsendeoi(irq);
  } else {
    if(proc == 0 || (tf->status & STATUS_KSU) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (bad=0x%x)\n",
              tf->cause, cpu->id, tf->epc, read_cop0_bad());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            proc->pid, proc->name, tf->cause, cpu->id, tf->epc, 
            read_cop0_bad());
    proc->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running 
  // until it gets to the regular system call return.)
  if(proc && proc->killed && (tf->status & STATUS_KSU))
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  //
  // claude: was "tf->cause == T_IRQ0+IRQ_TIMER", which can never be true
  // on MIPS and so meant this kernel NEVER preempted anything. That test
  // is inherited verbatim from x86 xv6, where tf->trapno really does hold
  // T_IRQ0+<irq>. Here tf->cause is the CP0 Cause register: the exception
  // code is a 5-bit field at bits 6:2 (CAUSE_EXC, decoded into "exccode"
  // at the top of this function), sharing the register with the IP
  // pending bits, BD, CE and friends. For a hardware interrupt exccode is
  // 0 and the register as a whole is something like 0x00008000 - never
  // the constant 32 this compared against. So the tick was delivered,
  // acked, and counted (ticks++ above), but no process ever gave up the
  // CPU for it.
  //
  // The visible symptom was usertests' preempt() hanging forever: it
  // forks CPU-bound children that never make a syscall, so with no
  // timer-driven yield() nothing else ever runs. It had been commented
  // out as "real memory pressure / multi-process timing under emulation".
  // The timer itself was fine all along - measured 346 specific-EOIs for
  // IRQ 0 in 15s of idle via "-trace memory_region_ops_write" on the
  // i8259 region, i.e. ~23Hz, exactly what timerinit()'s divisor gives.
  //
  // istimer is set in the EXC_INT switch above, where picgetirq() has
  // already told us which IRQ this was - the same shape forks/arm's own
  // device/gic.c pic_dispatch() uses.
  if(proc && proc->state == RUNNING && istimer)
    yield();

  // Check if the process has been killed since we yielded
  if(proc && proc->killed && (tf->status & STATUS_KSU))
    exit();
}
