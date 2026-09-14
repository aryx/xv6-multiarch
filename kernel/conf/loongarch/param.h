#define NPROC        32  // maximum number of processes
#define NCPU          1  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
// claude: was 1000 - the unified tools/mkfs.c's own freeblock-margin
// check (promoted from tools/mkfs-margincheck.c, which only ever ran
// for forks/arm-pi2/forks/arm-pi3) failed the build here: only 440
// data blocks were actually free after packing this image, well under
// the 628 (2*MAXFILE, at this fork's own 1024-byte BSIZE) it now
// requires. This fork always ran this thin - nothing enforced it
// before. Bumped to 1500 for real headroom, not just enough to clear
// the check.
#define FSSIZE       1500  // size of file system in blocks
#define MAXPATH      128   // maximum file path name
