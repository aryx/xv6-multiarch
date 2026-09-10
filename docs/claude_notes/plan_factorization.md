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

### What the mips pilot found (2026-09-10, DONE)

`forks/mips` is through Phase 0 and green: three commits — `af3eb0f`
(.gitignore prep), `f53c972` (83 files, **all R100** pure renames),
`ff85fed` (Makefile fixup). `make test-mips` reports ALL TESTS PASSED with
the same four skipped sub-tests as before (`sbrktest`, `validatetest`,
`exitwait`, `forktest`), and `make test-all` is green across all thirteen.
`git blame -C -C` on the moved files still shows Frans Kaashoek, Robert
Morris and Russ Cox, plus `pad` on `ulib/umalloc.c` — nothing blames to the
move.

The two-commit shape held, but it needed a **third commit in front of it**,
and two of the three gotchas below cost a failed build to discover. Check
all three before moving the next fork:

**1. A bare `.gitignore` word also matches a directory — and a build output
can collide with a new directory name outright.** `forks/mips/.gitignore`
had a bare `kernel`, which would have silently ignored the whole new
`kernel/` directory; worse, the build wrote a *file* named `kernel` at the
fork root, and a file and a directory of the same name cannot coexist at
all. Anchor the patterns first, in their own commit, so the `git mv` commit
stays a pure rename. `forks/riscv64/.gitignore` already has the right shape
(`/kernel/kernel`, `/mkfs/mkfs`).

Still to fix, found by grep: bare `kernel` **and** `mkfs` in `amd64` and
`i386`; bare `mkfs` in `amd64-jserv`, `arm64`, `arm64-pi4`, `loongarch`.

**2. A make target cannot share a name with an existing directory.** Once
`kernel/` exists, a `kernel:` target is considered already built — the
directory exists and has an mtime — so make never relinks. Rename it to
`kernel/kernel` (where `riscv64` already puts it). Keep a `.PHONY` alias for
any name the top-level Makefile or the fork's `test-xv6.py` asks for by
hand, so the move stays contained inside the fork: mips needed
`kernelmemfs: kernel/kernelmemfs`.

Still to fix: `amd64` and `i386` both have a literal `kernel:` target.

**3. `ld -b binary` derives its symbols from the file path, so an embedded
blob's output path is load-bearing.** `initcode.S` may move into `kernel/`,
but the generated `initcode` blob may **not**: `ld -b binary kernel/initcode`
mangles non-alphanumerics to underscores and emits
`_binary_kernel_initcode_start/_end/_size`, while `proc.c`'s `userinit()`
reads `_binary_initcode_start`. The link fails with four undefined
references. Relocating the blob would mean editing that C, which a layout
move is not allowed to do — so move the source and leave the output path
alone.

This is the widest of the three. Five forks embed a blob this way (`i386`,
`mips`, `amd64`, `amd64-jserv`, `arm64-pi4`), and **eleven** reference
`_binary_*` symbols from C: `amd64`, `amd64-jserv`, `arm`, `arm64-pi4`,
`arm-pi1`, `arm-pi1-bis`, `arm-pi2`, `arm-pi3`, `i386`, `mips` — mostly in
`proc.c`, `memide.c` and `main.c`. Any of them that embeds `fs.img` the same
way has the identical constraint on `fs.img`'s path.

**Cheap and it worked:** keep `-I.` replaced by explicit `-Ikernel -Iuser`
rather than rewriting includes. The sources keep their existing
`#include "types.h"` / `"user.h"` spellings and stay byte-identical, which is
what makes commit 2 a build-only change. Two `_%` pattern rules — `user/%.o`
and `tests/%.o` — resolve unambiguously, since GNU make picks whichever
rule's prerequisites can actually be made and a program's `.c` exists in
exactly one directory.

**Not addressed, and not Phase 0's business:** mips still scatters user-program
build outputs (`_cat`, `cat.o`, `cat.asm`, `cat.sym`, …) across the fork root,
because the `_%` rule writes `$*.o` there. All ignored and all removed by
`clean`; a separate tidy if it's ever worth doing.

### riscv64, the second fork (2026-09-10, DONE)

Two commits, no third needed: `2437049` (15 files, all R100) and `13b78e1`.
`make test-riscv64` - this port's own harness, the one with the
crash/log/orphan-recovery tiers rather than just usertests - reports ALL
TESTS PASSED, and `make test-all` is green.

Its `user/` split into `user/` (the eleven programs plus `user.h`),
`ulib/` (`printf.c`, `ulib.c`, `umalloc.c`, `usys.pl`, `user.ld`) and
`tests/` (the standard four plus `grind`, plus this port's own
`logstress`, `forphan`, `dorphan`, `sync` - it is the only fork that has
those at all); `mkfs/` became `tools/`.

**Two things made it much cheaper than mips, and both hold for the four
forks that share its shape.** Includes needed *no* change: this port spells
them fork-root-relative already (`"kernel/types.h"`, `"user/user.h"`)
against a single `-I.`, so which directory a `.c` sits in is invisible to
it - where mips needed `-Ikernel -Iuser` precisely because its includes are
bare. And its `_%: %.o` pattern rule is not directory-anchored, so one rule
builds `user/_cat` and `tests/_usertests` alike; mips's was `_%: usr/%.o`
and had to be split in two.

**Gotcha 4, and this one is a source fix, not a build fix.** `mkfs.c`
derived each file's in-image name by hardcoding a `strncmp` against the
literal prefix `"user/"`, then asserting no `/` survived:

```
mkfs: tools/mkfs.c:142: main: Assertion `index(shortname, '/') == 0' failed.
```

`tests/_usertests` keeps its slash and aborts the build. Fix it *generally*
- take the basename, stripping to the last `/` - rather than adding
`"tests/"` as a second special case, so the coupling to directory names
goes away for good. **`riscv32`, `arm64`, `loongarch` and `arm64-pi4` each
carry their own copy of this same `mkfs.c` and will each need the same
fix.**

### Group 1 done: the four MIT-2019 siblings (2026-09-10)

`riscv32`, `arm64`, `loongarch` and `arm64-pi4` are through Phase 0, two
commits each, each verified with its own full `test-<arch>` (not just the
quick boot check) and `make test-all` green after the batch. Six forks
done, eight to go.

The recipe held with no surprises: `T=tests`, `L=ulib`, `O=tools` alongside
the existing `K=kernel` and `U=user`; `ULIB`, the `usys.pl`/`usys.S`/
`usys.o` rules, `forktest`'s own explicit rule, the `mkfs` rule, the
`UPROGS` entries, the `.d` includes and `clean` all follow their files.
Each fork needed its own copy of the gotcha-4 `mkfs.c` basename fix -
there are five separate copies of that file, not one.

**`initcode.S` stays in `user/`.** All four keep it there (riscv64 has
none), and it should not move: it is neither a test nor library code, MIT
put it in `user/`, and leaving it put keeps the generated `user/initcode`
blob's path - and so its `_binary_*` symbol names - unchanged. Gotcha 3
avoided by not moving the file rather than by working around it. Same
reasoning protects `arm64-pi4`'s `fs.img`, which it also embeds.

**Gotcha 5: in make, a bare `$X` is a *single-character* variable
reference.** `arm64-pi4` already had a `T=`, bound to `test/` - a
bare-metal hardware-probe kernel building `test/hwtest`, unrelated to the
userland test programs. Renaming it to `HW=` and writing `$HW/entry.o`
silently expands as `$(H)` followed by the literal `W/entry.o`:

```
make: *** No rule to make target 'W/entry.o', needed by 'W/hwtest'.  Stop.
```

Multi-character variable names must be written `$(HW)`. Renamed to `H=`
instead, matching the single-letter convention. Worth noting *how* this was
caught: the kernel build passed clean, because nothing in the main build
path touches `$H`. Only building the fork's **other** target found it. When
a fork has a second, non-default target, build that too before believing
the fixup commit.

### Group 2 done: the flat MIT-classic pair (2026-09-10)

`i386` and `amd64` are through Phase 0 - one shared `.gitignore` prep
commit plus two commits each - and both report ALL TESTS PASSED on their
own full `test-<arch>`, with `make test-all` green after. **Eight forks
done, six to go.**

Both hit gotchas 1, 2 and 3 exactly as predicted, and both embed *two*
blobs (`initcode` and `entryother`), so two generated paths had to stay at
the fork root rather than one.

**Gotcha 6: use `-idirafter`, not `-I`, for the host-built tools.** `mkfs`
is compiled by the host `gcc` and includes the system `<fcntl.h>` alongside
xv6's own `"fs.h"`. `-Ikernel` is searched *before* the system directories,
so `<fcntl.h>` resolved to `kernel/fcntl.h` - which defines `O_RDONLY`,
`O_WRONLY`, `O_RDWR`, `O_CREATE` and no `O_TRUNC`:

```
tools/mkfs.c:87:39: error: 'O_TRUNC' undeclared (first use in this function)
```

`-idirafter kernel` puts it after the system directories instead. `mips`
already used this idiom (`-idirafter .`); the riscv-family forks never hit
it because they spell their includes `"kernel/fs.h"` against a plain `-I.`.

**Gotcha 7: the pre-2019 `mkfs.c` uses one string for two jobs.** It opens
`argv[i]` and also writes `argv[i]` into the directory entry - fine while
every program sat in the fork root, which its `assert(index(argv[i],'/')==0)`
enforced. `open()` still needs the full path; the directory entry needs the
basename. Split them, keeping the assert on the basename as documentation.
This is gotcha 4's flat-fork variant: the riscv-family `mkfs.c` at least had
a `shortname` variable to fix, this one does not.

**And a reminder of gotcha 5's real lesson, hit again:** the `_forktest`
rule has *comment lines between the target and its recipe* in every fork
that has it, so a patch keyed on target-plus-recipe silently does not
apply. Here the generic `_%: %.o $(ULIB)` rule then took over and linked
`printf.o` into forktest, which defines its own `printf` - caught only as
`multiple definition of 'printf'` at link time. Check that the special
`_forktest` rule actually changed before building.

**Also decided here: `tools/` means what the *build* and a debugger run.**
Both forks still carry MIT's book typesetting pipeline (`runoff`,
`runoff1`, `runoff.list`, `runoff.spec`, `toc.ftr`, `toc.hdr`, `pr.pl`,
`show1`, `cuth`). It stays at the fork root, deliberately: `runoff.list` is
a list of *bare* source filenames (`types.h`, `param.h`, `defs.h`, ...)
that Phase 0 has just moved into `kernel/`, so relocating the set would
mean rewriting its data file - and there is no troff/PDF toolchain here to
verify the result. Leaving it untouched is the verifiable choice.

### amd64-jserv (2026-09-10, DONE)

Two commits, no prep needed, `test-amd64-jserv` green. **Nine forks done,
five to go** - all five remaining are ARM.

This is the fork that suggested `ulib/` and `tools/` in the first place, so
only two things were missing: `tests/` split out of `user/`, and
`include/` **dissolved** - its 22 headers into `kernel/`, `user.h` into
`user/`, and `symlink.patch` (not a header) to the fork root. It was the
only fork of the fourteen with an `include/`, and the Phase 0 target puts
headers beside their consumers.

None of gotchas 1, 2, 3 or 7 applied, for one reason worth generalising:
**this fork's build is already decoupled from its source layout.** Objects
go to `.kobj/` and `.uobj/`, outputs to `out/`, and the filesystem is staged
in `.fs/` - so there is no bare `kernel`/`mkfs` ignore word, no target
sharing a name with a directory, no embedded blob whose path could move,
and `UPROGS` is a flat list of bare names that needed no change at all.
Where a program compiles from was already independent of what it is called
in the image. Worth remembering as the shape the other forks should
eventually converge on.

`mkfs.c` here already separates the host path it opens from the name it
writes - by stripping a hardcoded `".fs/"`. That is the same brittleness
the others' `"user/"` prefix had, and was deliberately left alone: the move
does not force it, and converging the seven divergent copies of `mkfs.c` on
one basename idiom is Tier 3 work, not a layout commit's business.

### arm (2026-09-10, DONE)

Four commits — the move, the build fixup, then a flatten of `kernel/device/`
and its own fixup (see the residue rule above for why). `test-arm` green,
`test-all` green. **Ten forks done, four to go**, all four Raspberry Pi ports.

This fork was the clearest illustration of what the exercise is for: its
`lib/` held `string.c`, a *kernel* library, where `mips`'s `lib/` held the
*user* one. Same name, opposite meanings.

**The pleasant surprise: `vpath` does almost all the work for a fork with a
sub-Makefile.** `arm` builds its userland from `usr/Makefile` (now
`user/Makefile`). Rather than rewrite every rule to reach `../tests` and
`../ulib`, two lines —

```make
vpath %.c ../tests ../ulib
vpath %.S ../ulib
```

— leave the objects and linked binaries landing in `user/` exactly as before,
so `UPROGS`, `ULIB` and the `$(MKFS)` invocation are **all unchanged**, `mkfs`
still sees bare names with no `/`, and gotcha 7 never arises. This is the same
source-layout/build-output decoupling `amd64-jserv` gets from staging into
`.fs/`, reached a different way. **Reach for `vpath` first on the four
remaining Pi ports** — they all have sub-Makefiles too.

One wrinkle worth naming, because it bit mid-commit: `git add -A` before
`git commit --amend` will sweep working-tree Makefile edits **into the move
commit**, destroying its purity (it showed up as `3 M` and one `R057` among
the renames). Recovering means `git reset --soft HEAD~1`, restoring the
touched files from `git show HEAD:<old path>`, committing the renames alone,
then re-applying the edits. Simpler not to amend a move commit at all once
fixup work has started.

**Gotcha 8: a linker script can name object files by their build path.**
`kernel/kernel.ld` places `.start_sec` by listing `build/entry.o` and
`build/start.o` explicitly, which became `build/kernel/*.o`. This cost the
longest debugging of the eight, because of how it presents:

```
arm-linux-gnueabihf-ld: cannot find build/entry.o: No such file or directory
```

while the link command line contains `build/kernel/entry.o` and no
`build/entry.o` anywhere, and `make -n` prints a command that looks entirely
correct. Grepping the Makefile for the missing path finds nothing. **When a
link fails naming a file that is not on the command line, read the linker
script.**

### Suggested order for the remaining twelve

Recon corrected an earlier guess that `arm-pi1-bis` would be an easy
second. It is not: it is a *nested sub-build* whose `usr/` has its own
Makefile, produces `fs.img` itself, and carries its own copy of sixteen
kernel headers - of which 8 are byte-identical but **8 genuinely differ**
(`types.h` by 97 diff lines, `arm.h` 85, `mmu.h` 72, `defs.h` 59). That is
a real userland header set, not a stale copy, and merging it is Tier 2
work, not Phase 0's. Its root Makefile also already works around gotcha 3
by hand, copying `build/initcode` to a bare root name before linking and
deleting it after - do not disturb that.

So, cheapest first:

1. ~~**`riscv32`, `arm64`, `loongarch`, `arm64-pi4`**~~ - **DONE**
   2026-09-10, see above.
2. ~~**`i386`, `amd64`**~~ - **DONE** 2026-09-10, see above.
3. ~~`amd64-jserv`~~ - **DONE** 2026-09-10, see above; it turned out to be
   the cheapest of the fourteen, not one of the hardest.
4. **`arm`, `arm-pi1`, `arm-pi1-bis`, `arm-pi2`, `arm-pi3`** - the five
   remaining, all ARM: nested sub-builds and/or duplicated header sets.
   Real design work; `arm-pi1-bis` alone has 8 of 16 userland headers
   genuinely divergent from the kernel's copies.

**Decide the residue rule once, not per fork: `kernel/` is FLAT, everywhere.**
Some files fit none of the five buckets: `arm/device/` (`gic.c`, `timer.c`,
`uart.c`), the Pi ports' `entry.s`/`exception.s`/`csud_glue.c`/`font.bin`, and
the linker scripts (`kernel.ld`, `loader.ld`, `armstub64.S`). All of it goes
directly in `kernel/` — no subdirectories, not even ones a fork already had.

Tested and confirmed on `arm` (2026-09-10). Its `device/` was first kept as
`kernel/device/`, because `arm.h` spells its include `"device/versatile_pb.h"`
and nesting avoided touching a source file. That was the wrong trade:
**Phase 0's job is to make the fourteen forks structurally consistent so they
can be merged, not to design the final subdivision.** `kernel/device/` was a
one-fork exception — no other fork has it — so keeping it would leave the merge
reconciling one extra level against thirteen forks without one. It was
flattened in a follow-up pair of commits, and `arm.h`'s include turned out to
be the only one of its kind in the entire repo, so the cost was a single line.

The meaningful split into subdirectories belongs **after** the merge, where it
can be designed once across all the ports at the same time rather than
inherited piecemeal from whichever fork happened to have a directory. Do not
invent `device/` or `boot/` now, and do not preserve one a fork brought with
it.

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
