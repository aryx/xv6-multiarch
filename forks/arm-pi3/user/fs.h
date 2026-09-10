// On-disk file system format. 
// Both the kernel and user programs use this header file.

// Block 0 is unused.
// Block 1 is super block.
// Blocks 2 through sb.ninodes/IPB hold inodes.
// Then free bitmap blocks holding sb.size bits.
// Then sb.nblocks data blocks.
// Then sb.nlog log blocks.

#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size

// File system super block
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
};

// claude: kept in lockstep with ../include/fs.h - a SEPARATE, duplicate copy of this
// file (uprogs/mkfs.c's own "#include "fs.h"" is unqualified, so it picks up
// that local copy, not the kernel's) - update both together, since they
// define the SAME on-disk layout and mkfs writing one format while the
// kernel reads another corrupts every file.
//
// NDIRECT was 12 (this fork's original value) - too small for a
// modern-toolchain build: MAXFILE*BSIZE, the largest file this filesystem
// can hold, worked out to 71680 bytes and this fork's own compiled
// usertests binary is 74512, so mkfs's iappend() aborted on
// "assert(fbn < MAXFILE)" while packing it into fs.img. This only surfaced
// now because uprogs/ had never been built here at all - source/fs.img was
// a committed blob predating the current toolchain.
//
// NDIRECT cannot be just any larger value: mkfs.c's own
// "assert((512 % sizeof(struct dinode)) == 0)" requires dinode's size
// (12 fixed bytes + 4*(NDIRECT+1) for addrs[]) to divide BSIZE(512)
// evenly. NDIRECT=60 gives a 256-byte dinode and MAXFILE*BSIZE=96256 -
// comfortable margin, and the same value the sibling forks/arm-pi2 and
// forks/amd64-jserv already landed on for this identical bug.
#define NDIRECT 60
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i)     ((i) / IPB + 2)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block containing bit for block b
#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

