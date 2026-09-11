// mkfs: host tool that builds an initial xv6 file system image.
//
// claude: shared by forks/arm, forks/arm-pi1, forks/arm-pi1-bis and
// forks/mips. Since include/kernel/fs.h unified the on-disk superblock
// (magic, stored logstart/inodestart/bmapstart) across every fork, this
// file now writes the same fields tools/mkfs.c does; what still keeps
// it a separate file is its own hardcoded-disk-budget algorithm below,
// not the on-disk format. Their mkfs.c copies differed only in two
// numbers that both turn out to be formulas already implicit in every
// copy, not real per-fork constants:
//
//   - `nblocks`: three of the four hardcoded 985; forks/mips wrote the same
//     value as `995 - LOGSIZE`, which is the general form (forks/arm,
//     forks/arm-pi1 and forks/arm-pi1-bis all have LOGSIZE 10, so
//     995-10=985; forks/mips has LOGSIZE (MAXOPBLOCKS*3)=30, so 965). Took
//     mips's formula.
//   - `size`: hardcoded per fork (1024 for forks/arm's NDIRECT-12 fs.h,
//     1099 for the other three's NDIRECT-60 fs.h - a bigger NDIRECT means a
//     bigger struct dinode, fewer inodes per block, more inode blocks) and
//     asserted to equal nblocks+usedblocks+nlog after the fact. usedblocks
//     already depends on IPB, which already comes from each fork's own
//     fs.h - so computing `size` directly from it, instead of hardcoding a
//     value by hand and asserting it back, produces the exact same 1024 and
//     1099 automatically and needs no per-fork literal at all.
//
// forks/arm-pi1's own copy also renamed the static_assert() helper to
// _static_assert() to dodge a redefinition under a C11 host <assert.h> -
// the same problem the #ifndef guard below already solves; folded back to
// the shared name.
//
// Every other fork keeps its own copy - see tools/mkfs.c's header comment
// for the fork-by-fork breakdown of the other on-disk formats in this repo.
// forks/amd64-jserv and forks/arm-pi2/forks/arm-pi3 share the same
// on-disk format now but compute nblocks/nmeta the other, dynamic way
// (like tools/mkfs.c), not this hardcoded-budget way.

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

// claude: guarded - modern glibc's own <assert.h> (already #included
// above) now provides a "static_assert" macro too (aliasing C11's
// _Static_assert, pulled in whenever gcc's default -std= is C11 or
// later) - this file's own hand-rolled version predates that and is
// functionally equivalent, so skip redefining it rather than erroring
// under -Werror.
#ifndef static_assert
#define static_assert(a, b) do { switch (0) case 0: case (a): ; } while (0)
#endif

// claude: general form of what all four forks already computed by hand -
// see this file's own header comment.
int nblocks = 995 - LOGSIZE;
int nlog = LOGSIZE;
int ninodes = 200;
int size;  // computed in main(), once usedblocks is known - see header comment

int fsfd;
struct superblock sb;
char zeroes[512];
uint freeblock;
uint usedblocks;
uint bitblocks;
uint freeinode = 1;

void balloc(int);
void wsect(uint, void*);
void winode(uint, struct dinode*);
void rinode(uint inum, struct dinode *ip);
void rsect(uint sec, void *buf);
uint ialloc(ushort type);
void iappend(uint inum, void *p, int n);

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
  char buf[512];
  struct dinode din;


  static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

  if(argc < 2){
    fprintf(stderr, "Usage: mkfs fs.img files...\n");
    exit(1);
  }

  assert((512 % sizeof(struct dinode)) == 0);
  assert((512 % sizeof(struct dirent)) == 0);

  fsfd = open(argv[1], O_RDWR|O_CREAT|O_TRUNC, 0666);
  if(fsfd < 0){
    perror(argv[1]);
    exit(1);
  }

  // claude: bitblocks depends on `size` and `size` depends on usedblocks,
  // which depends on bitblocks - true circularity, but this filesystem
  // never gets close to the 4096-block threshold where bitblocks would
  // need to be more than 1 (every fork here sits around 1000-1100 blocks
  // total), so seed it at 1 and assert that assumption instead of solving
  // the circularity properly.
  bitblocks = 1;
  usedblocks = ninodes / IPB + 3 + bitblocks;
  size = nblocks + usedblocks + nlog;
  assert(bitblocks == (uint)(size/(512*8) + 1));

  sb.magic = xint(FSMAGIC);
  sb.size = xint(size);
  sb.nblocks = xint(nblocks); // so whole disk is size sectors
  sb.ninodes = xint(ninodes);
  sb.nlog = xint(nlog);
  sb.inodestart = xint(2);
  sb.bmapstart = xint(ninodes / IPB + 3);
  sb.logstart = xint(size - nlog);

  freeblock = usedblocks;

  printf("used %d (bit %d ninode %zu) free %u log %u total %d\n", usedblocks,
         bitblocks, ninodes/IPB + 1, freeblock, nlog, nblocks+usedblocks+nlog);

  for(i = 0; i < nblocks + usedblocks + nlog; i++)
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

  for(i = 2; i < argc; i++){
    assert(index(argv[i], '/') == 0);

    if((fd = open(argv[i], 0)) < 0){
      perror(argv[i]);
      exit(1);
    }

    // Skip leading _ in name when writing to file system.
    // The binaries are named _rm, _cat, etc. to keep the
    // build operating system from trying to execute them
    // in place of system binaries like rm and cat.
    if(argv[i][0] == '_')
      ++argv[i];

    inum = ialloc(T_FILE);

    bzero(&de, sizeof(de));
    de.inum = xshort(inum);
    strncpy(de.name, argv[i], DIRSIZ);
    iappend(rootino, &de, sizeof(de));

    while((cc = read(fd, buf, sizeof(buf))) > 0)
      iappend(inum, buf, cc);

    close(fd);
  }

  // fix size of root inode dir
  rinode(rootino, &din);
  off = xint(din.size);
  off = ((off/BSIZE) + 1) * BSIZE;
  din.size = xint(off);
  winode(rootino, &din);

  balloc(usedblocks);

  exit(0);
}

void
wsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * 512L, 0) != sec * 512L){
    perror("lseek");
    exit(1);
  }
  if(write(fsfd, buf, 512) != 512){
    perror("write");
    exit(1);
  }
}

uint
i2b(uint inum)
{
  return (inum / IPB) + xint(sb.inodestart);
}

void
winode(uint inum, struct dinode *ip)
{
  char buf[512];
  uint bn;
  struct dinode *dip;

  bn = i2b(inum);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *dip = *ip;
  wsect(bn, buf);
}

void
rinode(uint inum, struct dinode *ip)
{
  char buf[512];
  uint bn;
  struct dinode *dip;

  bn = i2b(inum);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *ip = *dip;
}

void
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * 512L, 0) != sec * 512L){
    perror("lseek");
    exit(1);
  }
  if(read(fsfd, buf, 512) != 512){
    perror("read");
    exit(1);
  }
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
  uchar buf[512];
  int i;

  printf("balloc: first %d blocks have been allocated\n", used);
  assert(used < 512*8);
  bzero(buf, 512);
  for(i = 0; i < used; i++){
    buf[i/8] = buf[i/8] | (0x1 << (i%8));
  }
  printf("balloc: write bitmap block at sector %u\n", xint(sb.bmapstart));
  wsect(xint(sb.bmapstart), buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void
iappend(uint inum, void *xp, int n)
{
  char *p = (char*)xp;
  uint fbn, off, n1;
  struct dinode din;
  char buf[512];
  uint indirect[NINDIRECT];
  uint x;

  rinode(inum, &din);

  off = xint(din.size);
  while(n > 0){
    fbn = off / 512;
    assert(fbn < MAXFILE);
    if(fbn < NDIRECT){
      if(xint(din.addrs[fbn]) == 0){
        din.addrs[fbn] = xint(freeblock++);
        usedblocks++;
      }
      x = xint(din.addrs[fbn]);
    } else {
      if(xint(din.addrs[NDIRECT]) == 0){
        // printf("allocate indirect block\n");
        din.addrs[NDIRECT] = xint(freeblock++);
        usedblocks++;
      }
      // printf("read indirect block\n");
      rsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      if(indirect[fbn - NDIRECT] == 0){
        indirect[fbn - NDIRECT] = xint(freeblock++);
        usedblocks++;
        wsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      }
      x = xint(indirect[fbn-NDIRECT]);
    }
    n1 = min(n, (fbn + 1) * 512 - off);
    rsect(x, buf);
    bcopy(p, buf + off - (fbn * 512), n1);
    wsect(x, buf);
    n -= n1;
    off += n1;
    p += n1;
  }
  din.size = xint(off);
  winode(inum, &din);
}
