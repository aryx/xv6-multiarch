// mkfs: host tool that builds an initial xv6 file system image.
//
// claude: the single shared mkfs for every fork whose kernel/fs.h uses
// include/kernel/fs.h's now-universal on-disk superblock (magic plus
// stored logstart/inodestart/bmapstart) - twelve of the fourteen forks:
// forks/amd64, forks/i386, forks/arm64, forks/arm64-pi4, forks/loongarch,
// forks/riscv32, forks/arm, forks/arm-pi1, forks/arm-pi1-bis,
// forks/arm-pi2, forks/arm-pi3 and forks/mips. Used to be four separate
// files (tools/mkfs.c, tools/mkfs-nomagic.c, tools/mkfs-margincheck.c,
// tools/mkfs-fixedbudget.c) that differed only in two ways, both gone
// now: whether the superblock had a magic number and stored offsets at
// all (fs.h settled that for everyone), and whether `nblocks` was
// computed top-down from each fork's own FSSIZE (this file's original
// approach) or hardcoded bottom-up with `size` derived from it
// afterwards (the margincheck/fixedbudget forks, because until now they
// had no FSSIZE in their own kernel/param.h to compute top-down from -
// added one to each, using the totals their own hardcoded values already
// produced, so nblocks comes out byte-for-byte identical to before).
//
// The freeblock-margin check below (fail the BUILD if packing this image
// leaves less than 2*MAXFILE data blocks free) used to be
// tools/mkfs-margincheck.c's own feature alone, added after real growth
// in the shared userland library silently ate into forks/arm-pi2's disk
// budget and the kernel's own balloc() ran out mid-usertests instead of
// mkfs failing loudly at the one point that knows both numbers - see
// notes_arch_arm_pi2.txt. Promoted here to run for every fork now that
// they all share this file, since the failure mode it catches isn't
// specific to the forks that first hit it.
//
// Two forks keep their own standalone copy, each for a real reason:
//   - forks/riscv64's kernel/param.h renames LOGSIZE to LOGBLOCKS and
//     redefines it as data blocks only, so its own mkfs.c writes
//     nlog = LOGBLOCKS + 1 - one MORE on-disk log block than every fork
//     sharing this file, which writes nlog = LOGSIZE directly. A
//     deliberate headroom fix, not a spelling difference.
//   - forks/amd64-jserv's own dinode carries real, USED
//     ownerid/groupid/mode fields (every other fork carries them
//     zeroed and unused - see include/kernel/fs.h's own comment), and
//     its own ialloc() assigns default owner/group/mode to every
//     inode it creates. That is real per-fork behavior this shared
//     file has no equivalent of, not a formatting difference.

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#define stat xv6_stat  // avoid clash with host struct stat
#include "types.h"
#include "fs.h"
#include "stat.h"
#include "param.h"

#ifndef static_assert
#define static_assert(a, b) do { switch (0) case 0: case (a): ; } while (0)
#endif

#define NINODES 200

// Disk layout:
// [ boot block | sb block | log | inode blocks | free bit map | data blocks ]

int nbitmap = FSSIZE/(BSIZE*8) + 1;
int ninodeblocks = NINODES / IPB + 1;
int nlog = LOGSIZE;
int nmeta;    // Number of meta blocks (boot, sb, nlog, inode, bitmap)
int nblocks;  // Number of data blocks

int fsfd;
struct superblock sb;
char zeroes[BSIZE];
uint freeinode = 1;
uint freeblock;


void balloc(int);
void wsect(uint, void*);
void winode(uint, struct dinode*);
void rinode(uint inum, struct dinode *ip);
void rsect(uint sec, void *buf);
uint ialloc(ushort type);
void iappend(uint inum, void *p, int n);
void die(const char *);

// convert to intel byte order
ushort
xshort(ushort x)
{
  ushort y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  return y;
}

uint
xint(uint x)
{
  uint y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}

int
main(int argc, char *argv[])
{
  int i, cc, fd;
  uint rootino, inum, off;
  struct dirent de;
  char buf[BSIZE];
  struct dinode din;


  static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

  if(argc < 2){
    fprintf(stderr, "Usage: mkfs fs.img files...\n");
    exit(1);
  }

  assert((BSIZE % sizeof(struct dinode)) == 0);
  assert((BSIZE % sizeof(struct dirent)) == 0);

  fsfd = open(argv[1], O_RDWR|O_CREAT|O_TRUNC, 0666);
  if(fsfd < 0)
    die(argv[1]);

  // 1 fs block = 1 disk sector
  nmeta = 2 + nlog + ninodeblocks + nbitmap;
  nblocks = FSSIZE - nmeta;

  sb.magic = xint(FSMAGIC);
  sb.size = xint(FSSIZE);
  sb.nblocks = xint(nblocks);
  sb.ninodes = xint(NINODES);
  sb.nlog = xint(nlog);
  sb.logstart = xint(2);
  sb.inodestart = xint(2+nlog);
  sb.bmapstart = xint(2+nlog+ninodeblocks);

  printf("nmeta %d (boot, super, log blocks %u inode blocks %u, bitmap blocks %u) blocks %d total %d\n",
         nmeta, nlog, ninodeblocks, nbitmap, nblocks, FSSIZE);

  freeblock = nmeta;     // the first free block that we can allocate

  for(i = 0; i < FSSIZE; i++)
    wsect(i, zeroes);

  memset(buf, 0, sizeof(buf));
  memmove(buf, &sb, sizeof(sb));
  wsect(1, buf);

  rootino = ialloc(T_DIR);
  assert(rootino == ROOTINO);

  bzero(&de, sizeof(de));
  de.inum = xshort(rootino);
  strcpy(de.name, ".");
  iappend(rootino, &de, sizeof(de));

  bzero(&de, sizeof(de));
  de.inum = xshort(rootino);
  strcpy(de.name, "..");
  iappend(rootino, &de, sizeof(de));

  uint freeblock_before_files = freeblock; // claude: see the margin check below

  for(i = 2; i < argc; i++){
  // claude: was "get rid of 'user/'" - a hardcoded strncmp against
  // that one literal prefix. Phase 0 splits the user programs across
  // user/ and tests/, so that would have needed a second special
  // case. Take the basename instead - strip to the last '/', whatever
  // directory the program was built in. See plan_factorization.md.
  char *shortname = strrchr(argv[i], '/');
  shortname = shortname ? shortname + 1 : argv[i];

    if((fd = open(argv[i], 0)) < 0)
      die(argv[i]);

    // Skip leading _ in name when writing to file system.
    // The binaries are named _rm, _cat, etc. to keep the
    // build operating system from trying to execute them
    // in place of system binaries like rm and cat.
    if(shortname[0] == '_')
      shortname += 1;

    inum = ialloc(T_FILE);

    bzero(&de, sizeof(de));
    de.inum = xshort(inum);
    strncpy(de.name, shortname, DIRSIZ);
    iappend(rootino, &de, sizeof(de));

    while((cc = read(fd, buf, sizeof(buf))) > 0)
      iappend(inum, buf, cc);

    close(fd);
  }

  // claude: fail the BUILD, loudly and here, rather than let a future
  // growth spurt in any packed file (every utility here now statically
  // links the same shared, growing lib_core/libc/ulib/*.c) silently eat
  // into the margin usertests' own "big files test" needs at runtime.
  // Missing this does not fail cleanly: mkfs has plenty of *disk* room
  // left (nblocks was sized from FSSIZE, not from what actually got
  // packed), so nothing above catches it - it is the KERNEL's balloc()
  // that runs out mid-test and hands back a block number past the
  // compiled-in disksize, which panics ("iderw: sector out of range" or
  // "balloc: out of blocks") minutes into an unrelated QEMU test run
  // instead of erroring right here, at the one point that actually knows
  // both numbers. Margin is 2*MAXFILE: enough room, after everything
  // else this image carries, to write the single biggest file this
  // filesystem format supports and still have as much again to spare.
  {
    uint blocks_used_by_files = freeblock - freeblock_before_files;
    uint free_after_packing = nblocks - blocks_used_by_files;
    if (free_after_packing < 2 * (uint)MAXFILE) {
      fprintf(stderr,
              "mkfs: only %u data blocks free after packing this image "
              "(nblocks=%d, %u used by the %d packed files) - need at "
              "least %u (2*MAXFILE) so the largest file this filesystem "
              "supports still fits at runtime. Bump FSSIZE in this "
              "fork's own kernel/param.h.\n",
              free_after_packing, nblocks, blocks_used_by_files, argc - 2,
              2 * (uint)MAXFILE);
      exit(1);
    }
  }

  // fix size of root inode dir
  rinode(rootino, &din);
  off = xint(din.size);
  off = ((off/BSIZE) + 1) * BSIZE;
  din.size = xint(off);
  winode(rootino, &din);

  balloc(freeblock);

  exit(0);
}

void
wsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE)
    die("lseek");
  if(write(fsfd, buf, BSIZE) != BSIZE)
    die("write");
}

void
winode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *dip = *ip;
  wsect(bn, buf);
}

void
rinode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *ip = *dip;
}

void
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE)
    die("lseek");
  if(read(fsfd, buf, BSIZE) != BSIZE)
    die("read");
}

uint
ialloc(ushort type)
{
  uint inum = freeinode++;
  struct dinode din;

  bzero(&din, sizeof(din));
  din.type = xshort(type);
  din.nlink = xshort(1);
  din.size = xint(0);
  winode(inum, &din);
  return inum;
}

void
balloc(int used)
{
  uchar buf[BSIZE];
  int i;

  printf("balloc: first %d blocks have been allocated\n", used);
  assert(used < BSIZE*8);
  bzero(buf, BSIZE);
  for(i = 0; i < used; i++){
    buf[i/8] = buf[i/8] | (0x1 << (i%8));
  }
  printf("balloc: write bitmap block at sector %d\n", sb.bmapstart);
  wsect(sb.bmapstart, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void
iappend(uint inum, void *xp, int n)
{
  char *p = (char*)xp;
  uint fbn, off, n1;
  struct dinode din;
  char buf[BSIZE];
  uint indirect[NINDIRECT];
  uint x;

  rinode(inum, &din);
  off = xint(din.size);
  // printf("append inum %d at off %d sz %d\n", inum, off, n);
  while(n > 0){
    fbn = off / BSIZE;
    assert(fbn < MAXFILE);
    if(fbn < NDIRECT){
      if(xint(din.addrs[fbn]) == 0){
        din.addrs[fbn] = xint(freeblock++);
      }
      x = xint(din.addrs[fbn]);
    } else {
      if(xint(din.addrs[NDIRECT]) == 0){
        din.addrs[NDIRECT] = xint(freeblock++);
      }
      rsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      if(indirect[fbn - NDIRECT] == 0){
        indirect[fbn - NDIRECT] = xint(freeblock++);
        wsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      }
      x = xint(indirect[fbn-NDIRECT]);
    }
    n1 = min(n, (fbn + 1) * BSIZE - off);
    rsect(x, buf);
    bcopy(p, buf + off - (fbn * BSIZE), n1);
    wsect(x, buf);
    n -= n1;
    off += n1;
    p += n1;
  }
  din.size = xint(off);
  winode(inum, &din);
}

void
die(const char *s)
{
  perror(s);
  exit(1);
}
