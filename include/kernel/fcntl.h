// open() flags.
//
// claude: shared by eight of the fourteen forks (amd64, amd64-jserv,
// arm-pi1, arm-pi1-bis, arm-pi2, arm-pi3, i386, mips) - already
// byte-identical across all eight, including the O_TRUNC addition (an
// earlier session's own fix, kept here unchanged). Used by both kernel
// code (kernel/sysfile.c's own sys_open()) and user-side code (utilities/
// programs that open() files directly), so it lives under include/kernel/
// - same placement rule as include/kernel/syscall.h.
//
// forks/arm64 and forks/arm64-pi4 share a second, distinct copy of their
// own (also byte-identical to each other); forks/arm, forks/loongarch,
// forks/riscv32 and forks/riscv64 each keep their own - not yet
// reconciled.
#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200
// claude: added, with mit-pdos/xv6-riscv's value, so the shared shells/sh.c
// can use '>' redirection semantics. sys_open honours it - see sysfile.c.
#define O_TRUNC   0x400
