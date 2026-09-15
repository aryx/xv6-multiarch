
void OkLoop(void);
void NotOkLoop(void);

// mmu.c
void mmuinit1(void);
void barriers(void);
void dsb_barrier(void);
void flush_tlb(void);
void flush_dcache_all(void);
void flush_dcache(uint va1, uint va2);
void flush_idcache(void);
void set_pgtbase(uint base);

// console.c
void            cprintf(char*, ...);
void		drawcharacter(u8, uint, uint);
void		gpuputc(uint);

// fs.c
void            readsb(int dev, struct superblock *sb);
void            iinit(void);

// ide.c
void            ideinit(void);
void            ideintr(void);
void            iderw(struct buf*);

// file.c
int             filestat(struct file*, struct stat*);

// fs.c
void            readsb(int dev, struct superblock *sb);
void            iinit(void);

// kalloc.c
char*           kalloc(void);
void            kfree(char*);
void            kinit1(void*, void*);
void            kinit2(void*, void*);

// log.c
void            initlog(void);
void            begin_trans();
void            commit_trans();

// proc.c
struct proc*    copyproc(struct proc*);
void            pinit(void);
void            sleep(void*, struct spinlock*);

// swtch.S
void            swtch(struct context**, struct context*);

// syscall.c - see kernel/syscalls/interface.h

void kvmalloc(void);

int UsbInitialise(void);
void KeyboardUpdate(void);
char KeyboardGetChar(void);
uint KeyboardCount(void);
uint KeyboardGetAddress(uint);
struct KeyboardLeds KeyboardGetLedSupport(uint);

// spinlock.c
void            getcallerpcs(void*, uint*);
void            pushcli(void);
void            popcli(void);

// string.c
uint 		div(uint n, uint d);

// timer.c
void		timer3init(void);
void		timer3intr(void);
unsigned long long getsystemtime(void);
void		delay(uint);

// trap.c
void            tvinit(void);
void		sti(void);
void		cli(void);
void 		disable_intrs(void);
void 		enable_intrs(void);
uint		readcpsr(void);

// uart.c
void            uartinit(void);
void            miniuartintr(void);
void            uartputc(uint);
void		setgpiofunc(uint, uint);
void		setgpioval(uint, uint);

// vm.c
void            seginit(void);
void            kvmalloc(void);
void            vmenable(void);
pde_t*          setupkvm(void);
char*           uva2ka(pde_t*, char*);
int             allocuvm(pde_t*, uint, uint);
int             deallocuvm(pde_t*, uint, uint);
void            freevm(pde_t*);
void            inituvm(pde_t*, char*, uint);
int             loaduvm(pde_t*, char*, struct inode*, uint, uint);
pde_t*          copyuvm(pde_t*, uint);
void            switchuvm(struct proc*);
void            switchkvm(void);
int             copyout(pde_t*, uint, void*, uint);
void            clearpteu(pde_t *pgdir, char *uva);

// mailbox.c
uint readmailbox(u8);
void writemailbox(uint *, u8);
void create_request(volatile uint *mbuf, uint tag, uint buflen, uint len, uint *data);
void mailboxinit(void);

#include "interrupts/interface.h"
#include "syscalls/interface.h"

#include "console/interface.h"
