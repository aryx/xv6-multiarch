// Portable integer typedefs, shared by every port under forks/.
//
// Anything machine-word-dependent (uint64, uintp) lives in that port's own
// include/arch/<arch>/arch.h instead - see that directory's own files.
// kernel/types.h itself, which includes both this file and its own arch.h,
// keeps only genuinely kernel-specific typedefs: pde_t, pte_t, and any
// hardware structs a port happens to define there.
//
// claude: named core/types.h after ~/goken/include/core/types.h, whose own
// header draws the same line - "those are portable typedefs across
// architectures, the per-arch specific are in include/arch/<arch>/u.h".
// This repo spells the per-arch file "arch.h", not "u.h" - less cryptic,
// matching core/ and kernel/'s own full-word naming. The directory is what
// keeps the name unambiguous here, since every fork already has a types.h
// of its own; shared code says "core/types.h" and "arch/<arch>/arch.h".
//
// Both are included with angle brackets - #include <core/types.h> and
// #include <arch.h> (no directory prefix on the latter: each fork's own
// Makefile adds exactly one "-I.../include/arch/<arch>" so the same bare
// #include <arch.h> resolves to the right one per build, the same way
// Plan9's own <u.h> works) - signaling "found via the include search
// path", not "next to the file that includes it".
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
