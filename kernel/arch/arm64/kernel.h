// claude: arm64's own kernel-only, arch-specific types - the kernel-
// only counterpart to include/arch/arm64/arch.h (which is for general
// types a user program could conceivably need too). Shared by arm64
// and arm64-pi4 - same ISA, different boards, same include/arch/arm64
// pairing.

// A page table is 512 PTEs, uint64 each, on this 64-bit port.
typedef uint64 *pagetable_t;
