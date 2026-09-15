#ifndef INTERFACE_PARAM_H
#define INTERFACE_PARAM_H

// claude: documentation only - never #included by any build, same
// spirit as kernel/processes/interface.h and its siblings, but a
// different kind of contract: every fork's own kernel/conf/<arch>/
// param.h defines real per-board resource-budget constants (process
// table size, open-file limits, on-disk filesystem size, CPU count),
// not shared code. The VALUES are deliberately NOT unified - each one
// is its own board's individually boot-verified tuning (see any given
// fork's own "claude:" comments explaining a specific FSSIZE/NCPU
// choice), and forcing them to match has already caused a real kernel
// panic once in this tree (arm-pi2/arm-pi3's disk-budget incident).
// What's genuinely shared is just the NAMES - documented here so a
// reader can see the expected vocabulary in one place before diffing
// 14 files by hand.
//
// Universal (all 14 forks, same name, per-fork value):
//   NPROC        - maximum number of processes
//   NOFILE       - open files per process
//   NFILE        - open files per system
//   NINODE       - maximum number of active i-nodes
//   NDEV         - maximum major device number
//   ROOTDEV      - device number of file system root disk
//   MAXARG       - max exec arguments
//   NBUF         - size of disk block cache
//   LOGSIZE      - max data blocks/sectors in on-disk log
//   FSSIZE       - size of file system in blocks
//   NCPU         - maximum number of CPUs (real per-board core count,
//                  not a design choice - see e.g. NCPU=1 on mips and
//                  loongarch's own single-core boards vs NCPU=8 on
//                  amd64/i386/arm-pi2's own multi-core QEMU configs)
//
// 9 of 14 (everyone except the arm/arm-pi family - amd64, amd64-jserv,
// arm64, arm64-pi4, i386, loongarch, mips, riscv32, riscv64):
//   MAXOPBLOCKS  - max # of blocks any FS op writes
//   KSTACKSIZE   - size of per-process kernel stack
//
// 5 of 14 (the modern family exactly - arm64, arm64-pi4, loongarch,
// riscv32, riscv64):
//   MAXPATH      - maximum file path name length
//
// 1-fork exceptions, not part of any of the above:
//   USERSTACK, LOGBLOCKS  - riscv64 only
//   PARAM_INCLUDE, N_CALLSTK, HZ  - arm only

#endif /* INTERFACE_PARAM_H */
