// System call numbers.
//
// claude: shared by eleven of the fourteen forks (amd64, arm, arm64,
// arm64-pi4, arm-pi1, arm-pi1-bis, arm-pi2, i386, loongarch, mips,
// riscv32) - already byte-identical across all eleven (forks/loongarch's
// own copy differed only by a missing trailing newline). Used by both
// kernel code (the syscall dispatch table) and user-side code (every
// fork's own generated ulib/usys.S, which usys.pl emits an
// "#include "kernel/syscall.h"" into), so it lives under include/kernel/
// - the "used by both kernel and user, but it's fundamentally a
// kernel-ABI concept" bucket - rather than include/core/ (raw portable C
// types, independent of xv6 itself) or a kernel-only location. Each of
// the eleven forks' own kernel/syscall.h is gone; their existing
// #include "syscall.h" and #include "kernel/syscall.h" lines (both
// spellings appear across the eleven, sometimes within the same fork)
// resolve here unchanged via each fork's own Makefile, which already
// carries an -I../../include for the equivalent core/types.h and
// user/user.h wrappers and now also carries -I../../include/kernel for
// the (more common) bare spelling.
//
// forks/arm-pi3, forks/amd64-jserv and forks/riscv64 each keep their own
// copy - real per-fork differences (arm-pi3 adds SYS_time, amd64-jserv
// adds SYS_chmod, riscv64 renames SYS_sleep to SYS_pause and adds
// SYS_sync), not yet reconciled.
#define SYS_fork    1
#define SYS_exit    2
#define SYS_wait    3
#define SYS_pipe    4
#define SYS_read    5
#define SYS_kill    6
#define SYS_exec    7
#define SYS_fstat   8
#define SYS_chdir   9
#define SYS_dup    10
#define SYS_getpid 11
#define SYS_sbrk   12
#define SYS_sleep  13
#define SYS_uptime 14
#define SYS_open   15
#define SYS_write  16
#define SYS_mknod  17
#define SYS_unlink 18
#define SYS_link   19
#define SYS_mkdir  20
#define SYS_close  21
