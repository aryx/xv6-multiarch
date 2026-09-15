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
//     have. Three independent macro axes so far, each a genuinely
//     separate design question - don't assume one family boundary
//     applies to all of them:
//     - LEGACY: the old exit(void)/wait(void)/argstr(int,char**)/
//       consoleintr(int(*)(void)) API vs. the modern exit(int)/
//       wait(uintp)/argstr(int,char*,int)/consoleintr(int) one.
//     - OLD_FILEIO: the pre-copyin/copyout, direct-char*-pointer file/
//       pipe/inode read-write API vs. the address-based one - a
//       different split from LEGACY above (a fork can be LEGACY for
//       the syscall API but already on the modern file I/O one, since
//       that lives in a separately-shared file.c).
//     - FILEIO32: the shared kernel/files/file.c hardcodes a literal
//       "uint64" address parameter for the file I/O functions above,
//       independent of a given fork's own native pointer width (real,
//       if wasteful - not fixable by using "uintp" instead, since the
//       shared C file itself is what hardcodes the type). A fork with
//       its own genuinely 32-bit, non-shared file.c needs uint32
//       instead - #define FILEIO32 before including this file.
// Deliberately NOT centralized here: anything with more than two real
// variants for a given name, or a single-fork exception layered on top
// of one of the axes above - those stay in every fork's own
// <arch>_defs.h, including forks that would otherwise share the common
// version, rather than growing a fourth macro axis for one fork.

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

// syscall.c's own fetchint() moved to kernel/syscalls/interface.h

// trap.c
struct trapframe;  // forward decl only - real layout is per-arch (arch_proc.h)
extern uint     ticks;
extern struct spinlock tickslock;

// printk.c / console.c
void            panic(char*) __attribute__((noreturn));

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
