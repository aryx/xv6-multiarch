#define NPROC        64  // maximum number of processes
#define KSTACKSIZE 4096  // size of per-process kernel stack
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NBUF         10  // size of disk block cache
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define LOGSIZE      10  // max data sectors in on-disk log
// claude: was implicitly 1099 (tools/mkfs-fixedbudget.c's own hardcoded
// nblocks (995 - LOGSIZE) plus the inode/bitmap region) until the
// unified tools/mkfs.c's freeblock-margin check (promoted from
// tools/mkfs-margincheck.c, which only ever ran for forks/arm-pi2/
// forks/arm-pi3) failed the build here - this fork always ran this
// thin, nothing enforced it before. Bumped to 1300 (not the more
// generous round numbers used elsewhere): confirmed boot-tested at
// this value, and a much bigger jump broke forks/arm's own boot
// outright (see that fork's own param.h) - deliberately conservative
// here until this fork's own headroom is verified the same way.
#define FSSIZE       1300  // size of file system in blocks

