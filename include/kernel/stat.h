// File-type constants and the struct stat that fstat()/stat() fill in.
//
// Used by both kernel code (fs.c, sysfile.c) and user-side code (any
// program that stat()s a file - e.g. utilities/ls.c), so it lives under
// include/kernel/.
//
// claude: struct stat is a pure runtime/syscall ABI, never persisted to
// disk, and each fork compiles its own kernel and userland together
// against this same header - so field order here doesn't matter (every
// fork's own stati() in kernel/fs.c populates it by field NAME, never
// positional/memcpy) and `size` is uint64 rather than uint - a lossless
// widening every fork already supports - to hold the largest file size
// any of them can produce, not just the smallest.
#define T_DIR     1   // Directory
#define T_FILE    2   // File
#define T_DEVICE  3   // Device

struct stat {
  int dev;       // File system's disk device
  uint ino;      // Inode number
  short type;    // Type of file
  short nlink;   // Number of links to file
  uint64 size;   // Size of file in bytes
};
