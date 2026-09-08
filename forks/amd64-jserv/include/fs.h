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
#define BSIZE 512  // block size

// File system super block
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
};

// claude: NDIRECT was 10 (this fork's original i386-era value, still
// what forks/x86 uses) - too small for a 64-bit build: MAXFILE*BSIZE
// (the largest file this filesystem can hold) worked out to 70656
// bytes, and this fork's own compiled usertests binary is 79728 bytes
// (64-bit code is inherently bigger - more/wider register saves, wider
// pointers/immediates throughout, mcmodel=kernel addressing), so
// mkfs's own iappend() hit "assert(fbn < MAXFILE)" packing it into
// fs.img.
//
// A first attempt fixed this by doubling BSIZE instead (512 -> 1024,
// mirroring forks/riscv/kernel/fs.h's own BSIZE 1024) - that multiplies
// MAXFILE's NINDIRECT term without touching struct dinode's layout at
// all. It built and mkfs succeeded, but broke booting: it silently
// changes kernel/ide.c's own "sector_per_block = BSIZE/SECTOR_SIZE"
// from 1 to 2, and this fork's ideintr() assumes exactly ONE interrupt
// per idestart() request, doing a single insl() of the WHOLE transfer
// - correct when sector_per_block is 1 (BSIZE==SECTOR_SIZE, the config
// every version of this fork has ever actually been tested with), but
// a real ATA PIO "READ SECTORS" (not "READ MULTIPLE") command raises
// one interrupt PER SECTOR, not one for the whole multi-sector
// request. Confirmed via gdb-multiarch (see
// docs/claude_notes/notes_arch_x86_64.txt): ideintr() fired twice per
// idestart() once BSIZE=1024, and the first process's own disk read
// (loading /init) got stuck reading corrupted/incomplete data as a
// result - reproducibly booted further under gdb (whose breakpoint
// pauses happened to give the second sector time to actually become
// ready) than at full untraced speed, a classic race-condition
// signature. Rather than teach ideintr() to handle multi-sector PIO
// transfers correctly (a real fix, but a bigger and riskier change to
// this fork's disk driver), raising NDIRECT instead keeps
// sector_per_block at 1 - the configuration this driver was actually
// built and tested for - and solves the exact same MAXFILE*BSIZE
// shortfall without ever exercising the buggy path at all.
//
// NDIRECT can't be just any value large enough, either: tools/mkfs.c's
// own main() asserts "BSIZE % sizeof(struct dinode) == 0" (inodes must
// pack evenly into a block, no partial inode wasted at the end), and
// since BSIZE(512) = 2^9, sizeof(dinode) - 24 + 4*(NDIRECT+1) bytes -
// must itself be a power of two for that to hold. NDIRECT=26
// (dinode=128 bytes) very nearly works (MAXFILE*BSIZE=78848) but falls
// 880 bytes short of the 79728-byte usertests binary; the next valid
// value up is NDIRECT=58 (dinode=256 bytes, IPB=512/256=2), giving
// MAXFILE*BSIZE=95232 - comfortable margin.
#define NDIRECT 58
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

