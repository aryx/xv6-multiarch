#include "arch_vm.h"  // pagetable_t

struct rtcdate;

// console.c
void            cprintf(char*, ...);

// file.c
int             fileread(struct file*, uint64, int n);
int             filestat(struct file*, uint64);
int             filewrite(struct file*, uint64, int n);

// fs.c
void            readsb(int dev, struct superblock *sb);
void            iinit(void);
int             readi(struct inode*, int, uint64, uint, uint);
int             writei(struct inode*, int, uint64, uint, uint);

// ide.c
void            ideinit(void);
void            ideintr(void);
void            iderw(struct buf*);

// ioapic.c
void            ioapicenable(int irq, int cpu);
extern uchar    ioapicid;
void            ioapicinit(void);

// kalloc.c
char*           kalloc(void);
void            kfree(char*);
void            kinit1(void*, void*);
void            kinit2(void*, void*);

// kbd.c
void            kbdintr(void);

// lapic.c
void            cmostime(struct rtcdate *r);
int             cpunum(void);
extern volatile uint*    lapic;
void            lapiceoi(void);
void            lapicinit(void);
void            lapicstartap(uchar, uint);
void            microdelay(int);

// log.c
void            initlog(int dev, struct superblock *sb);

// mp.c
extern int      ismp;
int             mpbcpu(void);
void            mpinit(void);
void            mpstartthem(void);

// apic.c
int             acpiinit(void);

// cpuid.c
void            cpuidinit(void);

// picirq.c
void            picenable(int);
void            picinit(void);

// pipe.c
int             piperead(struct pipe*, uint64, int);
int             pipewrite(struct pipe*, uint64, int);

// proc.c
struct proc*    copyproc(struct proc*);
int             either_copyin(void*, int, uint64, uint64);
int             either_copyout(int, uint64, void*, uint64);
void            pinit(void);
void            sleep(void*, struct spinlock*);

// swtch.S
void            swtch(struct context**, struct context*);

// spinlock.c
void            getcallerpcs(void*, uintp*);
void            getstackpcs(uintp*, uintp*);
void            pushcli(void);
void            popcli(void);

// syscall.c - argint/argptr/argstr/fetchstr/syscall/arguintp moved to
// kernel/syscalls/interface_syscall.h. argaddr/fetchuintp are this
// fork's own extra.
int             argaddr(int, uint64*);
int             fetchuintp(uintp, uintp*);

// timer.c
void            timerinit(void);

// trap.c
void            idtinit(void);
void            tvinit(void);

// uart.c
void		uartearlyinit(void);
void            uartinit(void);
void            uartintr(void);
void            uartputc(int);

// vm.c
void            seginit(void);
void            kvmalloc(void);
void            vmenable(void);
pde_t*          setupkvm(void);
char*           uva2ka(pde_t*, char*);
int             allocuvm(pde_t*, uint, uint);
int             deallocuvm(pde_t*, uintp, uintp);
void            freevm(pde_t*);
void            inituvm(pde_t*, char*, uint);
int             loaduvm(pde_t*, char*, struct inode*, uint, uint);
pde_t*          copyuvm(pde_t*, uint);
void            switchuvm(struct proc*);
void            switchkvm(void);
void            clearpteu(pde_t *pgdir, char *uva);

#include "interrupts/interface_trap.h"
#include "syscalls/interface_syscall.h"

#include "console/interface_console.h"
