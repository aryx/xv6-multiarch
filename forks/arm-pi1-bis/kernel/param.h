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
// thin, nothing enforced it before. Kept deliberately conservative
// (not the more generous round numbers used elsewhere) and boot-
// tested at this exact value: this fork's own kernel.elf, like
// forks/arm's, embeds fs.img directly (see its own top-level
// Makefile's "kernel.elf: ... build/fs.img" rule) - forks/arm hit a
// real, silent boot panic from too large a jump (see its own
// param.h), so this one was raised by less and actually verified
// booting rather than assumed safe from the build succeeding alone.
#define FSSIZE       1250  // size of file system in blocks

