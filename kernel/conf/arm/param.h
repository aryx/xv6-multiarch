#ifndef PARAM_INCLUDE
#define PARAM_INCLUDE


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
// claude: was computed by tools/mkfs-fixedbudget.c's own hardcoded
// nblocks (995 - LOGSIZE) plus whatever the inode/bitmap region worked
// out to - 1099 total, unchanged, now that tools/mkfs.c computes
// nblocks = FSSIZE - nmeta instead of the other way around. Already
// clears the unified mkfs.c's own freeblock-margin check (promoted
// from tools/mkfs-margincheck.c) with no bump needed - unlike
// forks/arm-pi1/forks/arm-pi1-bis/forks/mips, don't raise this without
// checking real hardware boot first: this fork's own kernel/start.c
// embeds fs.img directly into the kernel ELF and has a hard, tight
// physical-memory ceiling (`vectbl`, in the first 1MB) that a much
// bigger FSSIZE (1500 was tried) silently blows through - the kernel
// still builds, but panics ("empty mark in the list") in kpt_freerange
// during boot, before any filesystem code runs at all.
#define FSSIZE       1099  // size of file system in blocks

#define HZ           10

#define N_CALLSTK    15
#endif