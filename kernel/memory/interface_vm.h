#ifndef INTERFACE_VM_H
#define INTERFACE_VM_H

// claude: documentation only - never #included by any build. C has no
// way to declare or enforce an interface, so this file exists purely
// so a reader can find, in one place, the contract every fork's own
// kernel/memory/<arch>/arch_vm.h (plus, for the two functions below,
// its own vm.c) is expected to satisfy. See
// docs/claude_notes/notes_new_kernel_organization.md for the
// "interface, not permanent fork" method this documents, and any one
// real arch_vm.h (e.g. kernel/memory/arm64/arch_vm.h) for a worked
// example.
//
// Adopted by 8 of 14 forks so far (amd64-jserv, amd64, arm64, i386,
// loongarch, mips, riscv32, riscv64); the six ARM ports don't have
// this interface yet.

// The only type every arch_vm.h must define - an opaque handle to a
// page table, needed by the shared kernel/pipe.c, kernel/file.c and
// kernel/sysfile.c. Concretely a pointer to this port's own top-level
// page-directory entry type (pde_t/pte_t), whatever width and layout
// that is on this ISA.
typedef void *pagetable_t;

// Needed only by the shared kernel/kalloc.c, and only on forks that
// don't already define these in their own kernel/arch/<arch>/mmu.h
// (amd64/mips keep them there instead and skip this section entirely
// - the true minimum contract is pagetable_t above, nothing else).
// pte_t: one page-table-entry, this port's own native width.
// PGSIZE/PGSHIFT: bytes per page and its log2, matching each other.
// PGROUNDUP/PGROUNDDOWN(a): round a byte address/size to page granularity.
#if 0
typedef /* ... */ pte_t;
#define PGSIZE    /* ... */
#define PGSHIFT   /* ... */
#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))
#endif

// Copy `len` bytes from kernel memory at `src` into `pagetable`'s own
// user address space starting at `dstva`, and the reverse direction
// for arch_copyin (`srcva` in `pagetable`'s user space to kernel `dst`).
// Both walk the page table themselves rather than assuming user memory
// is identity-mapped, since it isn't on most of these ports. Declared
// in each fork's own defs.h (not yet centralized here) and defined in
// its own vm.c, unlike arch_sleep_release/arch_disk_rw which are
// static-inline one-liners living directly in the arch_*.h itself -
// the page-table-walk logic is genuinely too different per ISA to fit
// in a header.
// The address/length type is each port's own native width (uint32 on
// riscv32, uint64 elsewhere - see e.g. riscv32's own defs.h), not
// fixed here.
// Called from: kernel/file.c, kernel/pipe.c, kernel/sysfile.c.
int arch_copyout(pagetable_t pagetable, uintp dstva, char *src, uintp len);
int arch_copyin(pagetable_t pagetable, char *dst, uintp srcva, uintp len);

#endif /* INTERFACE_VM_H */
