#ifndef ARCH_VM_H
#define ARCH_VM_H

// claude: loongarch's own VM-only, arch-specific types - not general
// enough for include/arch/loongarch/arch.h (which is for types a user
// program could conceivably need too; these are purely kernel-VM-
// internal). Named "arch_vm.h", not "vm.h": kernel/vm.h (the portable
// VM interface, not written yet) and kernel/arch/<arch>/vm.h would
// both be reachable from the same fork's own build (both directories
// are on its own -I list), so a bare #include "vm.h" would be
// genuinely ambiguous - resolved by search order, not by intent.
// "arch_vm.h" can't collide with anything portable-named.

// A page table is 512 PTEs, uint64 each, on this 64-bit port.
typedef uint64 *pagetable_t;
typedef uint64 pte_t;

// claude: duplicated from this fork's own kernel/loongarch.h (same
// lines exist there too, for the same "keep the shared file from
// pulling in a whole per-arch register/CSR header" reason as
// pagetable_t/pte_t above) - needed by the shared kernel/sysfile.c.
#define PGSIZE 4096 // bytes per page
#define PGSHIFT 12  // bits of offset within a page

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

#endif /* ARCH_VM_H */
