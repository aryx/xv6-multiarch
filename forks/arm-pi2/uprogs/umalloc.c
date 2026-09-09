#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

void
free(void *ap)
{
  Header *bp, *p;

  bp = (Header*)ap - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}

static Header*
morecore(uint nu)
{
  char *p;
  Header *hp;
  uint nunits = nu;   // claude: the caller's actual request, before the clamp

  if(nu < 4096)
    nu = 4096;

  // claude: round the chunk up to a whole multiple of the request, so the
  // tail of it does not become a permanently stranded fragment.
  //
  // Without this, malloc(10001) - usertests' mem() - asks for 1252 units
  // and gets a 4096-unit chunk. Three allocations fit (3*1252 = 3756) and
  // the remaining 340 units are then too small to ever satisfy another
  // request. They cannot be coalesced either: the next sbrk chunk is
  // separated from this remainder by the three blocks just handed out. So
  // the free list gained one dead 340-unit fragment per 32KB of heap, and
  // every third malloc() walked the whole accumulated list before giving
  // up and calling morecore() again - quadratic in heap size.
  //
  // That was invisible on ports with little RAM and fatal on ports with a
  // lot: forks/arm fixes PHYSTOP at 128MB and mem() passes, while this
  // port sizes memory dynamically from the firmware mailbox and gets
  // ~960MB under QEMU's raspi2b, i.e. ~30000 stranded fragments instead of
  // ~4000. Measured before this change: mem()'s allocation loop alone took
  // 306s and the free loop had not finished at 600s. It was recorded as
  // "hangs for real / memory pressure" in three ports; it was neither -
  // it terminates, just quadratically.
  //
  // 4096 stays the floor, so small allocations are unaffected (4096 is a
  // whole multiple of any nunits that divides it), and a request larger
  // than 4096 already has nu == nunits and is unchanged.
  if(nu % nunits)
    nu += nunits - (nu % nunits);

  p = sbrk(nu * sizeof(Header));
  if(p == (char*)-1)
    return 0;
  hp = (Header*)p;
  hp->s.size = nu;
  free((void*)(hp + 1));
  return freep;
}

void*
malloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    if(p->s.size >= nunits){
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      return (void*)(p + 1);
    }
    if(p == freep)
      if((p = morecore(nunits)) == 0)
        return 0;
  }
}
