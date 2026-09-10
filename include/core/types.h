// Portable integer typedefs, shared by every port under forks/.
//
// Anything machine-dependent stays in that port's own kernel/types.h, which
// includes this file: uint64 (unsigned long under LP64, unsigned long long
// under ILP32), pde_t and pte_t (pointer-sized by definition), and any
// hardware structs a port happens to keep there.
//
// claude: named core/types.h after ~/goken/include/core/types.h, whose own
// header draws the same line - "those are portable typedefs across
// architectures, the per-arch specific are in include/arch/<arch>/u.h". The
// directory is what keeps the name unambiguous here, since every fork
// already has a types.h of its own; shared code says "core/types.h".
#ifndef CORE_TYPES_H
#define CORE_TYPES_H

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

// Width-named aliases. These definitions hold on every target here - 32-bit
// and 64-bit alike - so they are portable even though not every port used to
// define them.
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;

#endif // CORE_TYPES_H
