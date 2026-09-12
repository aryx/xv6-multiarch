#ifndef ARCH_VM_H
#define ARCH_VM_H

// claude: riscv64's own VM-only, arch-specific types - not general
// enough for include/arch/riscv64/arch.h (which is for types a user
// program could conceivably need too; these are purely kernel-VM-
// internal). See kernel/arch/arm64/arch_vm.h's own comment for why
// this is named arch_vm.h, not vm.h or kernel.h. Duplicated, not
// migrated, from this fork's own kernel/riscv.h (same pte_t/pagetable_t/
// PGSIZE lines exist there too) - kept that way so this header can be
// included by the shared kernel/kalloc.c without pulling in riscv.h's
// own large CSR/register-access API that no shared file needs.

// A page table is 512 PTEs, uint64 each, on this 64-bit port.
typedef uint64 pte_t;
typedef uint64 *pagetable_t;

#define PGSIZE  4096 // bytes per page
#define PGSHIFT 12   // bits of offset within a page

#define PGROUNDUP(sz)  (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE - 1))

#endif /* ARCH_VM_H */
