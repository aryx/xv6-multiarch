# Plan: factoring the thirteen architectures together

**Status:** proposed, not started.
**Blocked on:** [plan_build_and_test.md](plan_build_and_test.md). Do not start
this without a working per-arch build and boot check. Merging files you cannot
verify is precisely how `gitlab.com/xv6-multiarch` — the abandoned project
this repo descends from — died. It got the architectures into one tree and
then stalled at exactly this step.

## Goal

Move from thirteen near-duplicate trees to a Linux-style layout: common code
shared, arch-specific code isolated behind a clean boundary.

```
user/            common userland (sh, ls, cat, echo, ...)
kernel/          common kernel (fs, log, pipe, file, syscall dispatch)
include/         common headers
arch/<name>/     arch-specific kernel, headers, boot, linker script, Makefile frag
```

## Two hard rules

**1. Preserve blame: delete and create in ONE commit.** When a shared file is
created, delete the per-arch copies *in the same commit*. Git's rename/copy
detection only bridges a delete and an add within a single commit, so
`git blame -C -C` on the resulting shared file can then attribute each line to
whichever port it came from — including the ARM and RISC-V porters' own work.
Split across two commits, that attribution is lost permanently.

This is the same mechanism that made this repo's grafts work, and it is the
whole reason all thirteen copies were imported before unifying rather than
after. Verify after each merge:

```sh
git blame -C -C user/<file> | awk '{print $2}' | sort | uniq -c
```

Expect to see several original authors. If everything blames to the merge
commit, the merge was done wrong — fix it before moving on.

**2. Prefer per-arch files over `#ifdef`.** Linux deliberately keeps
`#ifdef CONFIG_<ARCH>` out of common code, using per-arch implementations
behind a common interface. xv6's entire value is that a student can read it;
common files laced with thirteen-way `#ifdef` would be *worse* than the
duplication we have now. Reserve `#ifdef` for genuinely small differences
(a type width, a constant). If a function body differs, it belongs in
`arch/<name>/`.

## What the tree actually looks like today

Measured across all thirteen `arch/` subtrees, counting distinct blob contents
per filename:

```
across all shared basenames: 930 copies, 659 distinct contents
```

So roughly 29% whole-file redundancy — but it is very unevenly distributed,
and the userland is where the wins are:

| file | in N arches | distinct variants |
|---|---|---|
| `echo.c` `ln.c` `mkdir.c` `rm.c` `wc.c` `umalloc.c` | 13 | 4 |
| `syscall.h` | 13 | 4 |
| `fcntl.h` `stat.h` `forktest.c` `kill.c` `stressfs.c` | 13 | 5 |
| `fs.h` `grep.c` `init.c` `ls.c` `sh.c` `zombie.c` | 13 | 6 |
| `elf.h` `spinlock.h` `cat.c` `user.h` | 13 | 7 |

## The single most important structural finding

The ports fall into **two families**, split by MIT's 2019 reorganisation
(flat root → `kernel/` + `user/`, with header paths changed to match):

- **x86 family** — `x86`, `amd64`, `x86_64`, `mips`, `rpi1`, `rpi2`,
  `armv6-rpi` (and `armv7-rpi` nearby). These seven share *byte-identical*
  `echo.c`, `ln.c`, `mkdir.c`, `rm.c`, `wc.c`, `umalloc.c`.
- **riscv family** — `aarch64`, `d1`, `loongarch`, `rv32`. These four share
  *byte-identical* `ls.c`, `grep.c`, `kill.c`, `zombie.c`, and three share
  `cat.c`.

Between the families the same files differ only by the 2019 changes (e.g.
`#include "types.h"` vs `#include "kernel/types.h"`).

**Therefore: unify within each family first, then reconcile the two families.**
Trying to merge across the boundary first means fighting the reorg on every
single file simultaneously. Doing it second means the cross-family diff is one
well-understood, mechanical change applied once.

## Work queue

Ordered by risk. One file per commit; each commit builds and boots every arch
it touches.

**Tier 0 — free wins (byte-identical, no edit needed).**
Within the x86 family: `echo.c`, `ln.c`, `mkdir.c`, `rm.c`, `wc.c`,
`umalloc.c` — 7 arches, one content. Within the riscv family: `ls.c`,
`grep.c`, `kill.c`, `zombie.c` — 4 arches, one content. These are pure
deletions plus one add. Do these first: they exercise the blame-preservation
rule and the test harness on zero-risk changes.

**Tier 0.5 — intra-arch duplication.** The Raspberry Pi ports carry the *same
header twice inside one arch* (e.g. `include/fcntl.h` and `uprogs/fcntl.h`).
Free to collapse, and it shrinks the later work.

**Tier 1 — near-identical userland.** `sh.c`, `cat.c`, `init.c`, `stressfs.c`,
`forktest.c`. Small reconciliations only. Rule: if a file needs an edit before
it can be shared, make the *move* commit first (pure, blame-preserving), then
a separate edit commit. Never mix a move with a change.

**Tier 2 — headers.** `fcntl.h`, `stat.h`, `syscall.h`, `fs.h`, `user.h`,
`elf.h`, `spinlock.h`. Higher coupling; these define the interface between
common and arch code, so settle the boundary here before touching the kernel.

**Tier 3 — common kernel.** `fs.c`, `log.c`, `bio.c`, `pipe.c`, `file.c`,
`string.c`, and the arch-independent half of `syscall.c`. Genuinely shared
logic, but with real per-arch drift accumulated over a decade.

**Tier 4 — the irreducibly arch-specific.** `proc.c`, `vm.c`, `trap.c`,
`swtch.S`, `entry.S`, `trapasm.S`, `mmu.h`. These stay in `arch/<name>/`.
The work here is *defining the interface* they implement, not merging them.
This is the real design work of the project and should not be rushed at.

## Build system

Thirteen Makefiles become one plus per-arch fragments:

```
Makefile                 # common rules, ARCH=<name> selects a fragment
arch/<name>/config.mk    # TOOLPREFIX, CFLAGS, QEMU machine, link script
```

This should land early — around Tier 1 — because every subsequent merge needs
`make ARCH=x` to work for all thirteen, and the test harness from the other
plan already needs per-arch manifests. Fold the two: the harness manifests and
the build fragments should be the same files.

## What not to do

- **Do not unify the two families prematurely.** The riscv-family layout is
  MIT's own deliberate later design; it is not drift to be corrected.
- **Do not delete a port's copy without a passing build for that port.** A
  file being byte-identical does not mean the port's Makefile finds it in the
  new location.
- **Do not "clean up while you're in there."** Reformatting or fixing a bug in
  the same commit as a move destroys the copy detection that preserves
  authorship. Move first, then change.
- **Do not chase 100%.** Some duplication is honest: thirteen `proc.c` files
  that genuinely differ are thirteen files. The goal is a readable teaching OS,
  not a minimal byte count.

## Definition of done

There isn't a single finish line, so define it per tier. A tier is done when:

- every file in it exists once, in `user/`, `kernel/` or `include/`;
- `git blame -C -C` on each shared file still shows the original authors, not
  the merge commit;
- the test matrix is no worse than before the tier started;
- the diffstat shows deletions vastly outnumbering additions.

A reasonable first milestone: **Tier 0 and Tier 0.5 complete for both families,
with the matrix unchanged.** That alone removes ~60 duplicate files and proves
the method end to end.
