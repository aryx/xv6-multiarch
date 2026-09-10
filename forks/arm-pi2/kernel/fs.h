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

// claude: kept in lockstep with ../uprogs/fs.h - a SEPARATE, duplicate
// copy of this file (uprogs/mkfs.c's own "#include "fs.h"" is
// unqualified, so it picks up that local copy, not this one) - update
// both together if this ever changes again.
//
// NDIRECT was 12 (this fork's original value) - too small for
// a modern-toolchain build: MAXFILE*BSIZE (the largest file this
// filesystem can hold) worked out to 71680 bytes, and this fork's own
// compiled usertests binary is 72156 bytes (needs -marm - see
// uprogs/Makefile's own CFLAGS comment - and ARM's fixed 4-byte
// instruction encoding is less compact than the Thumb-2 code this
// toolchain would otherwise default to), so mkfs's own iappend() hit
// "assert(fbn < MAXFILE)" packing it into fs.img. Same root cause and
// same fix shape as forks/amd64-jserv's own fs.h (see that file's own
// long comment for the full reasoning) - NDIRECT can't be just any
// larger value: mkfs.c's own "assert((512 % sizeof(struct dinode)) ==
// 0)" requires dinode's own size (12 fixed bytes + 4*(NDIRECT+1) for
// addrs[]) to divide BSIZE(512) evenly. NDIRECT=28 (dinode=128 bytes)
// technically clears 72156 bytes (MAXFILE*BSIZE=79872) but with under
// 11% headroom; NDIRECT=60 (dinode=256 bytes, IPB=512/256=2) gives
// MAXFILE*BSIZE=96256 - comfortable margin, same dinode size
// amd64-jserv's own fix landed on.
#define NDIRECT 60
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEVICE only)
  short minor;          // Minor device number (T_DEVICE only)
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

