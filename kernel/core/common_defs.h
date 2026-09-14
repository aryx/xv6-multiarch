// claude: the subset of kernel/defs.h that's genuinely identical across
// (nearly) every fork - split out so each fork's own kernel/defs.h only
// has to carry what's actually specific to it (kernel/arch/<arch>/
// <arch>_defs.h). Two kinds of content below:
//   - plain declarations: truly universal, or safe as an unused extra
//     on the few forks that don't implement them (an unreferenced
//     prototype is harmless in C).
//   - "#ifdef" pairs: a real, structural difference that splits cleanly
//     into two variants - each fork's own kernel/defs.h #defines the
//     relevant macro before including this file if it belongs to that
//     variant's family. A pragmatic bridge, not the end state - meant
//     to shrink or disappear as more categories grow a real arch_*.h
//     dispatch interface the way processes/memory/devices already
//     have. Two INDEPENDENT macro axes, not one - found the hard way,
//     via an actual "conflicting types" build error on exactly 4 forks:
//     - LEGACY: exit/wait/argstr/consoleintr's own cprintf/pushcli/
//       argstr(int,char**)-style family (9 forks: amd64, amd64-jserv,
//       i386, mips, arm, arm-pi1, arm-pi1-bis, arm-pi2, arm-pi3).
//     - OLD_FILEIO: the pre-copyin/copyout, direct-char*-pointer file/
//       pipe/inode read-write API - a DIFFERENT 5-fork family (arm,
//       arm-pi1, arm-pi1-bis, arm-pi2, arm-pi3 only). amd64/amd64-jserv/
//       i386/mips are LEGACY for the former axis but already share
//       kernel/files/file.c's modern style for this one - don't assume
//       the two axes' family boundaries coincide.
//     - FILEIO32 (on top of the non-OLD_FILEIO branch above): the
//       shared kernel/files/file.c hardcodes a literal "uint64" address
//       parameter for every one of its 8 forks REGARDLESS of their own
//       native pointer width (confirmed: i386/mips's own "uintp" is
//       "uint", not "uint64", yet their real fileread/etc. still take
//       uint64 - a real, if wasteful, characteristic of that shared
//       file, not fixable by using "uintp" here). riscv32 alone has its
//       own genuinely-32-bit, non-shared file.c and needs uint32
//       instead - #define FILEIO32 before including this file for that
//       one fork only.
// Deliberately NOT centralized here: anything with more than two real
// variants, or a single-fork exception layered on top of one of the
// two splits above (riscv64's own argint/sleep, amd64/i386's own
// iinit, arm-pi3's cache-writeback params on switchuvm/allocuvm/
// copyuvm/inituvm, mips's mappages asid parameter, amd64's
// syscall(sysframe*), and more) - those stay in every fork's own
// <arch>_defs.h, including the forks that would otherwise share the
// common version, rather than growing a third macro axis.

struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct proc;
struct spinlock;
struct sleeplock;
struct stat;
struct superblock;

// bio.c
void            binit(void);
struct buf*     bread(uint, uint);
void            brelse(struct buf*);
void            bwrite(struct buf*);
void            bpin(struct buf*);
void            bunpin(struct buf*);

// console.c
void            consoleinit(void);
#ifdef LEGACY
void            consoleintr(int(*)(void));
#else
void            consoleintr(int);
#endif

// exec.c
int             exec(char*, char**);

// file.c
struct file*    filealloc(void);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
void            fileinit(void);
#ifdef OLD_FILEIO
int             fileread(struct file*, char*, int n);
int             filewrite(struct file*, char*, int n);
#elif defined(FILEIO32)
int             fileread(struct file*, uint32, int n);
int             filewrite(struct file*, uint32, int n);
#else
int             fileread(struct file*, uint64, int n);
int             filewrite(struct file*, uint64, int n);
#endif

// fs.c
void            fsinit(int);
int             dirlink(struct inode*, char*, uint);
struct inode*   dirlookup(struct inode*, char*, uint*);
struct inode*   ialloc(uint, short);
struct inode*   idup(struct inode*);
void            ilock(struct inode*);
void            iput(struct inode*);
void            iunlock(struct inode*);
void            iunlockput(struct inode*);
void            iupdate(struct inode*);
int             namecmp(const char*, const char*);
struct inode*   namei(char*);
struct inode*   nameiparent(char*, char*);
void            stati(struct inode*, struct stat*);
void            itrunc(struct inode*);
#ifdef OLD_FILEIO
int             readi(struct inode*, char*, uint, uint);
int             writei(struct inode*, char*, uint, uint);
#elif defined(FILEIO32)
int             readi(struct inode*, int, uint32, uint, uint);
int             writei(struct inode*, int, uint32, uint, uint);
#else
int             readi(struct inode*, int, uint64, uint, uint);
int             writei(struct inode*, int, uint64, uint, uint);
#endif

// log.c
void            begin_op(void);
void            end_op(void);
void            log_write(struct buf*);

// pipe.c
int             pipealloc(struct file**, struct file**);
void            pipeclose(struct pipe*, int);
#ifdef OLD_FILEIO
int             piperead(struct pipe*, char*, int);
int             pipewrite(struct pipe*, char*, int);
#elif defined(FILEIO32)
int             piperead(struct pipe*, uint32, int);
int             pipewrite(struct pipe*, uint32, int);
#else
int             piperead(struct pipe*, uint64, int);
int             pipewrite(struct pipe*, uint64, int);
#endif

// proc.c
int             fork(void);
int             growproc(int);
int             kill(int);
void            procdump(void);
void            scheduler(void) __attribute__((noreturn));
void            sched(void);
void            userinit(void);
void            wakeup(void*);
void            yield(void);
#ifdef LEGACY
void            exit(void);
int             wait(void);
#else
void            exit(int);
int             wait(uintp);
#endif

// spinlock.c
void            acquire(struct spinlock*);
int             holding(struct spinlock*);
void            initlock(struct spinlock*, char*);
void            release(struct spinlock*);

// sleeplock.c
void            acquiresleep(struct sleeplock*);
void            releasesleep(struct sleeplock*);
int             holdingsleep(struct sleeplock*);
void            initsleeplock(struct sleeplock*, char*);

// string.c
int             memcmp(const void*, const void*, uint);
void*           memmove(void*, const void*, uint);
void*           memset(void*, int, uint);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char*           strncpy(char*, const char*, int);

// syscall.c
int             fetchint(uintp, int*);

// trap.c
extern uint     ticks;
extern struct spinlock tickslock;

// printk.c / console.c
void            panic(char*) __attribute__((noreturn));

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
