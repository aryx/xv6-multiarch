#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200
// claude: added, with mit-pdos/xv6-riscv's value. This fork is 2019-era but
// predates O_TRUNC upstream; the shared shells/sh.c needs it for '>'.
#define O_TRUNC  0x400
