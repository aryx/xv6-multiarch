#ifndef ARCH_VM_H
#define ARCH_VM_H

// claude: i386's own VM-only, arch-specific types - see arm64's own
// arch_vm.h for why this is named arch_vm.h and not vm.h. i386 keeps
// PGSIZE/PGROUNDUP/etc. in its own mmu.h rather than duplicating them
// here too - only pagetable_t is needed by the shared kernel/pipe.c,
// kernel/file.c and kernel/sysfile.c.

typedef pde_t *pagetable_t;

#endif /* ARCH_VM_H */
