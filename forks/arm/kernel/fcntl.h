#define O_RDONLY        0x000
#define O_WRONLY        0x001
#define O_RDWR          0x002
#define O_CREATE        0x200
// claude: added, with mit-pdos/xv6-riscv's value, so the shared shells/sh.c
// can use '>' redirection semantics. sys_open honours it - see sysfile.c.
#define O_TRUNC   0x400
