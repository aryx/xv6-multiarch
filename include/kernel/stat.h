// File-type constants and the struct stat that fstat()/stat() fill in.
//
// claude: shared by seven of the fourteen forks (amd64, arm-pi1,
// arm-pi1-bis, arm-pi2, arm-pi3, i386, mips) - already byte-identical
// across all seven, including the T_DEV -> T_DEVICE rename an earlier
// session made identically to all seven (to match mit-pdos/xv6-riscv, so
// the shared utilities/ls.c compiles against it). Used by both kernel
// code (fs.c, sysfile.c) and user-side code (any program that stat()s a
// file - e.g. utilities/ls.c), so it lives under include/kernel/ - same
// placement rule as include/kernel/syscall.h and include/kernel/fcntl.h.
//
// forks/arm64, forks/arm64-pi4, forks/loongarch and forks/riscv32 share a
// second, distinct copy of their own (also byte-identical across all
// four); forks/amd64-jserv, forks/arm and forks/riscv64 each keep their
// own - not yet reconciled.
#define T_DIR  1   // Directory
#define T_FILE 2   // File
// claude: was T_DEV. Renamed to match mit-pdos/xv6-riscv so the shared
// utilities/ls.c compiles here too - same value 3, pure rename.
#define T_DEVICE  3   // Device

struct stat {
  short type;  // Type of file
  int dev;     // File system's disk device
  uint ino;    // Inode number
  short nlink; // Number of links to file
  uint size;   // Size of file in bytes
};
