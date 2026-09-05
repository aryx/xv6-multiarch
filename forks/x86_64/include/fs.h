#ifndef _FS__H
#define _FS__H

// On-disk file system format. 
// Both the kernel and user programs use this header file.

// Block 0 is unused.
// Block 1 is super block.
// Blocks 2 through sb.ninodes/IPB hold inodes.
// Then free bitmap blocks holding sb.size bits.
// Then sb.nblocks data blocks.
// Then sb.nlog log blocks.

#define ROOTINO 1  // root i-number
// claude: was 512 (this fork's original i386-era value, still what
// forks/x86 uses) - too small for a 64-bit build: MAXFILE*BSIZE (the
// largest file this filesystem can hold) worked out to 70656 bytes,
// and this fork's own compiled usertests binary is 79728 bytes (64-bit
// code is inherently bigger - more/wider register saves, wider
// pointers/immediates throughout, mcmodel=kernel addressing), so
// mkfs's own iappend() hit "assert(fbn < MAXFILE)"
// packing it into fs.img. Doubling BSIZE (not NDIRECT) mirrors exactly
// what MIT's own later xv6-riscv port did for the same reason (see
// forks/riscv/kernel/fs.h's own BSIZE 1024) - it multiplies MAXFILE's
// NINDIRECT term without perturbing struct dinode's on-disk layout at
// all (NDIRECT, hence sizeof(dinode)/IPB, is unchanged), unlike raising
// NDIRECT which would need re-deriving IPB/inode-block-count headroom
// too. Confirmed safe with this fork's own IDE driver: kernel/ide.c's
// idestart() already derives "sector_per_block = BSIZE/SECTOR_SIZE"
// generically (SECTOR_SIZE staying the real hardware constant 512) and
// only panics above sector_per_block > 7 - doubling to 2 has room to
// spare.
#define BSIZE 1024  // block size

// File system super block
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
};

#define NDIRECT 10
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  short ownerid;        // The ID of the user who owns the file.
  short groupid;        // The ID of the group who owns the file.
  uint mode;           // The files mode e.g. 0700
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

#endif /* _FS__H */

