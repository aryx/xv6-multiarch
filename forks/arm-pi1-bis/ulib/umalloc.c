// claude: LOCAL OVERRIDE of the shared lib_core/libc/ulib/umalloc.c -
// identical except for morecore()'s rounding arithmetic. This fork's
// toolchain/target (arm-linux-gnueabihf-gcc 13,
// "-mcpu=arm1176jzf-s -marm -mfloat-abi=soft") has no hardware divide
// instruction (ARMv6 without the Thumb-2 extension), so a "%"/"/" here
// compiles to a call into libgcc's __aeabi_uidivmod -> __udivsi3. That
// routine (this cross-toolchain ships only one, non-multilib libgcc.a,
// built for a Thumb-2 baseline) is itself written using genuine
// Thumb-2-only encodings ("it eq", "bcc.w", ...) that plain ARMv6
// silicon does not implement - and, on THIS specific QEMU CPU model
// ("arm1176"), executing one does not cleanly fault as "undefined
// instruction": it corrupts the instruction stream and the process
// takes a wild jump moments later (observed: a Prefetch Abort /
// Section Translation Fault at an address far outside the process's
// actual memory, always in the same ~0x10b1xxx-0x10b2xxx range
// regardless of which program or build hit it - the fingerprint of a
// fixed, repeatable garbage decode, not per-run randomness).
//
// Confirmed by tracing: this fork's own shell crashed on the very
// first malloc() any forked child ever makes (shells/sh.c's
// parsecmd()/execcmd(), building the parsed command tree) - it got as
// far as morecore()'s own entry and died before ever reaching sbrk(),
// i.e. inside the "nu % nunits" rounding computation itself. forks/arm
// (real ARMv7-A + Thumb-2, per its own "-march=armv7-a") and
// arm-pi2/arm-pi3 (cortex-a7, also ARMv7-A) never hit this because
// their CPUs genuinely support the Thumb-2 code libgcc ships - only
// this port's (and arm-pi1's own identical copy's) plain-ARMv6 target
// cannot run it. See notes_arch_arm_pi1_bis.txt.
//
// The fix: avoid the division operator here entirely, using the exact
// "accumulate whole multiples with adds only" logic this repository
// already tried once (584383e, before it got prematurely reverted by
// c24df89 on the assumption that supplying raise() and getting a clean
// LINK meant the runtime call was safe on every port - a real Makefile/
// docker.yml build passing is necessary but not sufficient evidence
// for a CPU instruction actually executing correctly).
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

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

  // claude: grow in whole multiples of the request WITHOUT dividing -
  // see this file's own header comment for why. Accumulating gives the
  // identical result (the smallest whole multiple of nunits that is at
  // least 4096) using only adds.
  if(nunits == 0)
    nunits = 1;
  if(nu < 4096){
    nu = 0;
    while(nu < 4096)
      nu += nunits;
  }

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
