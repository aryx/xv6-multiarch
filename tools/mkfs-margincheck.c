// mkfs: host tool that builds an initial xv6 file system image.
//
// claude: shared by forks/arm-pi2 and forks/arm-pi3 - their own copies
// differed only in comment wording (forks/arm-pi3's comment already named
// this pairing as a factorization candidate). Since include/kernel/fs.h
// unified the on-disk superblock (magic, stored logstart/inodestart/
// bmapstart) across every fork, this file now writes the same fields
// tools/mkfs.c does; what still keeps it a separate file is its own
// disk-budget algorithm - `nblocks` here is a deliberately inflated
// 1285 (not the minimal value the budget would allow), because this
// fork already hit a real bug from cutting it close - see the
// freeblock-margin check below, added after growth in the shared userland
// library silently ate into the disk budget and the kernel's own balloc()
// ran out mid-usertests ("iderw: sector out of range"), several minutes
// into an unrelated QEMU run, instead of failing at build time where the
// problem actually is. That margin check is what keeps this file separate
// from tools/mkfs-fixedbudget.c's four forks rather than folded into it.
//
// Every other fork keeps its own copy - see tools/mkfs.c's header comment
// for the fork-by-fork breakdown of the other on-disk formats in this repo.

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

#define _static_assert(a, b) do { switch (0) case 0: case (a): ; } while (0)

// claude: nblocks was 985 (size 1099) until the factorization session
// that moved every utility program (cat/echo/grep/ls/sh/wc/...) onto
// shared, more general utilities/*.c + lib_core/libc/ulib/*.c
// implementations - every single one of them grew by a near-identical
// ~4.3KB (a few hundred bytes of new library code statically linked
// into each), which alone ate into this budget enough that "big files
// test" (needs MAXFILE=188 blocks for its one test file) started
// missing by a handful of blocks. That does not fail cleanly: mkfs
// itself still had plenty of *disk* room (the assert below still
// held), but the kernel's own balloc() ran out mid-test and handed
// back a block number past the compiled-in disksize, which panics
// ("iderw: sector out of range") deep in a QEMU run instead of
// erroring at build time - see the freeblock-margin check after the
// packing loop below, added so the NEXT such regression fails loudly
// right here instead. nblocks bumped to 1285 (size 1399) for real
// headroom, not just enough to limp past today's specific shortfall.
int nblocks = 1285;
int nlog = LOGSIZE;
int ninodes = 200;
int size = 1399;

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


  _static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

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

  sb.magic = xint(FSMAGIC);
  sb.size = xint(size);
  sb.nblocks = xint(nblocks); // so whole disk is size sectors
  sb.ninodes = xint(ninodes);
  sb.nlog = xint(nlog);
  sb.inodestart = xint(2);
  sb.bmapstart = xint(ninodes / IPB + 3);
  sb.logstart = xint(size - nlog);

  bitblocks = size/(512*8) + 1;
  usedblocks = ninodes / IPB + 3 + bitblocks;
  freeblock = usedblocks;
  uint usedblocks_before_files = usedblocks; // claude: see the margin check below

  printf("used %d (bit %d ninode %zu) free %u log %u total %d\n", usedblocks,
         bitblocks, ninodes/IPB + 1, freeblock, nlog, nblocks+usedblocks+nlog);

  assert(nblocks + usedblocks + nlog == size);

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

  // claude: fail the BUILD, loudly and here, rather than let a future
  // growth spurt in any packed file (every utility here now statically
  // links the same shared, growing lib_core/libc/ulib/*.c) silently eat
  // into the margin usertests' own "big files test" needs at runtime.
  // Missing this does not fail cleanly: mkfs has plenty of *disk* room
  // left (nblocks is comfortably larger than what got packed), so the
  // assert two lines up still passes - it is the KERNEL's balloc() that
  // runs out mid-test and hands back a block number past the compiled-
  // in disksize, which panics ("iderw: sector out of range") minutes
  // into an unrelated QEMU test run instead of erroring right here,
  // at the one point that actually knows both numbers. Margin is
  // 2*MAXFILE: enough room, after everything else this image carries,
  // to write the single biggest file this filesystem format supports
  // and still have as much again to spare.
  {
    uint blocks_used_by_files = usedblocks - usedblocks_before_files;
    uint free_after_packing = nblocks - blocks_used_by_files;
    if (free_after_packing < 2 * MAXFILE) {
      fprintf(stderr,
              "mkfs: only %u data blocks free after packing this image "
              "(nblocks=%d, %u used by the %d packed files) - need at "
              "least %d (2*MAXFILE) so the largest file this filesystem "
              "supports still fits at runtime. Bump nblocks (and size to "
              "match) in tools/mkfs-margincheck.c.\n",
              free_after_packing, nblocks, blocks_used_by_files, argc - 2,
              2 * MAXFILE);
      exit(1);
    }
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
