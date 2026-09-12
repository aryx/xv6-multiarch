// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "arch_vm.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// claude: two init-entry shapes coexist here, one per fork's own boot
// sequence (Tier 4 territory, not merged): kinit() (one call over the
// whole range - riscv64, riscv32) and kinit1()/kinit2() (two calls, as
// more physical memory becomes mapped by the kernel's own page tables -
// arm64, arm64-pi4). Each fork's main.c calls only the one(s) it needs;
// the other(s) still compile but go unused, which is harmless (plain
// extern-linkage functions, not static, so no unused-function warning).
void
kinit(void)
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void *)P2V(PHYSTOP));
}

void
kinit1(void *pa_start, void *pa_end)
{
  initlock(&kmem.lock, "kmem");
  freerange(pa_start, pa_end);
}

void
kinit2(void *pa_start, void *pa_end)
{
  freerange(pa_start, pa_end);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uintp)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if (((uintp)pa % PGSIZE) != 0 || (char *)pa < end ||
      (uintp)pa >= (uintp)P2V(PHYSTOP))
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}
