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

### arm-pi1 (2026-09-10, DONE)

Two commits, `test-arm-pi1` green, `test-all` green. **Eleven forks done,
three to go.** 93 files moved, all R100; only `.gitignore`, `LICENSE`,
`Makefile`, `README` and `test-xv6.py` stay at the fork root.
`source/`, `include/` and `uprogs/` are all gone.

**The two header sets stay separate.** Of the 16 basenames this fork had in
both `include/` and `uprogs/`, **7 genuinely differ** — `types.h` by 97 diff
lines, `arm.h` by 85, `mmu.h` by 72, `defs.h` by 59, plus `memlayout.h`,
`proc.h`, `traps.h`. `uprogs/` is a real, deliberately distinct userland
header set, not a stale copy, so it moved to `user/` intact and `include/`
moved to `kernel/` intact. That leaves `kernel/user.h` and `user/user.h`
both present — identical files, the duplication the fork already had,
preserved rather than resolved. Reconciling them is Tier 2. **Expect the
same on `arm-pi1-bis`, `arm-pi2` and `arm-pi3`.**

`vpath` again did the work in `user/`, but this fork needed two things
`arm` did not:

1. **`-iquote .` is load-bearing.** `arm`'s sub-Makefile already pointed at
   the shared headers explicitly; this one relied on `usys.S`, `ulib.c` and
   the programs all sitting in the same directory as the headers they
   include. Compiled out of `../ulib` or `../tests`, a quoted `#include`
   resolves from the *includer's* directory and finds nothing.
2. **Gotcha 9: `ASFLAGS` is not `CFLAGS`, and a Makefile may not set it at
   all.** Adding `-iquote .` to `CFLAGS` fixed every `.c` and left `usys.S`
   alone failing with `traps.h: No such file or directory` — make's built-in
   `.S` rule uses `$(ASFLAGS)`/`$(CPPFLAGS)`, never `$(CFLAGS)`, and this
   Makefile defined neither. Expect it on the remaining Pi ports.

**Two process mistakes, both worth not repeating:**

- **`git mv source/* kernel/` aborts the whole operation** if any matched
  path is untracked (here a stale `source/fs.img` build output) — and it
  aborts *silently enough* to look partial: other `git mv` calls in the same
  loop had succeeded, so the staged diff looked plausible while `source/`
  had not moved at all. Then `rm -rf source/` deleted 30 tracked files.
  Recovered with `git reset -q HEAD -- <fork>`, `git checkout -- <fork>`,
  `git clean -fdq <fork>`. **Run `git clean -fdxq <fork>` first, then
  enumerate with `git ls-files` and move only tracked paths** — that is what
  worked on the retry.
- **Never `git checkout -- <dir>` while fixup edits are uncommitted.** Doing
  it to get a pristine tree for the rebuild check silently discarded all
  three Makefile fixes, and the "from-scratch rebuild" then failed for that
  reason rather than a real one. `git clean -fdxq` alone removes build
  output without touching tracked modifications.

### The Pi ports' duplicated header sets are smaller than they look (2026-09-10)

Prompted by an obvious question about the `arm-pi1` move — why is `proc.h`
in `user/`? — all four Raspberry Pi ports turn out to carry **seven dead
kernel headers** in their userland set, identical in all four:

```
buf.h  defs.h  elf.h  file.h  mmu.h  proc.h  spinlock.h
```

Not included by any userland source — not the programs, not the test
programs, not `ulib`, not `mkfs.c` — directly or transitively. (In these
forks no userland header includes another, so direct use *is* transitive
use.) Removed in one commit across all four; each still builds and reports
ALL TESTS PASSED. Deliberately a separate commit from any Phase 0 move,
which stay pure renames.

**This materially shrinks Tier 2.** `arm-pi1` looked like it had seven
headers genuinely diverging between its two sets — `arm.h`, `defs.h`,
`memlayout.h`, `mmu.h`, `proc.h`, `traps.h`, `types.h` — but `defs.h`,
`mmu.h` and `proc.h` were all dead. What actually has to be reconciled is
**four** headers that are both really used by userland and really different
from the kernel's copy:

```
arm.h  memlayout.h  traps.h  types.h
```

The rest of each userland set is either identical to the kernel's copy or
gone. Worth re-running the same reachability check on any fork before
assuming its duplication is real:

```python
# roots = every #include "..." in user/, tests/, ulib/, tools/
# then close over the headers found in the userland dir itself
# anything in user/*.h not reached is dead
```

### arm-pi2 (2026-09-10, DONE)

Three commits — the move, a correction to it, then the build fixup.
`test-arm-pi2` green, `test-all` green, **and a Docker build green**.
**Twelve forks done, two to go.**

The `arm-pi1` recipe transferred almost exactly: `-iquote .` on `CFLAGS`, a
new `ASFLAGS = -iquote .` (gotcha 9 again), two `vpath` lines so `UPROGS`,
`ULIB` and the `./mkfs` invocation need no change, and `mkfs.c` compiling
from `../tools/` with `-iquote .` rather than `-I` (gotcha 6). Its
`-I include` appears in all **three** `hw=` branches (fvp, rpi1, rpi2), not
one.

**Gotcha 10: a wildcard object list will silently enlist a file you move
into its directory.** `loader.S` and `loader.ld` were at the fork root, not
in `source/`, and that placement is load-bearing. `loader.S` is the
second-stage boot loader for real rpi2 hardware; it does
`.incbin "kernel7.bin"`, i.e. it *embeds the finished kernel* rather than
being part of it, and has its own linker script and its own explicit
`loader:` target. But the kernel's object list is a wildcard:

```make
ASM_OBJECTS = $(patsubst $(SOURCE)%.S,$(BUILD)%.o,$(wildcard $(SOURCE)*.S))
```

so moving it into `kernel/` enlisted it in the kernel's own build, where it
failed on the image it wants to embed and does not exist yet:

```
kernel/loader.S:42: Error: file not found: kernel7.bin
```

Moved back to the root in its own pure-rename commit. **Before moving a
file into `kernel/`, check whether a wildcard there would pick it up, and
whether it is kernel source at all or a separate artifact that wraps the
kernel.** `arm-pi3` has `loader.S`, `loader.ld` *and* `armstub64.S` at its
root for exactly this reason — leave all three there.

**Process fix applied here, after the `.dockerignore` CI regression:** a
fork is not done until `docker build --build-arg ARCH=<name>` passes, not
just `make test-<arch>`. `make test-all` cannot see build-context problems
at all, because every file is present on the host by construction. This is
CLAUDE.md's own long-standing rule; Phase 0 had not been following it.

### arm-pi1-bis (2026-09-10, DONE)

Two commits. `test-arm-pi1-bis` green, `test-all` green, `docker build
--build-arg ARCH=arm-pi1-bis` green. **Thirteen forks done, one to go —
`arm-pi3`.**

Structurally the closest of the Pi ports to `arm`: flat kernel sources plus
`usr/` and `tools/`, a `makefile.inc` shared by all three Makefiles. 81
files, all R100.

**Gotcha 8 applies here too.** `kernel/kernel.ld` places its entry section
by naming object files by build path — `build/entry.o` and `build/mmu.o`,
now `build/kernel/*.o`. Second fork of the fourteen to do this, after `arm`.

**A second, distinct instance of the `.incbin` problem.** `wrapper.s` does
`.incbin "font1.bin"`, and the `.s` and the font moved into `kernel/`
together; the assembler resolves `.incbin` against its include path, which
this fork's `ASFLAGS` did not set. So `ASFLAGS` needed `-Ikernel` for a
reason that has nothing to do with headers. Generalising: **an `.incbin`
is a build-time file reference that no grep for `#include` will find** —
check for them explicitly. `arm-pi1` and `arm-pi2` had theirs satisfied by
an existing `-I $(SOURCE)`; this one did not.

**And one worth admitting: a hand-check caught what a regex missed.** The
`OBJS` list's last entry is `vm.o \` — with a space before the
continuation — so the scripted edit that prefixed every other entry skipped
it silently, and the build failed with `No rule to make target
'build/vm.o'`. Cheap to spot here because the link named the missing file.
A scripted edit over a hand-maintained list needs its result read back, not
just its exit status checked.

### arm-pi3 (2026-09-10, DONE) — Phase 0 COMPLETE, all fourteen forks

Two commits. `test-arm-pi3` green, `test-all` green, `docker build
--build-arg ARCH=arm-pi3` green. **170 files moved, all R100 — the largest
move of the set**, because this port vendors the whole USPi USB stack
alongside xv6's own sources.

Ironically the *smallest* fixup of the four Pi ports: nothing here names an
object by build path (no gotcha 8), and `entry.S`'s three `.incbin`
references — `font1.bin`, `initcode`, `fs.img` — were already satisfied by
an existing `-I source`, which simply became `-I kernel`. `kernel.ld` is
referenced in four places, across two build trees (`build/` for real
hardware, `build-qemu/` for the emulator).

**The one real exception to the flat-`kernel/` rule, and it is forced.**
`kernel/uspi/` and `kernel/uspienv/` keep their subdirectories. Two things
make them different in kind from `arm`'s `device/`, which was flattened:

- they are the public API of a vendored third-party library, spelled with
  the directory in the include — `#include <uspi/dwhcidevice.h>` — across
  ~30 source files. Flattening means editing all of them, which a layout
  move may not do.
- flattening is not even *possible*. `uspi/types.h` and `include/types.h`
  are different files, as are `uspi/bcm2835.h` and `uspienv/bcm2835.h`. Two
  different headers cannot both be `kernel/types.h`.

So the rule stands as written — no *preserved preference* subdirectories —
with the amendment that a genuine basename collision in vendored
third-party code is an exception, and one Tier 4 will want to keep together
anyway.

`loader.S`, `loader.ld`, `armstub64.S` and `armstub64.ld` stay at the fork
root (gotcha 10): `loader.S` `.incbin`s the finished kernel, `armstub64.S`
is a separate AArch64 stub QEMU loads *instead of* the kernel, and neither
being in `$(SOURCE)` is what stops the wildcard object lists enlisting
them. `timetest` and `benchmark` join `tests/` by intent — they exercise
and measure the kernel rather than being usable userland.

## Phase 0 is complete

All fourteen forks now have the same five directories:

```
kernel/  user/  ulib/  tests/  tools/
```

Verified per fork: pure-rename move commit (every file `R100`), a separate
build-fixup commit, a from-scratch rebuild on a `git clean -fdx` tree, that
fork's own full `test-<arch>`, and `docker build --build-arg ARCH=<name>`.

What this bought the rest of the plan: the **two-family split is gone as a
structural problem**. The x86-family/riscv-family boundary that the "single
most important structural finding" section is built on *was* the layout
split; what remains between them is one mechanical include-path difference.
Tier 0 can now compare `*/user/echo.c` across fourteen forks by path rather
than by ad-hoc mapping.

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

### Started (2026-09-10)

Two merges landed, both following the same rule: take the most complete
version, not the most common one, and keep a fork's own copy only when it
has a real, stated reason to differ.

**Gotcha 11, found by a real CI break (2026-09-11): deleting the last
tracked file in a directory deletes the directory too, from git's point of
view.** `5d8aab0` deleted `forks/{riscv32,arm64,arm64-pi4,loongarch}/tools/
mkfs.c` (moved to the shared `../../tools/mkfs.c`) without noticing it was
the only tracked file each of those `tools/` directories had. Git does not
track empty directories, so a fresh checkout (CI, `docker build`) never
recreates `tools/`, and `gcc -o tools/mkfs ...` fails with "cannot open
output file tools/mkfs: No such file or directory". Every local build
stayed green throughout, because the directory physically survived on disk
from before the deletion - only a truly clean tree (verified here with
`git archive HEAD | tar -x` into a scratch directory, not just `make
clean`) exposes it. Same bug class `forks/i386`'s own `user/%.o` rules
already document from an earlier undocumented session (hit there when
`user/`'s own tracked source files all moved to `utilities/`). Fixed with
`@mkdir -p $O` before the recipe, `9c4c528`. **Before deleting the last
file in a per-fork directory (not just moving a source file elsewhere),
check `git ls-tree -r HEAD -- <dir>` for what else is tracked there, and
verify any build rule writing into it guards with `mkdir -p`.**

- **`kernel/init/user/init.c`** (`81d5802`) — thirteen of fourteen collapsed
  into one, based on `forks/riscv64`'s 54-line version (better error
  handling and orphan-reaping than the nine-fork 37-line majority). Only
  `amd64-jserv` kept its own, because it `mknod()`s a second device no other
  port has. Not yet wired into any fork's build — a content merge staged for
  the eventual `arch/<name>/` cutover, not a build change.
- **`tools/mkfs*.c`** — `mkfs.c`'s fourteen copies split into four
  on-disk-format families, unlike `init.c` each one **wired into its
  forks' Makefiles** (`../../tools/mkfs*.c` or `../../../tools/mkfs*.c`
  replaces each fork's own copy) and verified with a full rebuild, each
  fork's own full `test-<arch>`, `make test-all`, and
  `docker build --build-arg ARCH=<name>` for a representative fork of every
  distinct sub-Makefile shape touched:
  - `tools/mkfs.c` (`5d8aab0`) — `arm64`, `arm64-pi4`, `loongarch`,
    `riscv32`. Magic number + stored logstart/inodestart/bmapstart
    superblock.
  - `tools/mkfs-nomagic.c` (`03c7081`) — `amd64`, `i386`. Same stored
    superblock, no magic field.
  - `tools/mkfs-margincheck.c` (`03c7081`) — `arm-pi2`, `arm-pi3`. Nothing
    stored on disk (kernel recomputes via `i2b()`); a deliberately inflated
    `nblocks` plus a freeblock-margin build-time check neither other family
    has.
  - `tools/mkfs-fixedbudget.c` (`03c7081`) — `arm`, `arm-pi1`,
    `arm-pi1-bis`, `mips`. Same unstored superblock as margincheck, but a
    "hardcode a disk budget, assert it balances" algorithm instead of a
    dynamic one; the two per-fork literals it used to need (`nblocks`,
    `size`) turned out to already be formulas every copy computed by hand
    from values already in each fork's own headers, so the shared file
    computes both instead of hardcoding them.

  Two forks still have no shared file, each for a real reason recorded in
  the files' own header comments: `riscv64` (a deliberate one-block
  log-size bump making it incompatible with `tools/mkfs.c`'s other four)
  and `amd64-jserv` (an algorithm not close to any of the four clusters
  above). Forcing all fourteen (or even these last two) into one
  `#ifdef`-laden file was considered and rejected for the reason this
  plan's own "two hard rules" section gives — **for now**. The user's
  stated long-term goal is a single `tools/mkfs.c`; reaching it needs the
  on-disk superblock format itself unified first (add magic/stored fields
  to the ten non-conforming forks' `kernel/fs.h`, and change their
  `kernel/log.c`/`bio.c` to read them instead of recomputing from fixed
  macros) — real per-fork kernel changes on the scale of Phase 0, not a
  `tools/`-only change, and not started yet. Until then, these five files
  are the documented intermediate state, not a permanent design.

- **`tests/usertests*.c`** — the plan's own long-standing assumption that
  this file is "genuinely per-port" turned out to be only mostly true.
  Pairwise-diffing all fourteen forks' copies found two real clusters:
  - `tests/usertests-arm64.c` (`486dcbb`) — `arm64`, `arm64-pi4`,
    `loongarch`. Along the way, re-enabled `writebig` (previously disabled
    on `arm64`/`arm64-pi4` with a now-stale "disk almost full" comment -
    stale because `5d8aab0` had already unified their disk budget) and
    dropped `loongarch`'s own leftover debug `printf()`s.
  - `tests/usertests-pi.c` (`6cfaa72`) — `arm-pi1`, `arm-pi1-bis`,
    `arm-pi2`, `arm-pi3`. Investigating why `arm-pi3` skips `mem()`/
    `preempt()` by default led to a real, standalone kernel bug fix
    (`2e00f82` — `kernel/vm.c`'s `switchuvm()` did an O(n) cache flush on
    every `sbrk()` via `growproc()`, not a real process switch; see
    `notes_arch_arm_pi3.txt`'s own Bug 22). `mem()` is now shared
    unconditionally across all four; `preempt()` stays gated for
    `arm-pi3` alone (a real, still-unresolved 4-core issue, not fixed by
    the above) behind a `SKIP_PREEMPT_TEST` flag only that fork's own
    `user/Makefile` recipe defines.

  - `tests/usertests-x86.c` (`15935d9`) — `amd64`, `i386` (the "flat
    MIT-classic pair", already sharing `tools/mkfs-nomagic.c`). One real,
    irreducible difference kept behind `#ifndef __x86_64__`:
    `validateint()`'s 32-bit-only inline asm syscall-trap probe, a no-op
    on `amd64` already. Needed a genuine pointer-sized `uintp` type per
    fork (`i386`'s own `uint64` is 8 bytes despite 4-byte pointers there,
    so using it directly for pointer casts - safe on `amd64` only by
    coincidence - tripped `-Werror=pointer-to-int-cast`) - added as a
    direct per-fork typedef in each fork's own `kernel/types.h`, matching
    `amd64-jserv`'s own pre-existing `uintp`. `amd64-jserv` itself (76-120
    lines away) stayed out: missing two real test functions (`uio()`,
    `argptest()`) entirely, not just style.

  **`tests/usertests-jserv-mips.c`** (`08ba259`) — `amd64-jserv`, `mips`.
  Re-running `scripts/pairwise_diff.sh tests/usertests.c` after the three
  clusters above found the closest pair of any two forks left in the
  whole set: 99 lines apart, despite being unrelated ISAs (x86-64 and
  MIPS). Base taken from `forks/amd64-jserv` (it ran every sub-test,
  where `forks/mips` itself skips four). Same `NBIG`/`uintp` pattern as
  the other clusters, plus one new thing: `forks/amd64-jserv`'s own
  `validateint()` guard was `#ifndef X64` (a custom macro, true on any
  fork that never defines it, `mips` included) - replaced with the real
  GCC-predefined `#if defined(__i386__) && !defined(__x86_64__)`, since
  `mips`'s `.c` was carrying this 32-bit inline x86 asm fully commented
  out as raw text specifically because inline asm goes to the target
  assembler *verbatim* - x86 mnemonics reaching the MIPS assembler is a
  build failure, not a silent no-op like the equivalent `#ifdef` gets on
  amd64-jserv's own 64-bit build. `mips`'s four already-skipped
  sub-tests (`sbrktest`/`validatetest`/`exitwait`/`forktest`, all
  root-caused in `notes_arch_mips.txt`, not silently dropped) became
  `-DSKIP_*` flags on its own `usertests.o` recipe, the same shape as
  `usertests-pi.c`'s own `SKIP_PREEMPT_TEST`.

  **`tests/usertests-pi.c` renamed to `tests/usertests-arm32.c`**,
  widened to `arm`. Pairwise diff against `arm` looked huge (thousands of
  lines) against every other file, `riscv32`/`riscv64` included - but
  `diff -b -w` against `usertests-pi.c` specifically showed the real
  difference was indentation style alone (`arm`'s copy used 4-space
  indent, every other fork 2-space): same function set, same test
  coverage. Folded in with the existing `hi = 100*1024`/
  `SKIP_PREEMPT_TEST` behavior unchanged, and one real fix carried along:
  a stray `#include "traps.h"` (unused - `validateint()` here probes via
  `sleep(*p)`, not a raw trap) that `arm` has no such header for at all,
  dropped rather than worked around. Renamed because the file now covers
  every ARM32 port, not just the four Raspberry Pi ones.
  `riscv32`/`riscv64` remain real outliers (thousands of lines apart from
  every other fork, including each other) - not attempted. New tools:
  `scripts/debug_timing.py` (`95ceb85`) - per-checkpoint wall-clock timing
  for a `usertests` run, for telling "hung" apart from "just slow";
  `scripts/pairwise_diff.sh` (later commit) - the diff-line-count matrix
  generator used to find every cluster above, generalized from a
  hand-retyped bash loop into a reusable tool.

- **Tier 2 (headers) started: `include/kernel/syscall.h`** (`abf3382`) -
  eleven of fourteen forks, already byte-identical. First header merge
  since `include/core/types.h`/`include/user/user.h` (done in an earlier,
  undocumented session - see this file's own "Started" section above).
  Placement convention settled with the user this session: `include/core/`
  is for things that could be compiler builtins, independent of xv6 itself
  (raw C types); `include/kernel/` is for xv6's own kernel-ABI concepts
  used by *both* kernel and user code (syscall numbers, `open()` flags,
  `stat`/file-type constants); a header used by kernel code only would go
  in a top-level `kernel/` (matching `kernel/init/user/init.c`'s own
  precedent) - not yet exercised, no kernel-only header merged yet.

  **Gotcha 12: moving a header out of a directory it used to share with
  its own includer silently breaks a same-directory quoted-include.**
  Several forks' `kernel/initcode.S` did a bare `#include "syscall.h"`
  that needed no `-I` flag at all before, because quoted includes check
  the *including file's own directory* first and `syscall.h` used to sit
  right there. Once the shared content moved out, every such file needed
  the new `-I../../include/kernel` added to its own build flags too -
  easy to miss since nothing else in that file changed. Caught by a real
  build failure on `forks/arm`, then checked systematically (`grep -rl
  syscall.h forks/<name>/kernel/` per fork) rather than found one at a
  time. Combined with the already-known gotcha 9 (`ASFLAGS` is not
  `CFLAGS`, and some forks split it across *two* files - a top-level
  Makefile for kernel-side `.S` and a separate `user/Makefile` for the
  vpath-style forks' own `ulib`), a header move touches more build-flag
  surface than a `.c` file move does. **Before deleting a fork's last copy
  of a shared header, `grep -rl` every directory that header lived
  alongside for bare, same-directory includes of it - not just the files
  that already show up via a search for the header's own name from the
  repo root.**

  **Done: `include/kernel/fcntl.h`** (`baca96d`) — 8-fork cluster (`amd64`,
  `amd64-jserv`, `arm-pi1`, `arm-pi1-bis`, `arm-pi2`, `arm-pi3`, `i386`,
  `mips`). Simpler than syscall.h - only `kernel/sysfile.c` (an ordinary
  `.c` file) consumes it in any of the eight, so no gotcha 12 to worry
  about, just the same `-I` addition to each fork's `CFLAGS`.

  **Done: `include/kernel/stat.h`** (`3b94b79`) — 7-fork cluster (`amd64`,
  `arm-pi1`, `arm-pi1-bis`, `arm-pi2`, `arm-pi3`, `i386`, `mips` -
  `amd64-jserv` has its own distinct copy, not part of this one).

  **Gotcha 13: a shared header can have consumers outside every fork's
  own `kernel/` entirely - the host-built `mkfs` sources are exactly this,
  and `-idirafter` needs its own matching addition, not a plain `-I`.**
  All four `tools/mkfs*.c` variants `#include "stat.h"` (for `T_DIR`), a
  build rule with its own include flags completely separate from the
  fork's kernel-side `CFLAGS` - so `grep -rl <header> forks/<name>/kernel/`
  (gotcha 12's own advice) isn't enough; also check the already-shared
  `tools/*.c` and `tests/*.c` sources themselves. Worse: `forks/amd64`,
  `forks/i386` and `forks/mips`'s own mkfs rules use `-idirafter kernel`
  specifically so the HOST's real `<fcntl.h>` isn't shadowed by xv6's own
  (which lacks `O_TRUNC` under the old per-fork copy, and spells create
  `O_CREATE` not `O_CREAT` even in the new shared one) - adding a plain
  `-I../../include/kernel` there would have reintroduced exactly the bug
  `-idirafter` exists to prevent, since `mkfs.c`'s own `<fcntl.h>` is the
  *host's* header, not xv6's. Needed `-idirafter ../../include/kernel`
  instead, matching the existing `-idirafter kernel`'s own low-priority
  intent. And a genuine non-fix: `forks/arm-pi1`, `forks/arm-pi2` and
  `forks/arm-pi3`'s own mkfs builds needed no change at all, because each
  already carries its own separate, untouched `user/stat.h` (the Pi
  ports' long-known duplicated-header set - see this file's own
  "arm-pi1" section) that their `-iquote .` mkfs rule resolves against
  instead - checked byte-for-byte identical to the new shared content
  before trusting that, not assumed.

  **Done: `include/kernel/stat.h` widened to 13 of 14 forks** (`721bad2`)
  — folded the second `arm`/`arm64`/`arm64-pi4`/`loongarch`/`riscv32`/
  `riscv64` cluster into the same shared file rather than spinning off a
  second named variant (`stat-pi.h` or similar): all six were whitespace-
  only different from the existing 7-fork content, confirmed by diff
  before merging. `amd64-jserv` alone keeps its own copy - real extra
  `ownerid`/`groupid`/`mode` fields its `chmod.c` actually uses, not a
  reconcilable difference. One shared header, one fork-local exception -
  not "a cluster per near-identical variant."

  **Two real Docker CI regressions found in the *already-merged*
  `fcntl.h`/`syscall.h` commits** (`c329deb`), caught by the user reading
  a live GitHub Actions run rather than by anything local - `make
  test-all`/`test-<arch>` reuse `.o`/`.d` artifacts that predate a header
  move and never notice it went stale. Both are variants of gotcha 12/13's
  own lesson, one layer deeper each:
  - `forks/arm`'s `user/Makefile` and `tools/Makefile` compile one
    directory deeper than `makefile.inc`'s own `-I../../include/kernel`
    was scoped for, so the flag silently resolved to nothing at that depth
    (gcc treats a missing `-I` dir as a no-op, not an error) - any bare
    `#include "syscall.h"`/`"fcntl.h"` fell through to the HOST's own
    `/usr/include/syscall.h` inside the Docker image (present there,
    apparently absent on this dev host, which is exactly why it never
    showed up locally). Needed the correctly-depthed path added directly
    to each of those two files, not just `makefile.inc`.
  - `forks/amd64-jserv` keeps its own `stat.h`/`syscall.h` but shares
    `fcntl.h`; its main Makefile's `-I../../include/kernel` addition
    landed *earlier* in the CFLAGS flag order than its own `-Ikernel`, so
    gcc's first-match search let the shared header win for `stat.h` too -
    `chmod.c`'s `st.mode` access stopped compiling. **Flag ORDER matters
    when a fork keeps some of its own headers and shares others under the
    same directory name** - moving `-I../../include/kernel` after
    `-Ikernel` let the fork's own copies win while still falling through
    for the one file it doesn't have locally.

  **Gotcha 14: a fork's own `test-xv6.py` can silently lose a
  `TOOLPREFIX` override if it only relies on the environment.**
  `forks/riscv32` has no real `riscv32-unknown-elf-` toolchain installed
  (this host's `TOOLPREFIX_RISCV32` override points at `riscv64-unknown-
  elf-` instead - see `Makefile.config`), and its own `test-xv6.py` calls
  `["make", "fs.img"]` with no explicit `TOOLPREFIX=`, relying on it being
  inherited via the environment. But `forks/riscv32/Makefile` sets
  `TOOLPREFIX = riscv32-unknown-elf-` with a plain `=`, which *overrides*
  an inherited environment value - only a `make` command-line argument
  beats it. This stayed invisible for as long as nothing forced a rebuild
  inside that specific subprocess call (an up-to-date target never
  touches `CC`), until the `stat.h` merge's own mtime change made `make
  fs.img` legitimately want to rebuild `user/cat.o` and immediately hit
  the nonexistent compiler. Fixed by forwarding `TOOLPREFIX` explicitly
  on both `reset_cmds`. Any other fork whose installed toolchain name
  differs from its own Makefile's hardcoded default has this same latent
  exposure - `riscv32` is simply the only one where those two names
  genuinely differ right now.

  **Also restored `riscv64`'s own usertests timeout to 600s**, undoing
  `647cc47`'s blanket 600→300 drop for this one fork specifically: its
  own full run (through the `diskfull`/`outofinodes` disk-stress
  subtests) measures ~570s wall-clock here, matching CI's own ~8m12s job
  time, and a passing-but-still-running process at 300s looked identical
  to a hang (the harness only prints *completed* lines, so an in-flight
  `test diskfull: ` was invisible). Nothing about `riscv64` changed -
  `647cc47`'s own measurements simply didn't cover it.

  **Done: `include/elf.h`, 13 of 14 forks** (`3e95e33`). Unlike
  `stat.h`/`fcntl.h`/`syscall.h`, this one has a genuine structural split,
  not per-fork drift: ELF32 and ELF64 are different on-disk formats
  (`e_entry`/`e_phoff`/`e_shoff` are 32- vs 64-bit; `Elf64_Phdr` also moves
  `p_flags` right after `p_type`, where `Elf32_Phdr` has it between
  `p_memsz` and `p_align`). Rather than one struct behind a bitness macro
  (which would need a `uintp`/`X64`-style discriminator this repo doesn't
  have on every fork yet - see the postponed uintp/uintptr reorg idea),
  the shared header declares two named struct pairs unconditionally -
  `elf32hdr`/`proghdr32` and `elf64hdr`/`proghdr64` - and each fork's own
  `exec.c`/`bootmain.c` just references whichever pair matches its own
  word size. No macros, no conditional compilation, no Makefile changes
  anywhere (every fork's kernel build already carries `-I../../include`).
  `amd64-jserv` keeps its own copy for the same *kind* of real reason
  `stat.h` has one exception too: it's genuinely dual-mode (32/64-bit via
  its own `uintp` typedef and `#if X64`), not an oversight.

  **Done: `spinlock.h`, all 14 forks, as two files** (`d2d4759`,
  `91c9300`) — a genuine two-way split, not per-fork drift: 9 forks keep
  a debug `pcs[10]` call-stack array (`kernel/spinlock.h`, using `uintp`
  for the array width like `elf.h`/`usertests-x86.c` already do), 5 drop
  it entirely in the newer MIT layout (`kernel/nopcs/spinlock.h` - the
  first kernel-only header, placed at top-level `kernel/` rather than
  `include/kernel/`).

  **Done: `include/arch/<arch>/arch.h`, all 14 forks** (`5f08922`) - the
  `uint64`/`uintp` typedefs this tier kept re-adding by hand per fork
  finally got their own home, one per ISA (`arch/arm/arch.h` covers all
  five ARM32 ports, `arch/arm64/arch.h` covers `arm64`+`arm64-pi4`).
  `core/types.h`'s own header comment had named this design since it was
  written - "the per-arch specific are in `include/arch/<arch>/u.h`" -
  but nobody had followed through on it until this pass forced the
  question by needing `uintp` in enough places at once. Spelled `arch.h`,
  not `u.h` (less cryptic), and included as bare `#include <arch.h>` with
  angle brackets - each fork's own Makefile supplies exactly one
  `-I .../include/arch/<arch>`, so the same include line resolves
  correctly everywhere, matching Plan9's own `<u.h>`. `amd64-jserv` keeps
  its inline `uint64`/`uintp`, its usual dual-mode exception.

  **Done: `include/kernel/fs.h`, all 14 forks** (`dcdfb3a`, `d09b3fb`,
  `bff1e71`, `e0c3d5b`, `361bb58`) - the last Tier 2 header, and unlike
  every other one it turned into a real format unification rather than
  a byte-identical merge. Two independent axes of drift, found via
  `scripts/pairwise_diff.sh kernel/fs.h`:

  - **Stored vs. computed superblock.** 7 forks (`amd64`, `i386`,
    `arm64`, `arm64-pi4`, `loongarch`, `riscv32`, `riscv64`) already
    wrote `logstart`/`inodestart`/`bmapstart` into the on-disk
    superblock and had the kernel read them back; the other 7
    (`amd64-jserv`, `arm-pi2`, `arm-pi3`, `arm`, `arm-pi1`,
    `arm-pi1-bis`, `mips`) stored only `size`/`nblocks`/`ninodes`/`nlog`
    and hardcoded the layout offsets (`+2`, `+3`) directly into both
    `IBLOCK`/`BBLOCK` and each fork's own `mkfs` - the exact fragility
    (two copies of the same constant, nothing enforcing agreement, no
    magic number to catch a mismatch) that `tools/mkfs-margincheck.c`'s
    own freeblock-margin assert existed to paper over. Converted all 7
    to the stored form: `IBLOCK`/`BBLOCK` switched to their 2-arg
    (`sb`-taking) form, `iupdate()`/`ilock()` gained a local
    `struct superblock sb; readsb(...)` (the only two call sites with
    no superblock already in scope), `log.c`'s `log.start` now reads
    `sb.logstart` instead of recomputing `sb.size - sb.nlog`, and their
    own `mkfs` (`amd64-jserv`'s standalone copy,
    `tools/mkfs-margincheck.c`, `tools/mkfs-fixedbudget.c`) gained the
    matching writes. Each fork's own disk-budget *algorithm* - dynamic,
    margincheck's inflated-`nblocks`-plus-assert, or fixedbudget's
    hardcode-and-assert - was left untouched; only the format the
    algorithm produces changed.
  - **The dinode itself.** `amd64-jserv` was the only fork with
    `ownerid`/`groupid`/`mode` fields (for its own `chmod()`/`chown()`).
    Rather than keep it as fs.h's usual dual-mode exception, those
    fields became universal - every other fork carries them zeroed and
    unused (`mkfs` always `bzero()`s a fresh dinode first, so this cost
    nothing) rather than forking the format over one feature. That
    pushed the shared `dinode` to 256 bytes, which forced `NDIRECT`
    from each fork's own tuned value (12, 28, 58, 60) down to a single
    58 everywhere - the value that keeps `dinode` size a power of two
    (required for `BSIZE % sizeof(dinode) == 0`) with the new field
    layout, at both `BSIZE` values in the tree. Free side effect:
    `arm` had never received the NDIRECT-bump fix its four siblings
    got for the same "usertests binary barely fits in MAXFILE" bug
    (`notes_arch_arm_pi3.txt` etc.) - unifying to 58 fixed it too,
    without a separate investigation.

  `BSIZE` itself stays genuinely per-fork (coupled to each fork's own
  disk driver - `amd64-jserv/kernel/fs.h`'s own history already proved
  changing it silently breaks `ide.c`'s `sector_per_block` assumption)
  and, per the user's own correction mid-session, does NOT belong in
  `arch.h` (general per-arch C types, not a filesystem tuning knob) -
  it moved to a new one-line fork-local `kernel/conf.h` instead, which
  the shared `fs.h` `#include`s bare. Two build-flag gotchas came up
  applying this to headers for the first time: `-Ikernel` needed adding
  to several forks' kernel CFLAGS (gotcha 12 pattern - the shared
  header's own nested include needs the fork-local dir on the search
  path) and `-idirafter kernel` (never plain `-I`) needed adding to
  every fork's own `mkfs` recipe specifically, to avoid shadowing the
  HOST's real `<fcntl.h>` (gotcha 13's exact lesson, hit fresh on a new
  header). `amd64` and `i386` also needed `FSSIZE` doubled to 2000
  (`dcdfb3a`) - the bigger dinode nearly quadrupled their inode-region
  block cost (`IPB` 8 -> 2) and a real run hit "balloc: out of blocks"
  during "big files test" at the old 1000, the identical regression
  `amd64-jserv/kernel/param.h` had already diagnosed and fixed once
  before.

  **Done: `tools/mkfs*.c` collapsed to one shared file, 12 of 14 forks**
  (`4e5c15a`) - the very next session, at the user's prompting ("can
  probably factorize those mkfs-xxx.c to a single one?"). The four
  on-disk-format families turned out to differ, once fs.h was unified,
  only in whether `nblocks` was computed top-down from FSSIZE
  (`tools/mkfs.c`'s own approach) or hardcoded bottom-up with `size`
  derived afterward (`mkfs-margincheck.c`/`mkfs-fixedbudget.c`) -  the
  six forks using the second style had no FSSIZE in their own
  `kernel/param.h` at all. Verified algebraically before touching
  anything that `nblocks = FSSIZE - nmeta` reproduces the exact same
  `nblocks` these forks already shipped (their own hardcoded formulas
  all reduce to a fixed total independent of each fork's own LOGSIZE),
  then added an explicit FSSIZE to each using that total.

  `mkfs-margincheck.c`'s own freeblock-margin safety assert (fail the
  BUILD if packing an image leaves under 2×MAXFILE data blocks free -
  see this file's own `arm-pi2` writeup above) was promoted to run for
  every fork sharing the file, not just the two that first hit that
  bug. Running it for the first time on ten more forks immediately
  found two more real, previously invisible thin margins - `amd64`/
  `i386` (fixed in the fs.h commit itself) and `arm64`/`arm64-pi4`/
  `loongarch`/`riscv32`/`arm-pi1`/`arm-pi1-bis`/`mips` here, each
  needing `FSSIZE` raised.

  **Gotcha 17, a real boot-time regression the build never caught:**
  `forks/arm`'s own `kernel/start.c` embeds `fs.img` directly into the
  kernel ELF (`-b binary`-style) and has a genuine, tight physical-
  memory ceiling in the first 1MB (`vectbl`) that a too-large `FSSIZE`
  silently overruns - the kernel still *builds* fine, but panics
  ("empty mark in the list" in `kpt_freerange`) before any filesystem
  code runs, on every boot, not flakily. A first attempt raised `arm`
  to 1500 (matching the arm64 family, which has far more headroom) and
  broke it outright; `arm` itself needed no raise at all (already
  comfortable at its implicit 1099), and `arm-pi1-bis` - the other fork
  with this same fs.img-in-kernel-ELF construction - was raised by
  much less (1250) and boot-tested at that exact value rather than
  trusted from a successful build alone. **A margin-check or FSSIZE
  change is not verified by `make build-<arch>` succeeding - it must
  actually boot**, exactly the lesson gotcha 13 already taught about
  host-header shadowing, now recurring one layer up at the disk-image
  level.

  Net effect: two standalone copies left (`amd64-jserv`'s real, used
  `ownerid`/`groupid`/`mode` dinode fields with its own `ialloc()`
  default-assignment logic; `riscv64`'s deliberate `nlog = LOGBLOCKS +
  1` headroom bump), one shared `tools/mkfs.c` for the other twelve.
  Verified with `build-all`, `test-all`, a full `stress-test-all`, and
  `docker build --build-arg ARCH=<name>` for every fork with CI
  coverage.

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
**All done as of 2026-09-11** (`fs.h` last, see the "Started" section above
for its own fuller writeup - it was the one header that turned into a real
on-disk format unification, not a byte-identical merge).

**Tier 3 — common kernel.** `fs.c`, `log.c`, `bio.c`, `pipe.c`, `file.c`,
`string.c`, and the arch-independent half of `syscall.c`. Genuinely shared
logic, but with real per-arch drift accumulated over a decade.

**Checklist for picking and executing the next Tier 3 file (the user's
own criteria, 2026-09-12 - the sections right below spell out the full
reasoning and the real bugs each one caught; this is the fast-reference
version):**

1. **Mostly portable first.** Before committing to a file, check how
   much of it is genuinely arch-independent vs. really arch-coupled.
   `pipe.c`/`file.c` only touched `copyin()`/`copyout()`/`pagetable_t`;
   `syscall.c` touches trapframe registers directly in nearly every
   function (`p->trapframe->x0`..`x7` on arm64) - too coupled, skipped
   for now.
2. **Baseline = the most advanced implementation, not just the
   biggest existing cluster.** Don't merge whichever forks already
   happen to match each other - check whether some other fork (even a
   singleton, even outside the cluster) already does it better, and
   converge toward *that*. Diffing `file.c`'s own arm64-based merge
   against `riscv64` (not in that cluster) found two real gaps this
   way: a missing `n<0` guard, and a `copyout()` missing bounds/write-
   protection checks.
3. **Simplest or most modern implementation as the tiebreaker** when
   there is no clearly "most advanced" one.
4. **Real arch differences become portable interfaces, not permanent
   forks.** A named type/function (`pagetable_t`, `uintp`, eventually
   `copyin`/`copyout`/`myproc`) goes in a `kernel/arch/<arch>/arch_vm.h`-
   style file, implemented differently per arch, so portable code in
   `kernel/` calls it uniformly - matching the user's own
   `~/principia/kernel/` (its `portfns_`/`portdat_`/`dat_` convention,
   same idea, different names here). **A constant that happens to be
   identical across every current fork can still be a genuine
   "interface" worth keeping per-arch, not a candidate for hoisting
   into one shared literal** - the user's own example: `PGSIZE` is 4096
   on the 13 forks that define it at all, but that is a fact about
   today's hardware choices, not a portability guarantee; a future port
   with a different native page size would need to redefine it, the
   same way `uintp` already varies by word size. (`arm` is the 14th -
   checked, and it turns out to have no `PGSIZE` anywhere in its own
   tree at all: it uses a completely different buddy allocator,
   `kernel/buddy.c`, not the classic free-list `kalloc.c` this
   candidate file is about - a real, separate reason it won't join this
   particular merge, not an oversight.) Define `PGSIZE` per-arch in
   `kernel/arch/<arch>/arch_vm.h` (VM-scoped, alongside `pagetable_t`/
   `pte_t` - not the general `arch.h`) even where every arch's current
   definition is the literal same line - the point of an interface is
   the named contract, not
   whether today's implementations happen to agree.
5. **But not everything gets an interface.** Code with no counterpart
   in any other fork (real Tier 4 stuff - MMU walk internals, boot
   sequences, interrupt controllers) stays local; `arch_vm.h` is only
   for types/functions implementing a genuine cross-arch contract, not
   a promotion path for arbitrary per-fork content.
6. **Prefer the easier file when a candidate reveals deep, costly
   work.** Postpone rather than force it - this is how the session
   went `bio.c` (needs a `struct buf` unification + a new `disk_rw()`
   shim + a `log.c` edit) -> `file.c` (quick, reused existing
   infrastructure) -> `sleeplock.c` (turned out to be the biggest win
   yet, once actually checked).
7. **Verify by building, not by trusting a diff.** Concretely: check a
   file actually *exists* in a fork before trusting a "0 lines
   different" (`diff` against a missing file with stderr suppressed
   produces empty stdout, which `wc -l` happily reports as 0 - this
   produced a false 13-fork match for `sleeplock.c` that was really 6);
   check whether an include is *actually* dead by looking for every
   symbol it might provide, not just the ones the file's own body
   references directly (`pagetable_t`/`pte_t`/`struct spinlock` were
   each pulled in transitively through `defs.h`/`proc.h`, not used
   directly by the file being merged - grep-for-direct-use missed all
   three); and always build + boot-test before trusting either check.

**Working method, settled with the user 2026-09-12 (apply to every
remaining Tier 3 file):** don't just look for forks that already happen
to match. For each file: (1) read it in a few candidate forks and judge
how much of it is genuinely portable vs. really arch-coupled (the
`pipe.c` work found real couples like this - a `pagetable_t`/`copyin()`
memory-access model, not just naming); (2) pick ONE baseline - the
simplest or most modern/canonical implementation, not necessarily the
current majority - as the shared file's target content; (3) migrate
other forks toward that baseline deliberately, editing their real
differences away (a rename, a control-flow reshape, a missing type
factored out to `kernel/arch/<arch>/arch_vm.h`) rather than only merging
the forks that were already identical. This is slower and touches more
real code per file than the pure byte-identical merges below, but is
the actual point of Tier 3 - "genuinely shared logic... with real
per-arch drift", not just administrative dedup of accidental
duplicates. Verify every migrated fork exactly as everywhere else in
this plan: build, full boot-test, `docker build` where it applies.

**Refinement, same conversation: `kernel/arch/<arch>/kernel.h` is for
*interfaces*, not just types - which makes step (3) above collapse
splits that otherwise look permanent.** `uintp` is the existing proof
this already works in this repo: it names one contract ("the pointer-
sized integer"), each fork's own `arch.h` implements it differently
(`uint64` on 64-bit ports, plain `uint` on i386), and because shared
code casts through `uintp` instead of hardcoding a width, that code is
portable despite the real representation differing per arch. Extend
the identical idea from types to *functions*. The `pipe.c` "memory-
access model split" this session found and left unreconciled
(`arm64`'s own `copyin()`/`copyout()`, page-table-aware, vs `amd64`'s
own direct pointer dereference) is not actually two irreconcilable
implementations - it is one missing interface: `amd64`/`i386`/`mips`/
`amd64-jserv` have no `copyin()`/`copyout()` at all, only because they
have no separate user/kernel address space to translate through - a
*trivial* implementation (a straight `memmove`) is a perfectly valid
backend for that interface on those forks. Once every fork provides
`copyin()`/`copyout()` (real page-table walk on some, a bare `memmove`
on others) under a name shared kernel code can always call, `pipe.c`
stops needing two clusters at all. Same likely story for `myproc()`:
`amd64-jserv`/`mips` use a raw global `proc` instead, which is just a
zero-effort `#define myproc() (proc)` away from being the same
interface everyone else already calls. **Before accepting a split as
"a real arch difference, not naming" (as this session did for
`pipe.c`), check whether it is actually a missing interface
implementation instead** - the fork lacking the interface is usually
the trivial case, not the hard one, since it means that fork's own
hardware/memory model doesn't need to do the real work at all.

**Naming settled, same conversation: `kernel/vm.h` (portable interface,
not written yet) / `kernel/arch/<arch>/arch_vm.h` (per-arch types
implementing it) - xv6-style names, not `~/principia/kernel`'s own
`portfns_`/`portdat_`/`dat_` prefixes (that repo's own equivalent
convention, considered and explicitly not copied verbatim: same idea,
adapted naming).** `kernel/arch/arm64/kernel.h` (this session's first
name for the `pagetable_t`/`pte_t` file) got renamed to `arch_vm.h`
before anything else consumed the old name, for a real reason: a bare
`#include "vm.h"` from a fork whose own build has *both*
`kernel/`-the-portable-directory and `kernel/arch/<arch>/`-the-per-arch-
one on its `-I` list would be genuinely ambiguous, resolved by search
order rather than intent - not a hypothetical, exactly the kind of
subtle include-resolution bug this same session already chased for
`pipe.c`'s own `pagetable_t`. `arch_vm.h` can't collide with a future
portable `vm.h` no matter the search order.

**Scope discipline for `arch_vm.h`, stated explicitly so it doesn't
drift: it holds types that implement a genuine cross-arch interface
(something a *portable* `kernel/*.c` file depends on, like
`copyin()`/`copyout()`'s own `pagetable_t` parameter) - not "any type
this fork happens to need that's arch-specific."** Plenty of real Tier
4 code (MMU walk internals, boot sequences, interrupt controllers) is
irreducibly arch-specific with no counterpart in any other fork at all
- that stays in the fork's own local header (`aarch64.h`, `riscv.h`,
`loongarch.h`, ...) exactly as before. `arch_vm.h` is for the specific
case where multiple forks *do* implement the same conceptual operation
differently, not a promotion path for arbitrary per-fork content.

**Started (2026-09-11/12).** `scripts/pairwise_diff.sh` against these six
files immediately found a real 9-vs-5 split hiding under them: 9 forks
(`amd64`, `i386`, `arm64`, `arm64-pi4`, `loongarch`, `riscv32`, `riscv64`,
`mips`, `amd64-jserv`) use the newer `begin_op()`/`end_op()` concurrent-
transaction log design; 5 (`arm`, `arm-pi1`, `arm-pi1-bis`, `arm-pi2`,
`arm-pi3`) use the older `begin_trans()`/`commit_trans()` single-
transaction one - the same shape as `kernel/spinlock.h`'s own pcs/nopcs
split, just discovered a tier later than expected.

- **Placement strategy change: symlinks, not deleted-file-plus-new--I-
  flags.** For anything genuinely byte-identical (or made so by a small
  fix), the user's own correction this session: move the canonical copy
  to a shared location and replace each fork's own copy with a relative
  symlink, rather than deleting it and threading `-idirafter`/`-Ikernel`/
  `-iquote` through every consuming Makefile the way the fs.h/mkfs.c work
  needed. A symlink keeps the file at the exact path each fork's own
  build rules already look for it at, so zero Makefile lines change - the
  fs.h-style flag surgery is only actually necessary when a file has
  per-fork *content* variation (needing a `conf.h`-style per-fork
  include) or is host-compiled (mkfs's own `-idirafter`-to-avoid-
  shadowing-`<fcntl.h>` concern). Confirmed working through a real
  `docker build` (symlinks survive `COPY` and resolve correctly there
  too).
- **`kernel/legacy/{string,pipe,file,bio,log,fs}.c`** - the 4-fork ARM32
  Pi cluster (`arm-pi1`/`arm-pi1-bis`/`arm-pi2`/`arm-pi3`; `arm` itself
  isn't byte-identical to its siblings for these, despite sharing the
  same `begin_trans` log design) was byte-identical across all six
  files. Named `legacy/` rather than the plain names, on the same
  majority-gets-the-plain-name principle as `nopcs/spinlock.h`: this is
  the 5-fork minority, and the plain `kernel/{string,...}.c` names are
  worth keeping free for the 9-fork `begin_op` majority to grow into
  rather than being squatted on by the smaller group first.

  **Later (2026-09-12): the `legacy/` directory itself was retired**,
  each file flattened to a `kernel/<name>-legacy.c` sibling instead
  (`string.c` first, as part of its own three-way split above; then
  `bio`/`file`/`fs`/`log`/`pipe` together, pure `git mv`, no content
  change, `git blame -C -C` reverified on all five). Reason: `-legacy`
  turned out to be a *design-generation* label, not an ARM-specific
  one - confirmed while investigating `bio.c`'s own next-candidate
  split, where `mips`'s own `bio.c` turned out to be only ~14 lines
  from this cluster's `iderw()`/`B_BUSY`-polling design (no relation to
  ARM at all). A `kernel/legacy/` *directory* implies a fixed member
  set; a flat `-legacy` suffix says "this design generation" and stays
  accurate as more non-ARM forks join it later - matching
  `kernel/sysproc-legacy.c`'s own naming, which prompted the question.
  `kernel/string-arm.c`/`string-x86.c` were deliberately NOT touched by
  this same reasoning: those really are permanent, ISA-specific
  hardware optimizations (`memsetw`/`memsetb` vs `stosl`/`stosb`), not
  a generation gap, so the ISA-named suffix stays correct forever.
- **`kernel/string.c`** - the x86 family (`amd64`, `i386`, `amd64-jserv`),
  reconciled with one real one-line fix rather than a pure move: a
  pointer-size cast in `memset()`'s alignment check was hardcoded per
  fork (`(uint64)dst` / `(int)dst`); `amd64-jserv` already spelled it
  `(uintp)dst` (this repo's own per-arch pointer-sized typedef, from the
  `arch.h` work), which turned out to make all three byte-identical once
  applied everywhere. Kept separate from the wider `begin_op` cluster:
  all three `#include "x86.h"` for a `stosl`/`stosb` fast path the other
  ISAs have no equivalent of.
- **`kernel/pipe.c`** - `arm64`/`arm64-pi4`/`loongarch`, byte-identical
  modulo one include (`"aarch64.h"`/`"loongarch.h"`) that looked dead
  (no `cpuid()`/`mycpu()` calls) but wasn't - it was the only source of
  `pagetable_t`/`pte_t` for `defs.h`'s own `copyin()`/`copyout()`
  declarations. Dropping it as unused broke all three builds; caught by
  actually building, not by trusting the diff. Fixed properly rather
  than re-adding the per-fork include: new
  `kernel/arch/<arch>/arch_vm.h`, one directory per ISA mirroring
  `include/arch/<arch>/arch.h`'s own shape, but for kernel-only,
  VM-specific per-arch types - `arch.h` is for general types a user
  program could conceivably need too (the user's own correction: don't
  let kernel-internal VM types accumulate there). Named `arch_vm.h`,
  not the session's first try `kernel.h` (too generic - a different
  subsystem needing the same treatment gets its own file, not a
  shared catch-all) or plain `vm.h` (would collide with a future
  portable `kernel/vm.h` on the same fork's own `-I` list - see the
  "Naming settled" note above). Included as quoted `"arch_vm.h"`, not
  angled `<arch_vm.h>` like `arch.h` - angle brackets stay for the more
  universal, could-apply-to-any-project Plan9-`u.h` convention `arch.h`
  represents; this one is specific to how this repo itself organizes
  its own kernel internals. Scope discipline: only types genuinely
  implementing a cross-arch interface belong here (see the "Scope
  discipline" note above) - not a promotion path for arbitrary
  per-fork content. `riscv32`/`riscv64` use the same `copyin`/`copyout`
  model for `pipe.c` but have real control-flow differences (not just
  naming) - left for a future pass.
- Still open in the `begin_op` cluster: `file.c`, `bio.c`, `log.c` (the
  actual transaction-log logic itself - the deepest, riskiest merge in
  this tier) and the `amd64`/`i386` + `amd64-jserv`/`mips` sub-pairs of
  `pipe.c`/`file.c`, which use a different, real memory-access model
  (`addr` as an already-kernel-accessible pointer, vs `arm64`'s own
  page-table-aware `copyin`/`copyout`) - a genuine per-arch semantic
  split (CLAUDE.md's own "if a function body differs, it belongs in
  arch/<name>/"), not a naming difference, and not yet reconciled.
- **`kernel/sleeplock.c`** (`8627b0c`) - the biggest win in this tier so
  far, and found by applying the checklist above rather than starting
  from a pairwise-diff cluster: `sleeplock.c` only exists in 7 of the
  14 forks at all (the other 7 use plain spinlocks for buffers, no
  sleeplock concept), and 6 of those 7 (`amd64`, `i386`, `arm64`,
  `arm64-pi4`, `loongarch`, `riscv32`) reduce to the exact same code
  once each fork's own dead per-arch include is dropped - `riscv64`
  (the 7th) has one real difference, a `sleep_prepare()`/`sleep()`
  split instead of a single `sleep()` call, left out for a future pass.
  Two real gotchas, both already folded into the checklist above:
  (1) an early pairwise check reported a false "0 lines different" for
  the 7 forks that don't have the file at all - `diff` against a
  missing path produces empty stdout, and a stray `2>/dev/null` hid the
  real error, so `wc -l` happily reported 0; (2) dropping the "looked
  dead" per-arch include broke `defs.h`/`proc.h`'s own transitive
  reliance on `pagetable_t` and `struct spinlock` (never checked before,
  because it always worked) - fixed by making both files self-contained
  rather than restoring the per-fork include, which in turn surfaced a
  *third*, adjacent bug: `kernel/spinlock.h`, `kernel/nopcs/spinlock.h`,
  the `arch_vm.h` files, and `amd64`/`i386`'s own `kernel/mmu.h` had no
  include guards at all, harmless until a header started including
  another header twice in the same translation unit. All four now
  guarded. `riscv32` gained its own `kernel/arch/riscv32/arch_vm.h` -
  the first fork outside the `pipe.c`/`file.c` cluster to need one.
- **`kernel/kalloc.c`** (2026-09-12) - 4 of the 5 candidate forks
  (`arm64`, `arm64-pi4`, `riscv32`, `riscv64`); `loongarch` deliberately
  left out, see its own finding below. Confirmed criterion 1 "mostly
  portable": free-list allocator, spinlock-protected, no register/
  trapframe access. Two real per-arch splits, both resolved as
  interfaces rather than left as forks, per criterion 4:
  - `P2V`/`V2P`: `arm64`/`arm64-pi4` have a genuine higher-half kernel
    (`KERNBASE = 0xffffff8000000000`), so `kfree()`'s bounds check
    needs `P2V(PHYSTOP)`, not bare `PHYSTOP`. `riscv64`/`riscv32` are
    identity-mapped (`KERNBASE = 0x80000000`, the real physical base)
    and had no `P2V`/`V2P` macros at all - given a trivial `#define
    P2V(a) (a)` / `#define V2P(a) (a)` identity backend in their own
    `memlayout.h` (same pattern as this repo's `copyin()`/`copyout()`
    on the forks with no separate address space), so the shared file's
    `P2V(PHYSTOP)` call works unchanged on both kernel-address models.
  - `PGSIZE`/`PGSHIFT`/`PGROUNDUP`/`PGROUNDDOWN`: moved into each
    fork's own `kernel/arch/<arch>/arch_vm.h`, exactly as this plan's
    own criterion 4 discussion anticipated - `riscv64` needed a new
    `arch_vm.h` (the first of the three riscv64/riscv32/arm64-pi4-
    sharing-arm64 forks in this cluster to get one; `riscv32`/`arm64`
    already had one from the `pipe.c`/`file.c` work and just gained
    these four macros). Left duplicated in each fork's own big
    per-arch register header (`riscv.h`/`aarch64.h`) rather than
    migrated out of it - same already-accepted pattern as `pagetable_t`/
    `pte_t` there (identical-type re-typedefs and identical macro
    redefinitions in *different* translation units are not conflicts;
    kalloc.c only ever includes `arch_vm.h`, never the big header, so
    nothing is doubly defined in any one file). `uintp` (already
    established for the amd64/i386/mips/arm x86-ish cluster) added to
    `riscv64`/`riscv32`/`arm64`'s own `include/arch/<arch>/arch.h` for
    the same `PGROUNDUP((uintp)pa)` cast this file needs; `riscv64`'s
    Makefile gained the matching `-I../../kernel/arch/riscv64` (gotcha
    12's own lesson - a new shared-header consumer needs the same `-I`
    every existing one already has).

  One real behavior migrated to the baseline, not just moved:
  `arm64-pi4`'s own `kalloc()` filled freed-then-reallocated pages with
  `0` instead of the `5` every other fork in this cluster (including
  its own sibling `arm64`) uses - `5` is deliberately non-zero junk, to
  make a bug that reads memory it should have initialized itself crash
  loudly instead of silently seeing zeros; `0` defeats that purpose.
  Converged to `5` (the baseline every other fork already agreed on),
  per criterion 2.

  kinit()-family entry points (the fork's own boot sequence: `riscv64`/
  `riscv32` call a single no-arg `kinit()`, `arm64`/`arm64-pi4` call
  `kinit1(vstart,vend)` then `kinit2(vstart,vend)` as more of physical
  RAM becomes mapped) are genuinely per-arch, but all three are cheap,
  thin wrappers around the same shared `freerange()`/`kfree()`/
  `kalloc()` - so all three now live in the one shared file rather than
  splitting the file along that boundary; whichever ones a given fork's
  own `main.c` doesn't call simply go unused (harmless for a non-static
  extern-linkage function).

  **Finding, not fixed: `loongarch`'s own `kinit()` frees from
  `RAMBASE`, not from `end` (the first address after the kernel's own
  image) - every other fork in every cluster checked frees only
  `[end, PHYSTOP)`.** `RAMBASE` here is a DMW-mapped virtual alias for
  physical `0x90000000`, and the kernel itself loads at `RAMBASE +
  0x200000` - so `freerange(RAMBASE, RAMSTOP)` walks straight through
  the kernel's own text/data/bss and `kfree()`s them, since its bounds
  check (`pa < RAMBASE || pa >= RAMSTOP`) has no case that excludes the
  kernel's own image in between. Likely survives today only because
  `kalloc()`'s free list is LIFO over an ascending `freerange()` walk,
  so the highest addresses (near `RAMSTOP`, well above the kernel) are
  served first - a `usertests` run that never exhausts memory down to
  the kernel's own reused pages would never observe corruption. Not
  touched here: this is a real, pre-existing bug candidate, not a
  factorization concern, and unifying `loongarch` into this cluster
  would have meant silently changing its behavior (from `RAMBASE` to
  `end`) inside what should have been a pure move - exactly what this
  plan's own "do not clean up while you're in there" rule forbids.
  Worth its own investigation and fix, separately.

- **`kernel/log.c`** (2026-09-12) - 5 forks (`arm64`, `arm64-pi4`,
  `loongarch`, `riscv32`, `riscv64`). Baseline picked by criterion 2, not
  by which forks already matched: `arm64` (`amd64`/`i386` were the
  larger already-0-diff pair, but diffing them against `arm64` found
  `arm64` fixes two real things `amd64`/`i386` don't - `bpin()`/
  `bunpin()` refcount-based buffer pinning instead of a `B_DIRTY` flag
  (needs `bio.c`'s own `bpin`/`bunpin`, which `amd64`/`i386`/
  `amd64-jserv`/`mips` don't have - the same "struct buf differs by
  family" gap that still blocks `bio.c` itself, see below), and
  `log_write()` acquiring `log.lock` *before* reading `log.outstanding`/
  `log.lh.n` rather than after - the latter is a real data race on the
  unlocked read). `riscv64` (79-97 lines from the other four - the
  biggest gap in the cluster) turned out to be a *further* advance on
  top of `arm64`'s baseline, once actually read rather than assumed to
  be a bigger fork: a lost-wakeup fix (see the `sleep_release()`
  interface below) plus a tested `sys_sync()` syscall - both kept.

  Two real per-arch gaps resolved as interfaces, matching this file's
  own established method:
  - **`sleep_release(chan, lk)`** (new, `kernel/arch/<arch>/arch_proc.h`)
    - `arm64`/`arm64-pi4`/`loongarch`/`riscv32` all have a single
    `sleep(chan, lk)` that does register+release+block+reacquire
    atomically; `riscv64`'s own `sleep()` is deliberately split into
    `sleep_prepare(chan)` (register, while still holding the caller's
    lock) and a separate zero-arg `sleep()` (only actually block if the
    channel wasn't already woken up meanwhile) - a real fix for a
    narrow lost-wakeup race in the gap between releasing the caller's
    lock and the process actually going to sleep. This is the exact
    same split `kernel/sleeplock.c`'s own merge hit and deferred
    (`riscv64` was left out of that 6-fork cluster for precisely this
    reason - see that entry above). Rather than defer it again,
    `sleep_release()` gives every fork in this cluster the same call:
    a plain pass-through to `sleep(chan, lk)` for the four with the
    atomic version, and the real `sleep_prepare`/release/`sleep`/
    acquire sequence for `riscv64` - so `kernel/log.c` (and, if
    revisited, `sleeplock.c`) never needs to know which. `riscv64`'s
    own other callers of `sleep_prepare()`/`sleep()` (console.c,
    pipe.c, sysproc.c, proc.c, uart.c, virtio_disk.c) are untouched -
    they keep calling the fine-grained pair directly.
  - **`LOGSIZE`/`LOGBLOCKS`**: `riscv64`'s own `param.h` spells this
    constant `LOGBLOCKS` (`riscv64` also has its own separate
    `tools/mkfs.c`, one log block bigger than the four-fork shared
    family's - see this plan's own `mkfs*.c` entry - so its actual
    on-disk `nlog` genuinely differs at runtime, which is fine: the
    shared file reads `sb->nlog` into `log.size` regardless of arch,
    it never hardcodes either constant). Given `#define LOGSIZE
    LOGBLOCKS` as an alias in `riscv64`'s own `param.h` - same
    already-established pattern as `loongarch`'s own `KERNBASE ==
    RAMBASE` alias - rather than a rename, so nothing that already
    says `LOGBLOCKS` there needs to change.

  One real gap fixed in passing: `riscv64`'s own `kernel/defs.h` was
  the one fork in this cluster whose declarations (`pagetable_t` used
  in `proc_pagetable()` etc.) still relied on whichever `.c` file
  happened to `#include "riscv.h"` before `defs.h` - the same
  self-containment gap `sleeplock.c`'s own merge already fixed for
  `arm64`/`arm64-pi4`/`loongarch`/`riscv32`. Given the same fix:
  `#include "arch_vm.h"` at the top of `riscv64`'s own `defs.h` too.

  Verified: build + full `usertests` ("ALL TESTS PASSED") for all five
  forks, `make test-all`, `docker build --build-arg ARCH=<name>` for
  `arm64`/`loongarch`/`riscv32`/`riscv64` and `ARCH=all` for
  `arm64-pi4`. `riscv64`'s own `sys_sync()` has no coverage in its
  `test-xv6.py` harness at all (built into `fs.img` but never invoked)
  - checked by hand instead: booted interactively, ran `sync` at the
  shell, confirmed it returns and the shell keeps working afterward.

  **Still not merged: `amd64`, `i386`, `amd64-jserv`, `mips`.** All four
  use the *same* concurrent `begin_op()`/`end_op()` design as this
  cluster (not `kernel/log-legacy.c`'s older single-transaction one -
  confirmed by reading, not just line counts: `amd64-jserv`/`mips` are
  only 20-25 lines from `amd64`/`i386`, a difference of missing
  generalizations - `ROOTDEV` hardcoded instead of a `dev` param, no
  `bpin`/`bunpin` - not a different algorithm). But converging them
  onto *this* baseline means giving `amd64`/`i386`/`amd64-jserv`/`mips`'s
  own `bio.c` a `bpin()`/`bunpin()` pair and switching their `bget()`
  eviction check off `B_DIRTY`, which is the same `struct buf`
  unification `bio.c` itself has been waiting on. Left as their own
  future cluster (baseline `amd64`/`i386`, already 0-diff) rather than
  forced onto the `bpin`/`bunpin` design now - a real "give this file
  the more advanced design" case, but one that reaches into `bio.c`,
  so it should land together with (or after) that file, not as a
  drive-by inside `log.c`.

- **`kernel/string.c`** (2026-09-12, later same conversation) - split into
  three files, following the same "free the plain name" move as
  `kernel/legacy/` before it (`60352a9`):
  - `kernel/string.c` (new, from `arm64`'s own copy, `git mv`'d not
    recreated) - 5 forks (`arm64`, `arm64-pi4`, `loongarch`, `riscv32`,
    `riscv64`): a plain portable byte-loop `memset()`. `riscv64` looked
    like the furthest outlier by raw line count (42-49 lines) but that
    was pure whitespace/brace-style noise, caught with `diff -b -w` -
    the same false-outlier trap `arm`'s own `usertests.c` hit earlier.
    `riscv32` was the one real gap - missing the `n==0` early return in
    `memmove()` every other fork here already had; gained via the
    merge (baseline = most complete, not most common, per criterion 2).
  - `kernel/string-x86.c` (renamed from the old `kernel/string.c`) -
    `amd64`, `i386`, `amd64-jserv`: `memset()` via inline-asm
    `stosl()`/`stosb()`, a real hardware optimization, not a rename.
  - `kernel/string-arm.c` (renamed from `kernel/legacy/string.c`) - the
    four ARM32 Pi forks: `memset()` via a hand-rolled portable
    `memsetw()`/`memsetb()` word-at-a-time optimization, a different
    real optimization again - not the same code as `string-x86.c`'s,
    despite both being "an optimized memset".

  **`mips` deliberately not folded into `string-x86.c` yet**, despite
  its own `memset()` calling `stosl()`/`stosb()` with the exact same
  call shape (via its own `mips.h`, not `x86.h`) - confirmed by diffing
  directly against the shared file: only the include name, one
  `(int)dst` cast that should already be the established `(uintp)dst`,
  and whitespace differ. Joining needs `stosl`/`stosb` exposed under
  one arch-neutral header name first (the same `kernel/arch/<arch>/
  arch_*.h` shape as `kalloc.c`'s `PGSIZE`/`PGROUNDUP` or `log.c`'s
  `sleep_release()` - see `docs/claude_notes/
  notes_new_kernel_organization.md`), not a drive-by inside this
  commit.

  Verified: build + full `usertests` ("ALL TESTS PASSED") for all
  twelve touched forks (every fork except `mips` and `arm`), `make
  test-all`, and `git blame -C -C` on all three new/renamed files -
  confirmed tracing back to the real original authors (Russ Cox, Frans
  Kaashoek, Robert Morris, Austin Clements, Zhiyi Huang), not flattened
  to the move commit.

- **`kernel/sysproc.c`** (2026-09-12, later same conversation) - split
  into two files by design generation, the same shape as `kalloc.c`'s
  `use_lock` split and `log.c`'s `begin_op`/`end_op` split:
  - `kernel/sysproc.c` (new, from `arm64`'s own copy) - 4 forks
    (`arm64`, `arm64-pi4`, `loongarch`, `riscv32`): the newer design -
    `exit(status)`/`wait(&status)` (exit-status propagation), `uintp`
    return type (was `uint64`, converged to the established
    pointer-sized-integer interface so `riscv32`'s own `uint32` return
    just works). Picked over the larger `amd64`/`i386`/`amd64-jserv`/
    `mips`/arm-pi-cluster pool specifically because it's the more
    advanced design (criterion 2), confirmed by reading the diff, not
    just counting lines - the "58 lines apart" gap between the two
    pools is a real, later-xv6 feature addition, not noise. `loongarch`
    gained `uintp` in its own `include/arch/loongarch/arch.h` (the one
    fork in this cluster that didn't already have it from earlier
    work).
  - `kernel/sysproc-legacy.c` (renamed from `amd64`'s own copy) - 7
    forks (`amd64`, `i386`, `mips`, `arm-pi1`, `arm-pi1-bis`, `arm-pi2`,
    `arm-pi3`): the older design, `int` return, no exit status. The one
    real gap: `myproc()` isn't consistently a function - `amd64`/`i386`
    already call it, but `mips` uses a raw global `proc` and the
    (already-shared) `arm-pi` cluster uses a raw global `curr_proc`.
    Exactly the "missing interface, not a real difference" case this
    plan already flagged as likely (see the `copyin`/`copyout` write-up
    above) - closed with a trivial `#define myproc() (proc)` /
    `#define myproc() (curr_proc)` in each fork's own `proc.h`, not a
    real function anywhere but `amd64`/`i386`. Everything else was a
    dead per-arch include (`x86.h`/`mmu.h`/`arm.h`/`date.h` - none of
    which `sysproc.c` itself needs) plus whitespace.

  **`riscv64` and `amd64-jserv` both deliberately left out, for
  symmetric reasons** - each turned out to be a genuine further
  advance on its own family's baseline, not just a bigger diff:
  `riscv64`'s own `sysproc.c` has a whole extra lazy-`sbrk()`/
  page-fault-on-demand feature, `fork()`/`wait()`/`kill()`/`exit()`
  renamed to `kfork()`/`kwait()`/`kkill()`/`kexit()`, and a `killed()`
  accessor replacing direct `->killed` reads (it would also need
  `sleep_release()` in its own `sys_pause()` - already built for
  `log.c`, so at least that part is ready whenever this is revisited).
  `amd64-jserv`'s own `sys_sbrk()` uses `uintp`/a `arguintp()` argument
  parser instead of plain `int`/`argint()` - a real fix avoiding
  int-truncation of large growth amounts, but `arguintp()` doesn't
  exist in any of the other 7 forks, so adopting it means adding real
  new infrastructure across all of them, not a single-file move. Both
  are genuine "give this file the more advanced design" cases, same as
  the `amd64`/`i386`/`amd64-jserv`/`mips` gap left open in `log.c`
  above - flagged for a future, dedicated pass rather than forced in
  here.

  Verified: build + full `usertests` ("ALL TESTS PASSED") for all
  eleven touched forks, `make test-all`, `docker build --build-arg
  ARCH=<name>` for every touched fork with its own Dockerfile case and
  `ARCH=all` for `arm64-pi4`, and `git blame -C -C` on both new/renamed
  files.

- **Queued next: `kernel/bio.c`.** Previously postponed once already
  (criterion 6 - "prefer the easier file when a candidate reveals deep,
  costly work": `bio.c` needs a `struct buf` unification and a new
  `disk_rw()` shim before it can move, unlike `file.c`, which reused
  existing infrastructure) in favor of `file.c` -> `sleeplock.c` ->
  `kalloc.c` -> `log.c`. The `bpin()`/`bunpin()` gap found while doing
  `log.c` (above) is exactly this same prerequisite - `amd64`/`i386`/
  `amd64-jserv`/`mips`'s own `struct buf` uses a `B_DIRTY` flag where
  `arm64`/`arm64-pi4`/`loongarch`/`riscv32`/`riscv64` use a dedicated
  `int valid`/`int disk` pair plus real pin/unpin refcounting, and
  `bread()`/`bwrite()` call the disk driver by different names
  (`iderw(b)` vs `virtio_disk_rw(b, write)`) - a `disk_rw()` interface,
  the same shape as `sleep_release()` above, would close that second
  gap. Still on the Tier 3 list (`fs.c`, `log.c`, `bio.c`, the
  arch-independent half of `syscall.c`); not re-scoped yet.

  **Re-scoped (2026-09-12): the `disk_rw()`/`struct buf` prerequisite
  above turns out to block only one of three real clusters, not all of
  `bio.c`.** Re-running `pairwise_diff.sh kernel/bio.c` found:
  1. **`arm64`/`arm64-pi4`/`loongarch`/`riscv32`/`riscv64` (5 forks)
     merged into a new `kernel/bio.c`.** `riscv64`'s apparent 34-40 line
     gap was pure brace-style/whitespace noise (`diff -b -w`: a dead
     `#include "riscv.h"`, one stray comment, K&R-vs-Allman braces - no
     real difference), the same false-outlier trap hit repeatedly this
     session. One real gap turned up once actually merging, though:
     `arm64-pi4`/`loongarch` call `ramdiskrw(b, write)` (real Pi 4 and
     QEMU-loongarch boards, no virtio device), not `virtio_disk_rw(b,
     write)` like the other three - same `(struct buf*, int)` shape, so
     a `disk_rw(b, write)` interface closes it, same as `sleep_release`.
     **This one is genuinely board-scoped, not ISA-scoped** - `arm64`
     and `arm64-pi4` share one ISA directory (`kernel/arch/arm64/`) for
     `arch_vm.h`/`arch_proc.h` (those really are per-ISA), but need
     *different* `disk_rw()` backends. Solved with a new
     `kernel/arch/arm64-pi4/arch_disk.h` (ramdisk backend) added to
     that fork's own `-I` list *before* the shared `kernel/arch/arm64`
     one, so its file shadows the ISA-level one there (virtio backend)
     - the same "quoted-include search order" mechanism as gotcha 12,
     used deliberately this time rather than hit by accident. Verified
     past the source level: `objdump -dr kernel/bio.o` on both forks
     confirms each calls the right symbol (`ramdiskrw` for `arm64-pi4`,
     `virtio_disk_rw` for plain `arm64`). See `docs/claude_notes/
     notes_new_kernel_organization.md`'s own updated write-up.
  2. **`kernel/bio-legacy.c` can likely grow**, and isn't ARM-specific
     at all (further confirming the naming rationale above) - `mips`'s
     own `bio.c` is only ~14 lines from it (comment/whitespace noise,
     already calls the same `iderw()`); `amd64-jserv` is ~50 lines away
     but the same `B_BUSY`-flag-polling design (no real sleeplock),
     worth a closer read before merging. Not started.
  3. **`amd64`/`i386` are their own third cluster** (already 0-diff
     between themselves) - a genuinely more advanced locking primitive
     (real `sleeplock`/`acquiresleep()`, not `B_BUSY` polling) but
     still `iderw()`, not `virtio_disk_rw()`. Fits neither existing
     file; the `disk_rw()` interface only matters for bridging *this*
     pair into the fully modern `kernel/bio.c` eventually - not a
     blocker for (1) or (2). Not started.

  Verified: build + full `usertests` ("ALL TESTS PASSED") for all five
  forks in (1), `make test-all`, `docker build --build-arg ARCH=<name>`
  for `arm64`/`loongarch`/`riscv32`/`riscv64`, `ARCH=all` for
  `arm64-pi4`, and `git blame -C -C` on `kernel/bio.c`.

  **Finding, not fixed: `arm64`'s own `test-arm64` panics
  non-deterministically in `twochildren` (`kerneltrap`, a translation
  fault at a high kernel address), independent of this merge.** First
  seen as a real `docker build --build-arg ARCH=arm64` failure while
  verifying this commit; reproduced twice more on a truly clean local
  rebuild before being cleared as pre-existing - the deciding test was
  a bisection: `disk_rw(b, N)` preprocesses to byte-identical text as a
  direct `virtio_disk_rw(b, N)` call (checked with `gcc -E`), yet one
  run failed and a same-binary rerun passed, which is only possible if
  the failure is genuine QEMU/SMP-scheduling non-determinism, not a
  logic difference from this merge - confirmed with 5 further clean
  runs (3 on the reverted-to-direct-call binary, 2 more on the real
  `disk_rw` one), all "ALL TESTS PASSED". Rate looked like roughly 2 in
  7 runs failing this session, high enough to occasionally break CI on
  an unrelated commit. Not investigated further here (out of scope,
  pre-existing, not introduced by this merge) - worth a dedicated
  session with `notes_debugging_techniques.txt`'s own methodology if it
  recurs; not yet in any `notes_arch_arm64.txt`.

**Housekeeping, same conversation: `stress-test-all` moves to CI, not
every local iteration.** GitHub Actions CI is confirmed working now, so
the full ~25-minute `stress-test-all` sweep across every boot-testable
fork is the right thing to delegate there (on push) rather than
running locally after every single-file merge. `test-all` (the quick
boot-to-shell-prompt check, ~1 min for all 14) stays a mandatory local
gate - fast enough to run every time, and catches real breakage before
it ever reaches CI. Still run a targeted full `test-<arch>` and
`docker build --build-arg ARCH=<name>` locally for whichever forks a
given commit actually touches - both stay cheap at that scope, and
CI's own coverage is for confirming nothing *else* broke, not a
substitute for verifying the actual change.

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
