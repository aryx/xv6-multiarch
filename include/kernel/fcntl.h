// open() flags.
//
// Used by both kernel code (kernel/sysfile.c's own sys_open()) and
// user-side code (utilities/ programs that open() files directly), so
// it lives under include/kernel/.
#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200
// claude: added, with mit-pdos/xv6-riscv's value, so the shared shells/sh.c
// can use '>' redirection semantics. sys_open honours it - see sysfile.c.
#define O_TRUNC   0x400
