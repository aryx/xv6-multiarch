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
  short ownerid; // The owner of the file
  short groupid; // The group owner of the file
  uint mode;     // The permissions mode of the file
  uint size;   // Size of file in bytes
};
