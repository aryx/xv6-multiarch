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
// claude: was 1000 - too small once include/kernel/fs.h's own NDIRECT
// grew to 58 (universal dinode format, see that file's own comment):
// IPB dropped from 8 to 2, so the inode region alone nearly quadrupled
// (25 -> 101 blocks), and a real run of usertests hit "balloc: out of
// blocks" during "big files test" at the old FSSIZE. Doubled to 2000,
// matching forks/amd64-jserv/kernel/param.h's own fix for the identical
// shortfall (its dinode grew the same way, earlier), and
// forks/amd64/kernel/param.h alongside it.
#define FSSIZE       2000  // size of file system in blocks

