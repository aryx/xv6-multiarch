// claude: loongarch's own kernel-only, arch-specific types - the
// kernel-only counterpart to include/arch/loongarch/arch.h (which is
// for general types a user program could conceivably need too).

// A page table is 512 PTEs, uint64 each, on this 64-bit port.
typedef uint64 *pagetable_t;
typedef uint64 pte_t;
