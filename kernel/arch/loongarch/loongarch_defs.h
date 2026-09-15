#include "arch_vm.h"

// console.c
void            consputc(int);

// printf.c
void            printf(char*, ...);
void            printfinit(void);

// spinlock.c
void            push_off(void);
void            pop_off(void);

// uart.c
void            uartinit(void);
void            uartintr(void);
void            uartputc(int);
void            uartputc_sync(int);
int             uartgetc(void);

// trap.c - trapinit()/kerneltrap()/clockintr()/devintr() moved to
// kernel/interrupts/interface_trap.h. usertrapret() has no
// declaration there (a real per-fork name/signature exception).
void            usertrapret(void);

// proc.c
int             cpuid(void);
void            proc_mapstacks(pagetable_t);
pagetable_t     proc_pagetable(struct proc *p);
void            proc_freepagetable(pagetable_t, uint64);
struct cpu*     mycpu(void);
struct cpu*     getmycpu(void);
struct proc*    myproc();
void            procinit(void);
void            sleep(void*, struct spinlock*);
int             either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
int             either_copyin(void *dst, int user_src, uint64 src, uint64 len);

// swtch.S
void            swtch(struct context*, struct context*);

// kalloc.c
void*           kalloc(void);
void            kfree(void *);
void            kinit(void);

// vm.c
void            tlbinit(void);
void            vminit(void);
pte_t *         walk(pagetable_t pagetable, uint64 va, int alloc);
int             mappages(pagetable_t, uint64, uint64, uint64, uint64);
pagetable_t     uvmcreate(void);
void            uvminit(pagetable_t, uchar *, uint);
uint64          uvmalloc(pagetable_t, uint64, uint64);
uint64          uvmdealloc(pagetable_t, uint64, uint64);
int             uvmcopy(pagetable_t, pagetable_t, uint64);
void            uvmfree(pagetable_t, uint64);
void            uvmunmap(pagetable_t, uint64, uint64, int);
void            uvmclear(pagetable_t, uint64);
uint64          walkaddr(pagetable_t, uint64);
int             copyinstr(pagetable_t, char *, uint64, uint64);

// apic.c
void            apic_init(void);
void            apic_complete(uint64 irq);

// ramdisk.c
void            ramdiskinit(void);
void            ramdiskrw(struct buf*, int write);

// log.c
void            initlog(int, struct superblock*);

// fs.c
void            iinit();

// file.c
int             filestat(struct file*, uint64 addr);

// extioi.c
void            extioi_init(void);
uint64          extioi_claim(void);
void            extioi_complete(uint64);

#include "interface_trap.h"
#include "interface_syscall.h"

#include "console/interface_console.h"
