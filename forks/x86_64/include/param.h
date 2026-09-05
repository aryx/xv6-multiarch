#define NPROC        64  // maximum number of processes
#define KSTACKSIZE 4096  // size of per-process kernel stack
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
// claude: was 1000 (this fork's original i386-era value, still what
// forks/x86 uses) - too small once include/fs.h's own NDIRECT grew to
// 58 (see that file's own comment): mkfs itself reported only 118
// blocks free after laying down the initial /README + UPROGS files
// (nmeta grew to 104 blocks - mostly the inode region, now IPB=2
// instead of the original 8, needing 100 inode blocks instead of 25),
// and usertests.c's own writetest1() ("big files test") alone writes a
// single MAXFILE(186)-block file - already more than was free. Doubled
// to 2000, matching forks/riscv/kernel/param.h's own FSSIZE (which
// solved the identical kind of shortfall for the same underlying
// reason: a bigger MAXFILE/inode layout than the original i386 fork's
// FSSIZE=1000 was ever sized for).
#define FSSIZE       2000  // size of file system in blocks

