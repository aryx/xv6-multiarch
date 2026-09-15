#ifndef ARCH_VM_H
#define ARCH_VM_H

// claude: amd64's own VM-only, arch-specific types - see arm64's own
// arch_vm.h for why this is named arch_vm.h and not vm.h. amd64 keeps
// PGSIZE/PGROUNDUP/etc. in its own mmu.h rather than duplicating them
// here too - only pagetable_t is needed by the shared kernel/pipe.c,
// kernel/file.c and kernel/sysfile.c.

typedef pde_t *pagetable_t;

#include "memory/interface.h"

#endif /* ARCH_VM_H */
