#include "arch_vm.h"

// console.c
void            consputc(int);

// file.c
int             filestat(struct file*, uint32 addr);

// fs.c
void            iinit();

// ramdisk.c
void            ramdiskinit(void);
void            ramdiskintr(void);
void            ramdiskrw(struct buf*);

// kalloc.c
void*           kalloc(void);
void            kfree(void *);
void            kinit();

// log.c
void            initlog(int, struct superblock*);

// printf.c
void            printf(char*, ...);
void            printfinit(void);

// proc.c
int             cpuid(void);
pagetable_t     proc_pagetable(struct proc *);
void            proc_freepagetable(pagetable_t, uint32);
struct cpu*     mycpu(void);
struct cpu*     getmycpu(void);
struct proc*    myproc();
void            procinit(void);
void            setproc(struct proc*);
void            sleep(void*, struct spinlock*);
int             either_copyout(int user_dst, uint32 dst, void *src, uint32 len);
int             either_copyin(void *dst, int user_src, uint32 src, uint32 len);

// swtch.S
void            swtch(struct context*, struct context*);

// spinlock.c
void            push_off(void);
void            pop_off(void);

// syscall.c
int             argint(int, int*);
int             argstr(int, char*, int);
int             argaddr(int, uint32 *);
int             fetchstr(uint32, char*, int);
int             fetchaddr(uint32, uint32*);
void            syscall();

// trap.c
void            trapinit(void);
void            trapinithart(void);
void            usertrapret(void);

// uart.c
void            uartinit(void);
void            uartintr(void);
void            uartputc(int);
int             uartgetc(void);

// vm.c
void            kvminit(void);
void            kvminithart(void);
uint32          kvmpa(uint32);
void            kvmmap(uint32, uint32, uint32, int);
int             mappages(pagetable_t, uint32, uint32, uint32, int);
pagetable_t     uvmcreate(void);
void            uvminit(pagetable_t, uchar *, uint);
uint32          uvmalloc(pagetable_t, uint32, uint32);
uint32          uvmdealloc(pagetable_t, uint32, uint32);
int             uvmcopy(pagetable_t, pagetable_t, uint32);
void            uvmfree(pagetable_t, uint32);
void            uvmunmap(pagetable_t, uint32, uint32, int);
void            uvmclear(pagetable_t, uint32);
uint32          walkaddr(pagetable_t, uint32);
int             arch_copyout(pagetable_t, uint32, char *, uint32);
int             arch_copyin(pagetable_t, char *, uint32, uint32);
int             copyinstr(pagetable_t, char *, uint32, uint32);

// plic.c
void            plicinit(void);
void            plicinithart(void);
uint32          plic_pending(void);
int             plic_claim(void);
void            plic_complete(int);

// virtio_disk.c
void            virtio_disk_init(void);
void            virtio_disk_rw(struct buf *, int);
void            virtio_disk_intr();
