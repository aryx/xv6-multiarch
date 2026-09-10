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
    uint nunits = nu;   // claude: the caller's request, before the clamp

    if(nu < 4096)
        nu = 4096;

    // claude: round the chunk up to a whole multiple of the request, so
    // its tail does not become a permanently stranded fragment.
    //
    // If nunits does not divide the chunk, what is left after handing out
    // as many blocks as fit is BY CONSTRUCTION smaller than nunits, so a
    // program allocating that size repeatedly can never reuse it - and it
    // cannot coalesce either, because the next sbrk chunk is separated
    // from the remainder by the blocks just handed out. The free list then
    // gains one dead fragment per chunk and malloc() rescans that growing
    // list before every morecore(): quadratic in heap size.
    //
    // Same fix as forks/arm-pi2 and forks/mips, where it turned usertests'
    // mem() from "not finished in 600s" into ~4 seconds. This port passes
    // mem() either way because PHYSTOP is only 128MB, so the quadratic
    // term stays small - but it is the same latent bug and it is a real
    // part of why this port's usertests run is slow.
    //
    // A no-op wherever it cannot help: any nunits dividing 4096 (every
    // power-of-two size, i.e. most small allocations) leaves nu unchanged,
    // and a request above 4096 already has nu == nunits. The only cost is
    // that an affected chunk grows by less than one block's worth.
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
