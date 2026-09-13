struct buf {
  int flags;
  uint dev;
  // claude: was named "sector" in this file - an older, literal
  // disk-sector name - before converging on bio.c/bio-x86.c's own
  // "blockno", the modern block-cache term.
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  struct buf *qnext; // disk queue
  uchar data[512];
};
#define B_VALID 0x2  // buffer has been read from disk
#define B_DIRTY 0x4  // buffer needs to be written to disk
