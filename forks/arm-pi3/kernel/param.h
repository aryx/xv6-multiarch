#define NPROC        64  // maximum number of processes
#define KSTACKSIZE 4096  // size of per-process kernel stack
#define NCPU          4  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NBUF         10  // size of disk block cache
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define LOGSIZE      10  // max data sectors in on-disk log
// claude: was tools/mkfs-margincheck.c's own hardcoded nblocks (1285,
// deliberately inflated for real headroom - see notes_arch_arm_pi2.txt)
// plus whatever the inode/bitmap region worked out to - 1399 total,
// unchanged, now that tools/mkfs.c computes nblocks = FSSIZE - nmeta
// instead of the other way around.
#define FSSIZE       1399  // size of file system in blocks

