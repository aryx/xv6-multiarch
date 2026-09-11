// On-disk file system format.
// Both the kernel and user programs use this header file.
//
// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout; the kernel never recomputes
// logstart/inodestart/bmapstart itself, it just trusts what mkfs wrote.

#include "conf.h"  // fork-local: BSIZE (tied to that fork's own disk driver, not shared)

#define ROOTINO 1  // root i-number

struct superblock {
  uint magic;        // Must be FSMAGIC
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint bmapstart;    // Block number of first free map block
};

#define FSMAGIC 0x10203040

#define NDIRECT 58
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)
#define NLINK_MAX 32767 // nlink is a short; refuse links past its maximum

// On-disk inode structure. ownerid/groupid/mode are read and written
// only by forks/amd64-jserv's own chown()/chmod() - every other fork
// carries them zeroed (mkfs always zero-initializes a fresh dinode) and
// unused, kept here so that one feature doesn't fork the whole format.
struct dinode {
  short type;              // File type
  short major;             // Major device number (T_DEVICE only)
  short minor;             // Minor device number (T_DEVICE only)
  short nlink;             // Number of links to inode in file system
  short ownerid;           // Owning user id (amd64-jserv only)
  short groupid;           // Owning group id (amd64-jserv only)
  uint mode;                // File mode, e.g. 0755 (amd64-jserv only)
  uint size;                // Size of file (bytes)
  uint addrs[NDIRECT+1];    // Data block addresses
};

// Inodes per block.
#define IPB (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb) ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB (BSIZE*8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

// The name field may have DIRSIZ characters and not end in a NUL
// character.
struct dirent {
  ushort inum;
  char name[DIRSIZ] __attribute__((nonstring));
};
