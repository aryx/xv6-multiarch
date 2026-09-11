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


// claude: kept in lockstep with ../kernel/param.h - a SEPARATE, duplicate
// copy of this file (mkfs's own bare #include "param.h" resolves via
// -iquote . to this local copy, not the kernel's - see the Pi ports'
// long-known duplicated-header set in plan_factorization.md). Update
// both together, since mkfs and the kernel must agree on FSSIZE.
#define FSSIZE       1250  // size of file system in blocks
