#ifndef ARCH_VM_H
#define ARCH_VM_H

// claude: arm64's own VM-only, arch-specific types - not general enough
// for include/arch/arm64/arch.h (which is for types a user program
// could conceivably need too; these are purely kernel-VM-internal).
// Named "arch_vm.h", not "vm.h": kernel/vm.h (the portable VM
// interface, not written yet) and kernel/arch/<arch>/vm.h would both
// be reachable from the same fork's own build (both directories are
// on its own -I list), so a bare #include "vm.h" would be genuinely
// ambiguous - resolved by search order, not by intent. "arch_vm.h"
// can't collide with anything portable-named.
// Shared by arm64 and arm64-pi4 - same ISA, different boards, same
// include/arch/arm64 pairing.

// A page table is 512 PTEs, uint64 each, on this 64-bit port.
typedef uint64 *pagetable_t;

#endif /* ARCH_VM_H */
