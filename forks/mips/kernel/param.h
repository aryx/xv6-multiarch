#define NPROC        64  // maximum number of processes
#define KSTACKSIZE 4096  // size of per-process kernel stack
#define NCPU          1  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data sectors in on-disk log
// claude: was implicitly 1099 (tools/mkfs-fixedbudget.c's own hardcoded
// nblocks (995 - LOGSIZE) plus the inode/bitmap region) until the
// unified tools/mkfs.c's freeblock-margin check (promoted from
// tools/mkfs-margincheck.c, which only ever ran for forks/arm-pi2/
// forks/arm-pi3) failed the build here - this fork always ran this
// thin, nothing enforced it before. Bumped to 1400 for real headroom -
// safe to be generous here: this fork's normal boot target
// (kernel/kernel + xv6.img) loads the filesystem as a separate QEMU
// disk image, not embedded into the kernel ELF (only the unused
// kernel/kernelmemfs alternate target embeds it - see forks/arm's own
// param.h for why that combination is dangerous).
#define FSSIZE       1400  // size of file system in blocks
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache

