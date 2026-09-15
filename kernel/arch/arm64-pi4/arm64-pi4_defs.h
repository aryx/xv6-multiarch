#include "arch_vm.h"

struct trapframe;
enum pinmode;

void cpu_sync_cache(void *va, uint64 sz);

// console.c
void            consputc(int);

// file.c
int             filestat(struct file*, uint64 addr);

// fs.c
void            iinit();

// ramdisk.c
void            ramdiskinit(void);
void            ramdiskintr(void);
void            ramdiskrw(struct buf*, int);

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

// syscall.c - see kernel/syscalls/interface_syscall.h

// trap.c - trapinit()/kerneltrap()/clockintr()/devintr() moved to
// kernel/interrupts/interface_trap.h. trapinithart() stays here.
// usertrapret() is really an assembly label (trapasm.S), called from
// proc.c as if it were a C function - not covered by that interface.
void            trapinithart(void);
void            usertrapret(struct trapframe *);

// uart.c
void            uartinit(int);
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

// gicv2.c
void            gicv2init(void);
void            gicv2inithart(void);
uint32          gic_iar(void);
int             gic_iar_irq(uint32);
void            gic_eoi(uint32);

// timer.c
void            timerinit(void);
void            timerintr(void);

// gpio.c
void            set_pinmode(int pin, enum pinmode mode);
void            gpio_clr(int pin);
void            gpio_set(int pin);

#include "interface_trap.h"
#include "interface_syscall.h"
