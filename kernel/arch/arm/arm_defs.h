//
#ifndef INCLUDE_DEFS_H
#define INCLUDE_DEFS_H

#define UMAX(a, b) ((uint)(a) > (uint)(b) ? (uint)(a):(uint)(b))
#define UMIN(a, b) ((uint)(a) > (uint)(b) ? (uint)(b):(uint)(a))

// number of elements in fixed-size array

struct trapframe;

typedef uint32	pte_t;
typedef uint32  pde_t;
extern  uint32  _kernel_pgtbl;
typedef void (*ISR) (struct trapframe *tf, int n);

// arm.c
void            set_stk(uint mode, uint addr);
void            cli (void);
void            sti (void);
uint            spsr_usr();
int             int_enabled();
void            pushcli(void);
void            popcli(void);
void            getcallerpcs(void *, uint*);
void*           get_fp (void);
void            show_callstk (char *);

// buddy.c
void            kmem_init (void);
void            kmem_init2(void *vstart, void *vend);
void*           kmalloc (int order);
void            kfree (void *mem, int order);
void            free_page(void *v);
void*           alloc_page (void);
void            kmem_test_b (void);
int             get_order (uint32 v);

// console.c
void            cprintf(char*, ...);

// file.c
int             filestat(struct file*, struct stat*);

// fs.c
void            readsb(int dev, struct superblock *sb);
void            iinit(void);

// ide.c
void            ideinit(void);
void            iderw(struct buf*);

// kalloc.c
/*char*           kalloc(void);
void            kfree(char*);
void            kinit1(void*, void*);
void            kinit2(void*, void*);
void            kmem_init (void);*/

// log.c
void            initlog(void);
void            begin_trans();
void            commit_trans();

// picirq.c
void            pic_enable(int, ISR);
void            pic_init(void*);
void            pic_dispatch (struct trapframe *tp);

// proc.c
struct proc*    copyproc(struct proc*);
void            pinit(void);
void            sleep(void*, struct spinlock*);

// swtch.S
void            swtch(struct context**, struct context*);

// syscall.c - see kernel/syscalls/interface_syscall.h

//void            timer_init(int hz);
void            timer_init( void );
void            timer3_init( void );

// trap.c
void            trap_init(void);
void            dump_trapframe (struct trapframe *tf);

// trap_asm.S
void            trap_reset(void);
void            trap_und(void);
void            trap_swi(void);
void            trap_iabort(void);
void            trap_dabort(void);
void            trap_na(void);
void            trap_irq(void);
void            trap_fiq(void);

// uart.c
void            uart_init(void*);
void            uartputc(int);
int             uartgetc(void);
void            micro_delay(int us);
void            uart_enable_rx();

// vm.c
int             allocuvm(pde_t*, uint, uint);
int             deallocuvm(pde_t*, uint, uint);
void            freevm(pde_t*);
void            inituvm(pde_t*, char*, uint);
int             loaduvm(pde_t*, char*, struct inode*, uint, uint);
pde_t*          copyuvm(pde_t*, uint);
void            switchuvm(struct proc*);
int             copyout(pde_t*, uint, void*, uint);
void            clearpteu(pde_t *pgdir, char *uva);
void*           kpt_alloc(void);
void            init_vmm (void);
void            kpt_freerange (uint32 low, uint32 hi);
void            paging_init (uint phy_low, uint phy_hi);

// gic.c
void 		gic_init(void* base);

void _puts(const char* );// raspi tmp
void hexstrings ( unsigned int );
void hexstring ( unsigned int );

void flush_dcache_all(void);// raspi

#endif

#include "syscalls/interface_syscall.h"

#include "console/interface_console.h"
