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

// claude: everything below is a different kind of documentation than
// the arch_vm.h contract above - vm.c has no arch_*.h dispatch family
// of its own (it's Tier-4 material, nothing portable calls into page-
// table internals directly), but the per-fork implementations
// independently converged on the same function names for the same
// roles anyway, same "emergent, not deliberate" story as
// kernel/console/interface_console.h and
// kernel/interrupts/interface_trap.h. Split along this tree's usual
// legacy/modern family boundary, same as trap.c's own.

// --- legacy family (8 forks: amd64, amd64-jserv, arm-pi1, arm-pi1-bis,
//     arm-pi2, arm-pi3, i386, mips) ---
//
// pte_t *walkpgdir(pde_t *pgdir, const void *va, int alloc)
//   Find (or, if alloc, create) the PTE for virtual address `va` in
//   page directory `pgdir`, walking the page-table levels by hand.
//   `static` on every fork that has it - never called outside vm.c.
pte_t *walkpgdir(void *pgdir, const void *va, int alloc);

// int allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
// int deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
//   Grow/shrink a user address space from oldsz to newsz bytes,
//   allocating or freeing physical pages and their mappings as
//   needed. The sbrk()-level primitives.
int allocuvm(void *pgdir, uint oldsz, uint newsz);
int deallocuvm(void *pgdir, uint oldsz, uint newsz);

// void freevm(pde_t *pgdir)
//   Free a page directory and every user page it maps - called when
//   a process exits.
void freevm(void *pgdir);

// pde_t *copyuvm(pde_t *pgdir, uint sz)
//   Duplicate a whole user address space (page directory plus a fresh
//   copy of every mapped page) - the fork() primitive.
void *copyuvm(void *pgdir, uint sz);

// void switchuvm(struct proc *p)
//   Switch the hardware's active page table to process `p`'s own, and
//   load its kernel stack into the task-switch structure (TSS on x86).
//   Present on 10 of 14 forks - the 8-fork legacy family above, plus
//   (see the modern family note below) arm64/arm64-pi4 specifically.
//   arm-pi3's own copy has a real, different signature -
//   switchuvm(struct proc *p, u32 old_sz, int writeback_old) - an
//   explicit cache-writeback step this port's own real ARM hardware
//   needs and QEMU doesn't (see notes_arch_arm_pi3.txt and this
//   file's own switchuvm() cache-flush history); not a stray 4th
//   parameter to add to the common prototype below.
void switchuvm(struct proc *p);

// void inituvm(pde_t *pgdir, char *init, uint sz)
//   Map the very first page of a brand new process's address space
//   and copy initcode's own bytes into it - used once, by
//   userinit().
void inituvm(void *pgdir, char *init, uint sz);

// void loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
//   Load `sz` bytes from inode `ip` at `offset` into the already-
//   mapped user page at `addr` - exec()'s own segment loader.
void loaduvm(void *pgdir, char *addr, void *ip, uint offset, uint sz);

// pde_t *setupkvm(void)
//   Build a fresh page directory containing only the kernel's own
//   mappings (no user mappings yet) - the starting point both
//   userinit() and exec() build on.
void *setupkvm(void);

// void kvmalloc(void)
//   Call setupkvm() for the kernel's own permanent page directory and
//   switch to it - called once from main(), before any process exists.
void kvmalloc(void);

// void clearpteu(pde_t *pgdir, char *uva)
//   Clear the PTE_U bit on the page at `uva`, making it
//   kernel-only - used to create the one-page red zone below a user
//   stack.
void clearpteu(void *pgdir, char *uva);

// --- modern family (5 forks: arm64, arm64-pi4, loongarch, riscv32,
//     riscv64) ---
//
// pte_t *walk(pagetable_t pagetable, uint64 va, int alloc)
//   Same job as walkpgdir() above, renamed and widened to the generic
//   pagetable_t/uintp types this family uses throughout.
pte_t *walk(pagetable_t pagetable, uintp va, int alloc);

// uint64 uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz, int xperm)
// uint64 uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
//   allocuvm()/deallocuvm() renamed, returning the new size (0 on
//   failure) rather than a boolean - and uvmalloc() takes an explicit
//   xperm (extra PTE permission bits, e.g. PTE_X) argument the legacy
//   family's allocuvm() doesn't have.
uintp uvmalloc(pagetable_t pagetable, uintp oldsz, uintp newsz, int xperm);
uintp uvmdealloc(pagetable_t pagetable, uintp oldsz, uintp newsz);

// void uvmfree(pagetable_t pagetable, uint64 sz)
//   freevm() renamed - takes the address space's own size explicitly
//   (to know how much user memory to unmap) rather than inferring it
//   from the page table alone.
void uvmfree(pagetable_t pagetable, uintp sz);

// pagetable_t uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
//   copyuvm() renamed - copies into an already-allocated `new` page
//   table rather than allocating one itself.
int uvmcopy(pagetable_t old, pagetable_t new_pt, uintp sz);

// void uvmclear(pagetable_t pagetable, uint64 va)
//   clearpteu() renamed, same job.
void uvmclear(pagetable_t pagetable, uintp va);

// pagetable_t kvminit(void), void kvminithart(void)
// void kvmmap(pagetable_t kpgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
//   setupkvm()/kvmalloc() split into three: kvminit() builds the
//   kernel's own page table once, kvminithart() loads it into this
//   hart/core's own satp-equivalent register (called on every core,
//   not just once), kvmmap() is the single-mapping primitive the
//   fixed kernel layout is built from. Adopted by only 4 of the 5
//   modern forks - checked, loongarch folds kvminithart's own job
//   into kvminit instead, same exception pattern as
//   interface_trap.h's own trapinit/trapinithart note.
pagetable_t kvminit(void);
void kvminithart(void);
void kvmmap(pagetable_t kpgtbl, uintp va, uintp pa, uintp sz, int perm);

// int mappages(pagetable_t/pde_t *, void *va, uint64 size, uint64 pa, int perm)
//   Map `size` bytes starting at physical address `pa` into `va` in
//   the given page table, `perm` bits included on every PTE created -
//   the one true common primitive under *both* families above (13 of
//   14 forks that have a vm.c at all, legacy and modern alike,
//   whichever name their own walk function uses). mips's own copy
//   takes an extra `char asid` parameter (its own software-managed
//   TLB is tagged by address-space ID, unlike every other fork's
//   hardware page-table walker) - a real, narrower 1-fork exception,
//   not part of the common shape below.
int mappages(pagetable_t pagetable, uintp va, uintp size, uintp pa, int perm);

#endif /* INTERFACE_VM_H */
