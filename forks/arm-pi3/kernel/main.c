/*****************************************************************
*       main.c
*       by Zhiyi Huang, hzy@cs.otago.ac.nz
*       University of Otago
*
********************************************************************/


#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "arm.h"
#include "mailbox.h"

#include <uspi.h>
#include <uspios.h>

extern char end[]; // first address after kernel loaded from ELF file
extern char bss_start[]; // claude: first address of .bss - see bsszero()
//extern pde_t *kpgdir;
extern volatile uint *mailbuffer;
extern unsigned int pm_size;
volatile extern u32 cpu_sig[];

void basic_delay(uint n) {
  uint sum;
  volatile uint i;
  for (i = 0; i < n; i++) {
    sum += i;
  }
}

void OkLoop()
{
   setgpiofunc(12, 1); // gpio 18 for Ok Led, set as an output
   setgpioval(12, 1);
   while(1){
        setgpioval(12, 1);
        basic_delay(2000000);
        setgpioval(12, 0);
        basic_delay(2000000);
   }
}

void NotOkLoop()
{
   setgpiofunc(12, 1); // gpio 18 for Ok Led, set as an output
   while(1){
        setgpioval(12, 1);
        basic_delay(500000);
        setgpioval(12, 0);
        basic_delay(500000);
   }
}

unsigned int getpmsize()
{
    create_request(mailbuffer, MPI_TAG_GET_ARM_MEMORY, 8, 0, 0);
    writemailbox((uint *)mailbuffer, 8);
    readmailbox(8);
    if(mailbuffer[1] != 0x80000000) cprintf("Error readmailbox: %x\n", MPI_TAG_GET_ARM_MEMORY);
    return mailbuffer[MB_HEADER_LENGTH + TAG_HEADER_LENGTH+1];
}

void machinit(void)
{
  int cpu;
  memset(cpus, 0, sizeof(struct cpu)*NCPU);
  for (cpu = 0; cpu < NCPU; cpu++) {
    cpus[cpu].id = cpu;
    cpus[cpu].first_sched = 1;
  }
}


void enableirqminiuart(void);

int lock __attribute((aligned (64))) = 0;

void locktest(void) {
  invalidate_dcache_range(&lock, 4);
  cprintf("Lock value: 0x%x\n", lock);
  cprintf("Lock addr: 0x%x\n", &lock);
  cprintf("Locking...\n");
  spin_acquire((void *) &lock);
  cprintf("locked\n");
  cprintf("Lock value: 0x%x\n", lock);
  spin_release((void *) &lock);
  cprintf("Unlocked\n");
  cprintf("Lock value: 0x%x\n", lock);
}

/* claude: secondary-core boot handshake.
 *
 * The original code used bare SEV/WFE pairs (signal_event()/
 * wait_for_event()) to sequence CPU0 against cores 1-3. That cannot
 * work: WFE returns immediately when the Event Register is already set
 * (armstub64.S broadcasts "sev" from every core as it passes through),
 * and is architecturally allowed to return spuriously besides - so
 * every wait_for_event() in aux_main() fell straight through. Cores 1-3
 * therefore ran aux_main() concurrently with cmain()'s own early init,
 * which meant they:
 *   - called cprintf() before consoleinit() had initialised cons.lock
 *     (hence the visibly interleaved mid-string boot output) and before
 *     gpuinit() had set up the framebuffer gpuputc() writes to;
 *   - wrote cpus[] (curr_cpu->kpgdir, via aux_mmu_init()) before
 *     machinit() memset that array, so their state was silently erased;
 *   - ran aux_mmu_init() before mmuinit1() had finished the master page
 *     table, so each secondary copied a half-finished page directory.
 *
 * Replaced by an explicit monotonic stage counter that each waiter
 * re-reads. SEV is kept only as a wake-up hint; correctness comes from
 * the flag. Explicitly initialised to 0 so that -fno-zero-initialized-
 * in-bss keeps it in .data (which IS part of the loaded raw image -
 * .bss is NOBITS here and nothing zeroes it).
 */
#define MP_CPU0_READY   1  /* cpus[], console, bootstrap allocator, GPU up */
#define MP_INIT_DONE    2  /* all of cmain()'s single-threaded init done */

volatile int mp_stage = 0;

static void
mp_wait_stage(int stage)
{
  while (mp_stage < stage)
    ;
  dsb_barrier();
}

/**
 * Blocks the current CPU until all other cores report 
 * particular status.
 */
void wait_on_cores(enum cpustate target)
{
  while (1) {
    int ready = 1;
    uchar local_cpu = curr_cpu->id;
    for (int i = 0; i < NCPU; i++) {
      if (i == local_cpu) {
	continue;
      }
      if (cpu_sig[i] != target) {
	ready = 0;
      }
    }
    if (ready) {
      break;
    }
  }
}
  

void startothers(void)
{
  curr_cpu->started = CENTRY;
  /* claude: everything a secondary needs before it may print or touch
   * cpus[] is up by now - release them past aux_main()'s first gate. */
  dsb_barrier();
  mp_stage = MP_CPU0_READY;
  signal_event();
  cprintf("CPU %x: Starting other cores\n", cpu_id());
  invalidate_dcache_range((void*) cpu_sig, 20);
  wait_on_cores(CENTRY);
}


/**
 * Configure and start secondary CPUs.
 * aux_main assumes the primary CPU has already
 * initialised shared OS data structures.
 */
void aux_main(void)
{
  /* claude: wait for cmain() to finish machinit()/consoleinit()/kinit1()/
   * gpuinit() - before that point cpus[] is about to be memset, cons.lock
   * does not exist yet and gpuputc() has no framebuffer. */
  mp_wait_stage(MP_CPU0_READY);
  cpu_sig[curr_cpu->id] = CENTRY;
  cprintf("CPU %d: Booted\n", curr_cpu->id);
  /* claude: and wait for ALL of cmain()'s remaining init before running
   * any of our own. Two separate reasons, both real:
   *   - aux_mmu_init() copies the master page table, so it must not run
   *     until mmuinit1() has added the high-RAM mappings and dropped the
   *     identity map of the first MB;
   *   - tvinit() calls kalloc() four times, and kmem.use_lock stays 0
   *     from kinit1() all the way until kinit2() returns - i.e. the
   *     free list is deliberately lock-free during single-threaded
   *     boot. A secondary calling kalloc() in that window corrupts it;
   *     the observed symptom was kalloc() handing back 0 and the very
   *     next memset(ptr, 0, PGSIZE) taking a write translation fault at
   *     address 0 ("unexpected trap 4 ... far 0", DFSR 0x805) on
   *     several cores at once.
   */
  mp_wait_stage(MP_INIT_DONE);
  tvinit();
  cprintf("CPU %d: tvinit Ok.\n", curr_cpu->id);
  aux_mmu_init();
  //Can secondary cores see the primary core tvint alloc? Move tvinit earlier in main?
  cprintf("CPU %d: aux_mmu_init: Ok.\n", curr_cpu->id);
  cprintf("CPU %d: Handing off to scheduler.\n", curr_cpu->id);
  scheduler();
}


// claude: nothing in this fork ever zeroed .bss - kernel.ld only declared
// the section, and objcopy's raw binary output omits it entirely because
// it is NOBITS, so every uninitialized global started out holding
// whatever the previous occupant of that RAM left behind. QEMU happens to
// hand out zeroed RAM at reset, which is why this stayed invisible here,
// but real Pi 3 hardware makes no such promise. Identical bug and
// identical fix to forks/arm-pi1's own Bug 5, forks/arm-pi1-bis's
// equivalent, and forks/arm-pi2's own Bug 5 (see their notes_arch_*.txt).
// Safe for real hardware either way: this only makes true what the C
// standard already guarantees for globals with no initializer.
//
// Runs on CPU0 only, and only ever touches .bss - the secondary cores are
// parked in aux_main()'s own mp_wait_stage(MP_CPU0_READY) spin at this
// point, and mp_stage itself lives in .data (see its own comment), so
// this cannot erase the flag they are waiting on.
void bsszero(void)
{
  char *p;
  for(p = bss_start; p < end; p++)
    *p = 0;
}

int cmain()
{
    bsszero();
    mmuinit0();
    machinit();
    #if defined (RPI1) || defined (RPI2)
    uartinit();
    #elif defined (FVP)
    uartinit_fvp();
    #endif
    dsb_barrier();
    consoleinit();
    cprintf("\nHello World from xv6\n");
    kinit1(end, P2V((8*1024*1024)+PHYSTART));
    cprintf("kinit1: Ok\n");
    mailboxinit();
    cprintf("Mailbox init: Ok\n");
    gpuinit();
    cprintf("GPU init: Ok\n");
    // collect some free space (8 MB) for imminent use
    // the physical space below 0x8000 is reserved for PGDIR and kernel stack
    //kpgdir=p2v(K_PDX_BASE);
    curr_cpu->kpgdir = p2v(K_PDX_BASE);
    
    pm_size = getpmsize();;
    cprintf("ARM memory: %x\n", pm_size);
    startothers();
    mmuinit1();
    cprintf("mmuinit1: OK\n");
    //Alert secondary cores to copy the page table and contiue.
    /* claude: the secondaries are NOT released here any more - see
     * aux_main()'s own MP_INIT_DONE wait for why they have to stay
     * parked until kinit2() has switched kmem over to locked mode. */
    cprintf("ARM xv6 MP USB\n");
    pinit();
    cprintf("pinit: OK\n");
    tvinit();
    cprintf("tvinit: OK\n");
    binit();
    cprintf("binit: OK\n");
    fileinit();
    cprintf("fileinit: OK\n");
    iinit();
    cprintf("iinit: OK\n");
    ideinit();
    cprintf("ideinit: OK\n");
    kinit2(P2V((8*1024*1024)+PHYSTART), P2V(pm_size));
    cprintf("kinit2: OK\n");
    usbinit();
    userinit();
    cprintf("userinit: OK\n");
    timer3init();
    cprintf("timer3init: OK\n");
    enableirqminiuart();
    /* claude: single-threaded init is over - every shared structure the
     * secondaries touch (the master page table, kmem's now-locked free
     * list, the buffer/inode/file tables) is built and locked. Release
     * them into tvinit()/aux_mmu_init()/scheduler(). The flag, not the
     * SEV, is what they actually test - see mp_wait_stage() above. */
    dsb_barrier();
    mp_stage = MP_INIT_DONE;
    signal_event();
    cprintf("Handing off to scheduler...\n");
    
    scheduler();
    NotOkLoop();
    return 0;
}
