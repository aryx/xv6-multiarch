#ifndef ARCH_VM_H
#define ARCH_VM_H

// claude: riscv32's own VM-only, arch-specific types - not general
// enough for include/arch/riscv32/arch.h (which is for types a user
// program could conceivably need too; these are purely kernel-VM-
// internal). See kernel/arch/arm64/arch_vm.h's own comment for why
// this is named arch_vm.h, not vm.h or kernel.h.

// A page table is 1024 PTEs, uint32 each, on this 32-bit port.
typedef uint32 pte_t;
typedef uint32 *pagetable_t;

// claude: duplicated from this fork's own kernel/riscv.h (same lines
// exist there too, for the same "keep the shared file from pulling in
// a whole per-arch register/CSR header" reason as pte_t/pagetable_t
// above) - needed by the shared kernel/kalloc.c.
#define PGSIZE 4096 // bytes per page
#define PGSHIFT 12  // bits of offset within a page

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

#endif /* ARCH_VM_H */
