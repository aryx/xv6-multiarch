# Notes: the emerging header/interface organization

This is a reference note for a convention that grew out of
`docs/claude_notes/plan_factorization.md`'s Tier 3 work (`file.c`,
`pipe.c`, `sleeplock.c`, `kalloc.c`, `log.c`, `sysproc.c`, `bio.c`,
session of 2026-09-12), not
a plan of its own - it exists so the *shape* of the convention is
findable in one place instead of scattered across that file's own
blow-by-blow entries. Read `plan_factorization.md` for the history and
the reasoning behind each individual file; read this for "where does a
new type/constant/function go and why."

**Updated 2026-09-13**: `kernel/arch/<arch>/arch_vm.h`, `arch_proc.h`
and `arch_disk.h` - as named throughout the rest of this file below -
moved out to `kernel/memory/<arch>/`, `kernel/processes/<arch>/` and
`kernel/devices/<arch>/` respectively, joining the rest of their own
subsystem's files (`mmu.h`/`memlayout.h`; `swtch.S`/`procasm.S`/
`regs.h`; `mailbox.h`/`virtio.h`/...). `kernel/arch/<arch>/` itself now
holds only genuine CPU register/CSR definitions (`arm.h`, `x86.h`,
`mips.h`, `riscv.h`, `loongarch.h`, `aarch64.h`, `msr.h`, `cpuid.h`) -
see `plan_factorization.md`'s own "A parallel track" section for the
full `kernel/<category>/<arch>/` layout this is part of. The `-I` flag
mechanics below are otherwise unchanged: each fork's existing
`-I../../kernel/arch/<arch>` flag still resolves the quoted
`#include "arch_vm.h"` etc., now via a symlink left at that same path
pointing at the new location - read every `kernel/arch/<arch>/...`
below as that symlink's target, not its literal current location.

## The layering, top to bottom

```
include/core/types.h          portable typedefs (uint, uint8/16/32) -
                               true on every arch, no exceptions
include/arch/<arch>/arch.h     machine-word-dependent types for THIS isa
                               (uint64, uintp) - general enough a user
                               program could conceivably need them too;
                               this port's own Plan9-u.h equivalent
include/kernel/*.h              xv6 kernel-ABI concepts used by BOTH
                               kernel and user code (syscall numbers,
                               open() flags, stat/file-type constants)
kernel/*.h, kernel/init/...     kernel-only headers, no user-side need
kernel/memory/<arch>/arch_vm.h    VM-only per-arch interface: pagetable_t,
                               pte_t, PGSIZE/PGSHIFT/PGROUNDUP(DOWN)
kernel/processes/<arch>/arch_proc.h  process/scheduling-only per-arch
                               interface: arch_sleep_release()
kernel/devices/<arch>/arch_disk.h  block-device-only per-arch interface:
                               arch_disk_rw() - board-scoped, not
                               ISA-scoped (see its own section below)
```

(As of 2026-09-13, `kernel/arch/<arch>/` itself holds only CPU
register/CSR headers with no subsystem of their own - `arm.h`,
`x86.h`, `riscv.h`, ... - and each fork's own `-I../../kernel/
arch/<arch>` flag still finds `arch_vm.h`/`arch_proc.h`/`arch_disk.h`
via a symlink left at their old path. See the note at the top of this
file.)

Each `include/arch/<arch>/` or `kernel/arch/<arch>/` directory is one
`-I` flag added to that fork's own `CFLAGS`/`ASFLAGS` (sometimes more
than one flag list per fork - gotcha 9 in `plan_factorization.md`: some
forks split kernel-side and `user/`-side flags across two files). A
bare `#include <arch.h>` (angle brackets, no directory prefix) or
`#include "arch_vm.h"` (quoted, no prefix) then resolves to the right
fork's own file purely through the search path - the same trick
Plan 9's own `<u.h>` uses.

## Why `arch_vm.h`/`arch_proc.h`, not `vm.h`/`proc.h`

Both `kernel/vm.h` (the eventual portable VM interface) and
`kernel/arch/<arch>/vm.h` would be reachable from the same fork's own
build (both directories sit on its own `-I` list), so a bare
`#include "vm.h"` would be genuinely ambiguous - resolved by search
order, not by intent. The `arch_` prefix can't collide with anything
portable-named. Same reasoning for `arch_proc.h`/`arch_disk.h`. Follow
this pattern for any future scoped interface header (a plausible next
one: `arch_string.h` for an `arch_stos()`-style wrapper around
`stosb()`/`stosl()`, see `plan_factorization.md`'s own `string.c`
entry) - but check the new "board-scoped, not ISA-scoped" section
below first, since not every one of these can just reuse the existing
per-ISA directory the way `arch_vm.h`/`arch_proc.h` do.

**The functions themselves are `arch_`-prefixed too, not just the
header**, settled after `sleep_release()`/`disk_rw()` were renamed to
`arch_sleep_release()`/`arch_disk_rw()`: a portable file's own call
site should read as obviously crossing into arch-specific code without
having to check which header the name came from - `arch_disk_rw(b,
write)` in `kernel/bio.c` is self-explanatory, `disk_rw(b, write)`
looked like any other portable call. Matches the same idea already
used in the user's own `~/principia/kernel/` (its `portfns_`/`portdat_`
convention marks the portable side instead; this repo marks the
arch-specific side, same reasoning in reverse). Applies going forward
to every new interface *function* invented for this purpose
(`arch_copyin`/`arch_copyout` when that one lands, say) - but not
retroactively to `myproc()`, which predates this convention and is
xv6's own long-established name, not something invented for cross-arch
dispatch here; renaming it would ripple through every caller for no
real gain. Types and constants (`pagetable_t`, `PGSIZE`, `P2V`/`V2P`)
are deliberately left unprefixed per the user's own call - those
already read as arch-specific from their file location
(`arch_vm.h`/`memlayout.h`), and `PGSIZE`/`P2V` etc. are today's
established xv6 spelling, not new coinages either.

## The "interface, not permanent fork" method

This is the actual point of the layering, not just a filing convention.
When Tier 3 work finds a real per-arch difference (not just a rename),
the move is: name the contract, put a type/constant/function of that
name in each fork's own `arch_vm.h`/`arch_proc.h`, implemented
differently per arch, so the *shared* file in `kernel/` calls it
uniformly and never needs to know which fork it's running on.

Concrete precedents so far, each a template for the next one:

- **`uintp`** (`include/arch/<arch>/arch.h`) - the pointer-sized
  integer. `uint64` on 64-bit ports, plain `uint`/`uint32` on 32-bit
  ones. The oldest one, predates this session.
- **`pagetable_t`/`pte_t`, `PGSIZE`/`PGSHIFT`/`PGROUNDUP`/`PGROUNDDOWN`**
  (`kernel/memory/<arch>/arch_vm.h`) - a page table is a different width
  and depth per arch; `PGSIZE` happens to be 4096 everywhere *today*,
  which is a fact about current hardware choices, not a portability
  guarantee (`arm`'s own buddy allocator has no `PGSIZE` concept at
  all) - so it gets a per-arch definition even where every arch's
  current value is the literal same line. Duplicated, not migrated, out
  of each fork's own big per-arch register header (`riscv.h`,
  `aarch64.h`, ...) rather than deleted from there - a shared file only
  ever includes `arch_vm.h`, so nothing is doubly defined in any one
  translation unit, and that big header's own other consumers are
  unaffected.
- **`P2V`/`V2P`** (each fork's own `memlayout.h`) - real page-table-
  aware translation on forks with a higher-half kernel (`arm64`,
  `arm64-pi4`), a trivial `#define P2V(a) (a)` identity backend on
  identity-mapped forks (`riscv64`, `riscv32`) that never needed real
  translation at all.
- **`arch_copyin()`/`arch_copyout()`** (declared in each fork's own
  `defs.h`, defined in its own `vm.c` - not yet centralized into
  `arch_vm.h` itself the way the static-inline interfaces below are,
  since the page-table-walk logic is genuinely too different per ISA to
  fit in a header; documented anyway in `kernel/memory/interface_vm.h`)
  - completed 2026-09-12/13, see `plan_factorization.md`'s own entry:
  every one of the 8 forks with this interface turned out to already
  need a real page-table walk (just narrower in scope before), not the
  trivial `memmove()` backend first predicted for `amd64`/`i386`/
  `mips`/`amd64-jserv` - renamed and widened instead of replaced.
- **`arch_sleep_release(chan, lk)`** (`kernel/processes/<arch>/arch_proc.h`)
  - most forks' own `sleep(chan, lk)` already does
  register+release+block+reacquire atomically, so `arch_sleep_release()`
  is a plain pass-through there; `riscv64`'s own `sleep()` is
  deliberately split into `sleep_prepare()` + a separate zero-arg
  `sleep()` instead, a real fix for a narrow lost-wakeup race between
  releasing the caller's lock and the process actually blocking.
  `arch_sleep_release()` lets `kernel/log.c` call the same thing either
  way, without `riscv64`'s other callers (console.c, pipe.c, proc.c,
  ...) changing at all - they keep calling the fine-grained pair
  directly.
- **`myproc()`** (each fork's own `proc.h`) - `amd64`/`i386` already
  have a real `myproc()` function; `mips` and the `arm-pi` cluster use
  a raw global (`proc`, `curr_proc`) instead. `#define myproc() (proc)`
  / `#define myproc() (curr_proc)` closed the gap for
  `kernel/sysproc-legacy.c` - the same "trivial backend" shape as
  `copyin`/`copyout`, just confirmed in practice instead of only
  predicted. Deliberately NOT renamed `arch_myproc()` - see the naming
  note above.
- **`arch_disk_rw(b, write)`** (`kernel/devices/<arch>/arch_disk.h`) -
  most of the `kernel/bio.c` cluster (`arm64`, `riscv32`, `riscv64`)
  calls `virtio_disk_rw(b, write)`; `arm64-pi4`/`loongarch` call
  `ramdiskrw(b, write)` instead (no virtio device on those boards) -
  identical `(struct buf*, int)` shape either way. See its own section
  below for why this one needed a different directory shape than the
  others.

## `interface_*.h`: documenting the contract C can't declare

Added 2026-09-13, one per `kernel/<category>/` that has grown an
`arch_*.h` family: `kernel/processes/interface_proc.h`,
`kernel/memory/interface_vm.h`, `kernel/devices/interface_disk.h`.
Each is a plain header - real prototypes and typedefs, not just prose,
matching the actual signatures below - that is **never `#include`d by
any build** (checked: no Makefile references it). C has no `interface`
keyword and no way to say "every `arch_proc.h` must define a function
with this exact signature" and have the compiler enforce it, so there
is otherwise no single place a reader can jump to that lists the whole
contract - only the scattered real implementations, each only showing
its own fork's answer. These files are that place, kept honest by
hand rather than by the compiler; nothing stops one from drifting out
of sync with the real `arch_*.h` files it describes, so treat it as
comment, not oracle - if in doubt, read the real files linked from the
top of each one.

Add one more here whenever a new `arch_*.h` family is created (the
next likely candidate is `kernel/init/`'s or `kernel/interrupts/`'s
own, once either grows a real cross-arch dispatch point the way
processes/memory/devices already have) - not for `kernel/arch/<arch>/`
itself, which holds no interface, only raw per-ISA register
definitions with nothing to document beyond what each register does.

## The check before accepting "real difference, not naming"

Before concluding two forks genuinely can't share a file over some
split, ask: **is the "simple" side actually a trivial backend of an
interface the other side needs for a real reason?** The fork lacking
the complexity is usually the easy case, not the hard one - it means
that fork's own hardware/memory model doesn't need to do the real work
at all (no separate address space -> `copyin` is just `memmove`; no
lost-wakeup race in that scheduler -> `arch_sleep_release` is just
`sleep`).
This reframing is what turned `pipe.c`'s "two irreconcilable memory
models" into "one missing interface", and what let `riscv64` join
`log.c`'s cluster instead of being deferred a second time (it was
deferred exactly this way once already, for `sleeplock.c`).

## Not every interface is ISA-scoped - some are board-scoped

`arch_vm.h`/`arch_proc.h` are shared per-*ISA*: `arm64` and `arm64-pi4`
are different boards but the same ISA, so they share one
`kernel/arch/arm64/` directory for both, and that's correct - page
table shape and the lost-wakeup race are properties of the CPU, not
the board. `arch_disk_rw()` broke that assumption the first time:
`arm64` (QEMU `virt`) has a virtio block device, `arm64-pi4` (real
Raspberry Pi 4 hardware) doesn't, so the two need genuinely different
backends despite sharing an ISA directory for everything else.

Fix: give the board that needs to differ its *own* directory
(`kernel/arch/arm64-pi4/`, now a symlink to the real file at
`kernel/devices/arm64-pi4/arch_disk.h` - see the note at the top of
this file), holding only the one file that actually diverges
(`arch_disk.h`), and list it in that fork's own `-I` flags *before*
the shared ISA directory - a quoted `#include "arch_disk.h"` then
resolves to the board-specific one first, falling through to the
ISA-level one for any fork that doesn't override it. This is the same
search-order mechanism gotcha 12 (`plan_factorization.md`) warns about
being bitten by accidentally; here it's used deliberately. Verified past
the source level too: `objdump -dr kernel/bio.o` on both forks confirms
each calls the symbol it should (`ramdiskrw` vs `virtio_disk_rw`), not
just "it compiled."

**Before assuming a new interface is ISA-scoped, ask whether every
fork sharing that ISA directory would actually agree on the answer.**
`pagetable_t`/`PGSIZE`/`arch_sleep_release` all happened to pass that
check without anyone verifying it explicitly; `arch_disk_rw` is the
first one that didn't, and won't be the last - anything
hardware-attached (disks, interrupt controllers, UARTs) is a board
property first, an ISA property only incidentally.

## Not everything gets an interface

`arch_vm.h`/`arch_proc.h` are for a genuine cross-arch *contract* -
something more than one fork's own shared file needs to call uniformly.
Code with no counterpart in any other fork (MMU walk internals, boot
sequences, interrupt controllers, `entry.S`/`swtch.S`/`trapasm.S`) stays
local - that's Tier 4, and forcing it into this shape would just be
`#ifdef`-shaped abstraction with one real implementation, not a
contract. See `plan_factorization.md`'s own "what not to do" section.

## Where this is heading

Long term, the bet is that most of `kernel/proc.c`, `kernel/vm.c`,
`kernel/trap.c` (today's Tier 4) become mostly-portable Tier 3 files
once enough of their own real differences are named as `arch_*.h`
interfaces this same way - `arch_copyin`/`arch_copyout` (real
page-table walk vs. a trivial `memmove()`, first predicted here, done
2026-09-12/13) was the last one of these; no interface is currently
known-and-not-yet-built. What's left once that's done - register/CSR
access, context-switch asm, entry/trap asm, interrupt controller
drivers, the genuinely irreducible per-ISA, per-board code - already
has somewhere to live today, not just eventually: the
`kernel/<category>/<arch>/` layout (`kernel/processes/`, `kernel/
memory/`, `kernel/interrupts/`, `kernel/init/`, `kernel/devices/`,
`kernel/syscalls/`, `kernel/arch/`) documented in
`plan_factorization.md`'s own "A parallel track" section, not a single
future `arch/<arch>/` tree.
