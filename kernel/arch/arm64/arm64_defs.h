#include "arch_vm.h"

struct trapframe;

// console.c
void            consputc(int);

// file.c
int             filestat(struct file*, uint64 addr);

// fs.c
void            iinit();

// ramdisk.c
void            ramdiskinit(void);
void            ramdiskintr(void);
void            ramdiskrw(struct buf*);

// kalloc.c
void*           kalloc(void);
void            kfree(void *);
void            kinit1(void *, void *);
void            kinit2(void *, void *);

// log.c
void            initlog(int, struct superblock*);

// printf.c
void            printf(char*, ...);
void            printfinit(void);

// proc.c
void            proc_mapstacks(pagetable_t);
struct cpu*     mycpu(void);
struct cpu*     getmycpu(void);
struct proc*    myproc();
void            procinit(void);
void            sleep(void*, struct spinlock*);
int             either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
int             either_copyin(void *dst, int user_src, uint64 src, uint64 len);

// swtch.S
void            swtch(struct context*, struct context*);

// spinlock.c
void            push_off(void);
void            pop_off(void);

// syscall.c - see kernel/syscalls/interface.h

// trap.c - trapinit()/kerneltrap()/clockintr()/devintr() moved to
// kernel/interrupts/interface.h. trapinithart() stays here.
// usertrapret() is really an assembly label (uservec.S/trapasm.S),
// called from proc.c as if it were a C function - not part of
// trap.c's own dispatch, so not covered by that interface either.
void            trapinithart(void);
void            usertrapret(struct trapframe *);

// uart.c
void            uartinit(void);
void            uartintr(void);
void            uartputc(int);
void            uartputc_sync(int);
int             uartgetc(void);

// vm.c
void            kvminit(void);
void            kvminithart(void);
void            kvmmap(pagetable_t, uint64, uint64, uint64, uint64);
int             mappages(pagetable_t, uint64, uint64, uint64, uint64);
pagetable_t     uvmcreate(void);
void            uvminit(pagetable_t, uchar *, uint);
uint64          uvmalloc(pagetable_t, uint64, uint64);
uint64          uvmdealloc(pagetable_t, uint64, uint64);
int             uvmcopy(pagetable_t, pagetable_t, uint64);
void            uvmfree(pagetable_t, uint64);
void            uvmunmap(pagetable_t, uint64, uint64, int);
void            uvmclear(pagetable_t, uint64);
uint64          uva2ka(pagetable_t, uint64);
int             copyinstr(pagetable_t, char *, uint64, uint64);
void            switchuvm(struct proc *);
void            switchkvm(void);

// gicv3.c
void            gicv3init(void);
void            gicv3inithart(void);
uint32          gic_iar(void);
int             gic_iar_irq(uint32);
void            gic_eoi(uint32);

// timer.c
void            timerinit(void);
void            timerintr(void);

// virtio_disk.c
void            virtio_disk_init(void);
void            virtio_disk_rw(struct buf *, int);
void            virtio_disk_intr(void);

#include "interrupts/interface.h"
#include "syscalls/interface.h"

#include "console/interface.h"
