# Plan: factoring the thirteen architectures together

**Status:** proposed, not started — but **no longer blocked** as of
2026-09-09.
**Was blocked on:** [done/plan_build_and_test.md](done/plan_build_and_test.md),
which is now done: all 14 ports build, boot to a shell under QEMU and pass
their own `usertests`, and `make test-all` re-checks every one of them in
about a minute. That is the safety net this plan required. The residue that
plan left behind — a few skipped `usertests` sub-tests, `arm64-pi4`'s boot
kept out of CI, no real-hardware verification — is tracked in
[plan_build_and_test_2.md](plan_build_and_test_2.md) and does **not** block
this work; see that file's own header for why.

The rule that produced the block still stands, though, and now applies
per-commit rather than per-phase: do not merge a file you cannot rebuild and
re-boot afterwards. Merging files you cannot verify is precisely how
`gitlab.com/xv6-multiarch` — the abandoned project this repo descends
from — died. It got the architectures into one tree and then stalled at
exactly this step. Run `make test-all` after every merge commit, and
`stress-test-all` before trusting a batch of them.

## Goal

Move from thirteen near-duplicate trees to a Linux-style layout: common code
shared, arch-specific code isolated behind a clean boundary.

```
user/            common userland (sh, ls, cat, echo, ...)
ulib/            common user library linked into every program (printf, ulib,
                 umalloc; usys.S is the arch-specific part and leaves later)
tests/           test programs (usertests, forktest, stressfs, zombie, grind)
kernel/          common kernel (fs, log, pipe, file, syscall dispatch)
tools/           host-built tools (mkfs, vectors.pl, sign.pl, gdbinit)
arch/<name>/     arch-specific kernel, headers, boot, linker script, Makefile frag
```

Headers live next to their consumers (`kernel/types.h`, `user/user.h`), as in
MIT's own 2019 layout — there is deliberately no `include/`; see Phase 0.

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

## Phase 0 — normalise every fork's layout first

Before merging anything across forks, give all fourteen the same internal
shape:

```
kernel/  user/  ulib/  tests/  tools/
```

**Why this comes first.** Fourteen forks currently carry *five* different
layouts:

| shape | forks |
|---|---|
| flat (pre-2019 MIT) | `amd64`, `i386` |
| flat kernel + `usr/` `lib/` `tools/` | `arm`, `mips`, `arm-pi1-bis` |
| `kernel/` `user/` `mkfs/` (MIT 2019) | `riscv64`, `riscv32`, `arm64`, `arm64-pi4`, `loongarch` |
| `source/` `include/` `uprogs/` | `arm-pi1`, `arm-pi2`, `arm-pi3` |
| `kernel/` `user/` `ulib/` `tools/` `include/` | `amd64-jserv` |

and the divergence is not merely cosmetic:

- **`lib/` means opposite things.** `forks/arm/lib/` is `string.c` — a *kernel*
  library. `forks/mips/lib/` is `printf.c ulib.c umalloc.c usys.S` — a *user*
  library.
- **`mkfs.c` is a host tool built with the host `cc`, but it sits in `uprogs/`
  in all three Pi ports**, among cross-compiled guest binaries. Different
  toolchain, different build-rule class, same directory.
- **The Pi ports carry the same header twice inside one fork** —
  `arm-pi1/include/user.h` and `arm-pi1/uprogs/user.h`, likewise `arm-pi2`,
  `arm-pi3`; `arm-pi1-bis` has `user.h` at its root *and* `usr/user.h`. This is
  the old Tier 0.5, and normalising forces it to be resolved rather than
  carried forward.

**Why it is low risk.** Two properties no later tier has:

1. **Every move is a pure `git mv`.** Nothing is deleted, nothing is shared, no
   content changes — blame preservation is free rather than something to verify
   carefully. It exercises `make test-all` across fourteen commits of zero
   semantic risk before a single byte is merged.
2. **The classification is already written down, in each fork's own Makefile.**
   `forks/i386/Makefile` has `OBJS` (→ `kernel/`), `UPROGS` (→ `user/` and
   `tests/`), `ULIB = ulib.o usys.o printf.o umalloc.o` (→ `ulib/`). Splitting
   the 46 flat files in `i386`/`amd64` is derivable from lists upstream already
   maintains, not a judgement call.

**Why these five directories and not MIT's two.** `kernel/` + `user/` with
headers beside their consumers is MIT's own 2019 design, already the shape of
five forks — do not "correct" it, and do not invent an `include/` that only
`amd64-jserv` uses. The other three are each justified by a boundary that
actually exists:

- **`ulib/` marks the arch boundary.** `printf.c`/`ulib.c`/`umalloc.c` are
  arch-independent; `usys.S` (and `usys.pl`, in the five riscv-family forks) is
  syscall-stub assembly and is not. Separating `ulib/` from `user/` makes
  visible that this directory later splits into a shared `ulib/` plus
  `arch/<name>/usys.S`, which `user/` — one file per program, all shareable —
  never does.
- **`tools/` marks the toolchain boundary** — host `cc`, not the cross
  compiler.
- **`tests/` quarantines the one file that will never merge.** Measured across
  all fourteen forks:

  | file | forks | distinct contents | avg lines |
  |---|---|---|---|
  | `usertests.c` | 14 | **12** | 2148 |
  | `grind.c` | 4 | 3 | 350 |
  | `zombie.c` | 14 | 6 | 15 |
  | `forktest.c` | 14 | 5 | 56 |
  | `stressfs.c` | 14 | 5 | 49 |
  | *(for contrast)* `echo.c` | 14 | 4 | 15 |

  `usertests.c` alone is **30,080 lines across the tree — more than all
  fourteen other user programs combined (14,499)** — and is the single most
  divergent file in the repo, at 12 distinct contents for 14 copies. The
  divergence is real and mostly permanent: `riscv64` vs `riscv32` is a 3,222-line
  diff, and each port has its own skipped sub-tests (see `plan_build_and_test_2.md`
  item 1). Left in `user/`, it dominates the directory whose whole point is that
  its files merge 4-to-1. Note the corollary: `forktest.c`, `stressfs.c` and
  `zombie.c` have the *mergeability profile of ordinary userland*, not of
  `usertests.c`. They go in `tests/` anyway — the classification is by intent,
  so that it stays stable as files merge.

**How to sequence it.** One fork per pair of commits, never one:

1. the `git mv` commit — pure renames, no content change;
2. the Makefile fixup commit — the only real risk, kept separate so the rename
   detection in (1) is unambiguous.

Run `make test-<arch>` (the full form, not `quick-test`) after each fork's pair.
Path coupling varies a lot: `amd64-jserv` (26 directory references in its
Makefile) and `arm-pi2` (17) are the hardest; `arm` has essentially none.
Suggested pilots: `mips` and `arm-pi1-bis`, which between them hit the
`lib/`-ambiguity and the duplicate-header cases and are small enough to verify
quickly.

**Decide the residue rule once, not per fork.** Some files fit none of the five
buckets: `arm/device/` (`gic.c`, `timer.c`, `uart.c`), the Pi ports'
`entry.s`/`exception.s`/`csud_glue.c`/`font.bin`, and the linker scripts
(`kernel.ld`, `loader.ld`, `armstub64.S`). Put all of it in `kernel/` — MIT
keeps `kernel.ld` in `kernel/` — and let Tier 4 lift it into `arch/<name>/`
later. Do not invent `device/` or `boot/` now and move everything twice.

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

**Superseded by Phase 0 (2026-09-09).** The original conclusion here was
"unify within each family first, then reconcile the two families". That was
right only as long as the layouts stayed as they are. Phase 0 normalises all
fourteen layouts up front, which dissolves the family boundary as a *structural*
problem: afterwards the two families differ only by `#include "kernel/types.h"`
vs `#include "types.h"` — one mechanical content diff applied once, not an
ordering constraint the whole work queue has to be sequenced around. The
finding above is kept because it still describes where the byte-identical
clusters are, and Tier 0 still exploits them.

## Work queue

Ordered by risk, and starting *after* Phase 0 above. One file per commit; each
commit builds and boots every arch it touches.

**Tier 0 — free wins (byte-identical, no edit needed).**
Within the x86 family: `echo.c`, `ln.c`, `mkdir.c`, `rm.c`, `wc.c`,
`umalloc.c` — 7 arches, one content. Within the riscv family: `ls.c`,
`grep.c`, `kill.c`, `zombie.c` — 4 arches, one content. These are pure
deletions plus one add. Do these first: they exercise the blame-preservation
rule and the test harness on zero-risk changes.

**Tier 0.5 — intra-arch duplication.** *Folded into Phase 0.* The Raspberry Pi
ports carry the *same header twice inside one arch* (e.g. `include/fcntl.h` and
`uprogs/fcntl.h`); normalising those forks' layout forces the collapse rather
than carrying the duplicate into the new tree.

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

A reasonable first milestone: **Phase 0 complete for all fourteen forks, with
the matrix unchanged.** Every fork then has the same five directories, the
family boundary is reduced to an include-path diff, and the method — pure
`git mv`, verify, then fix the build — has been proved end to end fourteen
times before anything is actually merged.
