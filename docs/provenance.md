# Provenance and methodology

How `xv6-multiarch` was assembled from 13 separate upstream repositories, why
each fork point was chosen, the git techniques involved, four pitfalls that
silently break the result, and what is still approximate. See the root
`README.md` for the short version and the current build/boot status; this
file is the full evidence trail behind it.

The abandoned `gitlab.com/xv6-multiarch` project is *not* merged in — it is a
fresh `git init` with nothing recoverable — but its `kernel/i386`,
`kernel/arm` layout is the precedent for the `arch/<name>/` (now
`forks/<name>/`) convention here.

## Ports and where they came from

| `forks/` | Upstream source | Description |
|---|---|---|
| `i386` | [mit-pdos/xv6-public](https://github.com/mit-pdos/xv6-public) | the original teaching OS (2006-2020) |
| `amd64` | [mit-pdos/xv6-riscv](https://github.com/mit-pdos/xv6-riscv), pre-riscv | MIT's abandoned x86-64 port (2018) |
| `amd64-jserv` | [jserv/xv6-x86_64](https://github.com/jserv/xv6-x86_64) | a maintained x86-64 port (2013-2023) |
| `riscv64` | [mit-pdos/xv6-riscv](https://github.com/mit-pdos/xv6-riscv) | CURRENT MIT teaching OS (2019-present) |
| `riscv32` | [michaelengel/xv6-rv32](https://github.com/michaelengel/xv6-rv32) | 32-bit RISC-V, qemu (2020-2021) |
| `mips` | [nullpo-head/xv6-mips](https://github.com/nullpo-head/xv6-mips) | MIPS (2014-2016) |
| `loongarch` | [SKT-CPUOS/xv6-loongarch-exp](https://github.com/SKT-CPUOS/xv6-loongarch-exp) | LoongArch (2022-2023) |
| `arm-pi1` | [zhiyihuang/xv6_rpi_port](https://github.com/zhiyihuang/xv6_rpi_port) | ARMv6, Raspberry Pi 1 (2014) |
| `arm-pi2` | [zhiyihuang/xv6_rpi2_port](https://github.com/zhiyihuang/xv6_rpi2_port) | ARMv7, Raspberry Pi 2/3 (2017-2022) |
| `arm-pi1-bis` | [inaciose/xv6-armv6-rpi](https://github.com/inaciose/xv6-armv6-rpi) | ARMv6, Raspberry Pi B (2017) |
| `arm` | [inaciose/xv6-armv7-rpi](https://github.com/inaciose/xv6-armv7-rpi) | ARMv7 Cortex-A15/A7, Banana Pi (2017) |
| `arm-pi3` | [patha454/xv6_pi_mp](https://github.com/patha454/xv6_pi_mp) | AArch32 MP, Raspberry Pi 3 (2019) |
| `arm64` | [k-mrm/xv6-aarch64](https://github.com/k-mrm/xv6-aarch64) | 64-bit ARM (2021-2023) |
| `arm64-pi4` | [k-mrm/xv6-rpi4](https://github.com/k-mrm/xv6-rpi4) | AArch64, real Raspberry Pi 4 hardware (2022) |

14 ports covering 8 instruction sets (x86, x86-64, ARM32, AArch64, RV64,
RV32, MIPS, LoongArch), assembled from 13 distinct upstream repositories —
one fewer port than architectures because `amd64` and `riscv64` share a
single upstream repo (see below), and one fewer than there used to be: a
fifteenth port, `forks/d1` (michaelengel/xv6-d1, RISC-V64, Allwinner D1), was
imported, evaluated and then removed (commit `c233d3b`) once bring-up showed
it was build-only on real hardware with no QEMU machine model and no
kernel-logic difference from `forks/riscv64` worth keeping — see
`docs/claude_notes/done/plan_build_and_test.md`.

Several ports target the same instruction set independently: `amd64` and
`amd64-jserv` are unrelated x86-64 efforts (`amd64` is MIT's own abandoned
2018 experiment; `amd64-jserv` is jserv's independent, still-maintained
port); four separate ARM32 ports exist (`arm`, `arm-pi1`, `arm-pi1-bis`,
`arm-pi2`, plus `arm-pi3` in AArch32 compatibility mode — see CLAUDE.md's
"Architecture naming convention"); and `arm64`/`arm-pi3`/`arm64-pi4` are
three independent AArch64-family efforts.

Standalone branches (`git branch -a`) still use each port's *original* name
from when it was first imported, not its current `forks/` path — e.g.
`git log armv7-rpi`, not `git log arm`. `pi_mp` and `rpi4` don't have
standalone branches, only their `forks/` subtree on `main`. `main` is an
octopus merge of every port.

## The key property

The common MIT history is stored **once**, at its **original upstream commit
hashes** — not once per architecture. A line untouched since 2007 blames to
the *same commit object* from every port:

```sh
$ git blame forks/i386/sh.c             | head -1
1b25f3b0 sh.c (rsc 2007-08-28 ...) // Shell.
$ git blame forks/loongarch/user/sh.c   | head -1
1b25f3b0 sh.c (rsc 2007-08-28 ...) // Shell.
$ git blame forks/arm-pi1/uprogs/sh.c   | head -1
1b25f3b0 sh.c (rsc 2007-08-28 ...) // Shell.
```

`1b25f3b0` is Russ Cox's real commit in `mit-pdos/xv6-public`, a single node
here shared by every lineage. Files the ports actually changed blame to the
porters: `forks/arm-pi1/source/proc.c` is 226 lines of `rsc` and 67 of `rtm`
(MIT, 2006-07) alongside 78 of Zhiyi Huang (2014).

## Two kinds of upstream

The single most useful thing to establish about any port is whether it is a
**real git fork** (it kept MIT's commit objects) or a **fresh `git init`**
(someone unpacked a tarball and started over). It decides all the work:

| | real fork | fresh init |
|---|---|---|
| ports | `amd64-jserv`, `mips`, `arm64`, `arm64-pi4` (and `i386`/`riscv64`/`amd64`, one lineage) | `arm-pi1`, `arm-pi2`, `arm-pi1-bis`, `arm`, `riscv32`, `loongarch` (`d1`, removed, was also fresh-init) |
| fork point | computed: `git merge-base` | reconstructed from file content |
| certainty | fact | best-supported inference |
| work needed | none — just a staging commit | graft as a two-parent merge |

Check it in one command:

```sh
git -C <clone> cat-file -e 55e95b16db458b7f9abeca96e541acbdf8d7f85b && echo "real fork"
```

Three of the four architectures added in the second pass (`amd64-jserv`,
`mips`, `arm64`) turned out to be real forks, so they needed no grafting at
all. `arm64-pi4` (added later still) is also a real fork — of `arm64`
itself, not of MIT directly.

**A third case, found with `arm-pi3` (originally `pi_mp`):** a real fork
(genuine `git merge-base`, not content inference) of a port that was itself a
*fresh init* relative to MIT. Its fork point checks out exactly against
`arm-pi2` (originally `rpi2`) — but `arm-pi2`'s own upstream commits were
already re-parented once (its graft onto `arm-pi1`), so their hashes changed.
`arm-pi3`'s own commits needed the same re-parenting treatment, single-parent
(no two-parent merge needed, since the ancestry is real, not inferred) — just
onto the recreated hash rather than the vanished original upstream one. See
the table below.

## x86, amd64 and riscv are one repository

`xv6-riscv` and `xv6-x86` are not similar forks — they are literally the same
repository: same root (`55e95b1`, 2006-06-12), same commit objects, diverging
only at `82638c019ced44651d65c99aaa7c698676c217be` (2019-03-20). Afterwards
`xv6-x86` got 4 final commits; `xv6-riscv` got 700+, including a complete
**x86-64 port** later deleted wholesale:

- `amd64` begins at `ab0db651af6f1ffa8fe96909ce16ae314d65c3fb` ("Checkpoint port
  of xv6 to x86-64", Kaashoek, 2018-09-23).
- Last purely-amd64 state: `0f90388c893d1924e89e2e4d2187eda0004e9d73`
  ("No T_SYSCALL", 2018-10-10).
- `2ec1959fd1016a18ef3b2d154ce7076be8f237e4` (2019-05-31) starts deleting
  `mmu.h`/`msr.h` and adding `riscv.h`.

So `forks/i386`, `forks/riscv64` and `forks/amd64` are **three tips of one
lineage**. No splicing — only choosing three commits as branch heads. This is
also why `amd64` (MIT's dead experiment) is kept distinct from `amd64-jserv`
(jserv's independent, maintained port of the same ISA).

Lineage transitions inside MIT's own history are tagged too:
`history/x86-riscv-split`, `history/amd64-start`, `history/amd64-last`,
`history/riscv-transition`.

## Fork points and evidence

Where each architecture left the shared lineage, and how each claim is
backed. Every row also has an annotated tag carrying its evidence —
`git show forkpoint/<name>`.

**Verified** means the port is a genuine git fork that kept MIT's history, so
the point is computed by `git merge-base` and is fact. **Inferred** means the
port started as a fresh `git init`, so the point was reconstructed from file
content and documentary evidence — a best-supported conclusion, not a
certainty.

| Arch (original name) | Upstream source | Forked from | Status | Graft commit (this repo) | Evidence |
|---|---|---|---|---|---|
| `i386` (x86) / `riscv64` (riscv) | mit-pdos/xv6-public / xv6-riscv | same repository, split at `82638c0` (2019-03-20) | verified | — | `git merge-base` |
| `amd64` | xv6-riscv @ `0f90388` (2018-10-10) | riscv line, last purely-amd64 commit | verified | — | last commit before `2ec1959` removes amd64 |
| `amd64-jserv` (x86_64) | jserv/xv6-x86_64 | i386 @ `ff27834` (2013-03-04) | verified | — | `git merge-base`; carries Brian Swetland's 2013 restructuring (userspace into `user/`) |
| `mips` | nullpo-head/xv6-mips | i386 @ `41f16c2` (2014-10-03) | verified | — | `git merge-base` |
| `arm64` (aarch64) | k-mrm/xv6-aarch64 | riscv64 @ `a1da53a` (2021-09-01) | verified | — | `git merge-base`, the same commit `d1` used, independently |
| `arm-pi1` (rpi1) | zhiyihuang/xv6_rpi_port | i386 @ `ff27834` (2013-03-04) | inferred | `9f5d9ac2` | README cites MIT xv6 2012; `sh.c` byte-identical |
| `arm-pi2` (rpi2) | zhiyihuang/xv6_rpi2_port | `arm-pi1` tip @ `c4af2fe` (2014-07-16) | inferred | `50a6ec81` | README: "x86 to armv6 **and then to armv7**"; `uprogs/sh.c` identical |
| `arm-pi1-bis` (armv6-rpi) | inaciose/xv6-armv6-rpi | `arm-pi1` tip @ `c4af2fe` (2014-07-16) | inferred | `204d40a5` | upstream description: "based on zhiyihuang/xv6_rpi_port" |
| `arm` (armv7-rpi) | inaciose/xv6-armv7-rpi | i386 @ `ff27834` (2013-03-04) | inferred — weakest match | `ab5bfce1` | commit message names `xv6-armv7-a15`; identical under `diff -w` |
| `riscv32` (rv32) | michaelengel/xv6-rv32 | riscv64 @ `050a696` (2020-07-23) | inferred | `2c829c08` | README links mit-pdos/xv6-riscv; 45 lines / 6 files anchor diff |
| `loongarch` | SKT-CPUOS/xv6-loongarch-exp | riscv64 @ `cd00a82` (2021-10-17) | inferred | `356286f0` (root) | README links mit-pdos/xv6-riscv, uses riscv `kernel/`+`user/` layout; last riscv commit before the port began |
| `arm-pi3` (pi_mp) | patha454/xv6_pi_mp | `arm-pi2` mid-lineage @ `243c0e5` (2018-03-07) | verified | `243c0e5b` (re-parented copy of upstream `b90b42b9`) | real `git merge-base` against zhiyihuang/xv6_rpi2_port; own history includes a real two-parent merge (`7ff48ab`), preserved topologically rather than flattened |
| `arm64-pi4` (rpi4) | k-mrm/xv6-rpi4 | `arm64` mid-lineage @ `2c8131b` (2022-02-19) | verified | — | real fork; `2c8131bd` is a byte-identical commit object already present in this repo under `arm64`, so no re-parenting at all was needed, only a staging commit |
| `d1` — **removed** | michaelengel/xv6-d1 | riscv64 @ `a1da53a` (2021-09-01) | inferred | `ecb94ece` | README_D1.txt: "Changes to MIT's xv6 RISC-V version"; 4 lines / 6 files anchor diff — kept for the historical record even though `forks/d1` itself was later dropped |

## Commit counts by fork

Commits each port added *after* leaving the shared lineage — i.e. the actual
porting work, excluding the xv6 history it inherited. This is why the repo is
~2,000 commits rather than 13 × 1,700: almost everything is shared, and each
port is a comparatively small delta on top.

| arch (original name) | own commits | years | principal authors |
|---|---|---|---|
| `riscv64` (riscv) | 703 | 2018-2026 | Robert Morris, Frans Kaashoek |
| `amd64-jserv` (x86_64) | 124 | 2012-2023 | Brian Swetland, Jim Huang |
| `arm64` (aarch64) | 84 | 2021-2023 | Keisuke Iida |
| `arm-pi2` (rpi2) | 44 | 2017-2022 | Zhiyi Huang |
| `arm-pi3` (pi_mp) | 41 | 2019 | Harley Paterson |
| `arm64-pi4` (rpi4) | 30 | 2022 | Keisuke Iida |
| `amd64` | 22 | 2018 | Frans Kaashoek |
| `mips` | 22 | 2014-2016 | Yuichi Nishiwaki, Takaya Saeki |
| `loongarch` | 20 | 2022-2023 | luoszu, zhangxi |
| `arm-pi1` (rpi1) | 14 | 2014 | Zhiyi Huang |
| `d1` (removed) | 9 | 2021-2022 | Michael Engel, Daniel Maslowski |
| `i386` (x86) | 4 | 2019-2020 | Frans Kaashoek, James Houghton |
| `riscv32` (rv32) | 3 | 2020-2021 | Michael Engel |
| `arm-pi1-bis` (armv6-rpi) | 2 | 2017 | inaciose |
| `arm` (armv7-rpi) | 2 | 2017 | inaciose |

Some of these need reading with care:

- **`riscv64`'s 703** is not one port — it is MIT's whole post-2019 line,
  including the amd64 experiment and its removal, and continues to today. The
  `amd64` row's 22 commits are a *segment of that same lineage*, not separate
  work: amd64 is a tip on MIT's own history, not a fork of it.
- **`i386` only has 4** because the x86 repo was retired shortly after
  RISC-V took over; its real substance is the 1,000 commits of shared history
  every other row is built on.
- **`amd64-jserv` starts in 2012**, before its own fork point, because it
  carries Brian Swetland's restructuring commits with their original author
  dates.
- **`arm-pi1`'s 14 and `arm-pi2`'s 44** understate the family: `arm-pi1-bis`
  and `arm-pi2` both build on `arm-pi1`, so the ARM Raspberry Pi lineage is
  ~60 commits of shared effort across three repositories and two authors.
- **The small numbers are not small ports.** `arm-pi1-bis` and `arm` each
  landed an entire working ARM port in a single "initial import" commit.
  Commit count measures how the work was *published*, not how much of it
  there was.
- **`arm-pi3` and `arm64-pi4` are real forks *of* other forks in this repo**,
  not of MIT directly — `arm-pi3` off `arm-pi2` mid-lineage, `arm64-pi4` off
  `arm64` mid-lineage. Both fork points are verified by real `git merge-base`,
  not inferred from content, even though neither port's own commit chain
  touches MIT's history at all.

## The layout constraint (why one rename is unavoidable)

A commit's tree fixes its files' paths, so shared ancestor commits can hold
exactly one layout, not fourteen. Since each architecture needs its files at
a distinct path to coexist, **some rename must occur between shared history
and each tip.** The only choice is how to pay for it:

| approach | shared history | log |
|---|---|---|
| rewrite every commit into `arch/<name>/` (`filter-repo --to-subdirectory-filter`) | duplicated per arch under new hashes | every old message repeated once per arch |
| **one staging commit per arch (used here)** | **stored once, original hashes** | **~2,000 commits for 14 arches** |

Verified empirically: plain `git blame` and `git log --follow` both cross a
whole-tree rename with no flags, so the staging commit costs nothing.

## Technique for the fresh-init ports

**1. Find the fork point by content, not date.** Diff files that change slowly
across ports — `sh.c`, `ls.c`, `cat.c`, `wc.c`, `ulib.c` for x86-styled forks;
`kernel/spinlock.c`, `kernel/string.c`, `user/{sh,ls,cat,wc}.c` for RISC-V
ones — between the port's earliest real-content commit and each plausible
upstream commit. Lowest total diff wins. Sanity-check the result against a
much older baseline: `loongarch` scores 114 against its chosen point versus
323 against a 2020 one, confirming the vintage.

**2. Graft as a merge at the real-content commit, never at the port's root.**
Git's rename detection only bridges a delete and an add **within one commit**.
Several ports have throwaway commits first — `xv6_rpi_port` goes `"initial
files"` → `"try to delete files"` → `"add initial files"` → `"Create
README.md"` → `"This is just a test"` → **`"Initial full repository"`**.
Grafting the root puts the delete and the add several commits apart, detection
never sees them together, and blame dies at the port's own author for every
line. Instead make the real-content commit a two-parent merge: first parent
its own prior history, second parent the matched upstream commit. (Where root
*is* the real-content commit, as in `loongarch`, it simply takes the upstream
commit as its sole parent.)

**3. Re-parent with plumbing, not `filter-repo`** — see the GPG pitfall. Only
the port's own commits are recreated with `git commit-tree`, preserving tree,
message, author and committer identity and dates.

**4. One staging commit** moving the tree into `arch/<name>/` (now
`forks/<name>/`).

**5. Union via plumbing.** `git read-tree --prefix=arch/<name>/` per tip, then
`git commit-tree` with all tips as parents.

## Four pitfalls

**`filter-repo` strips GPG signatures.** MIT's history contains PGP-signed
commits (e.g. `fc1a5da2`, Tej Chajed, 2016). Running `git filter-repo` over
them removes the `gpgsig` header, changing the commit object, changing its
hash, cascading through every descendant. In an earlier build this silently
duplicated ~335 riscv commits into `arch/rv32` and ~173 into `arch/d1`, while
`arch/rpi1` was unaffected because its 2013 graft point predates the signed
commits. Never run `filter-repo` over upstream history; re-parent the port's
own handful of commits with `git commit-tree` instead.

**`git merge` cannot do the union.** Once branches share real ancestry, git
sees `sh.c` renamed to `arch/x86/sh.c` on one side and `arch/riscv/user/sh.c`
on the other and reports rename/rename conflicts on ~50 files per pair.
`-X no-renames` does **not** suppress this (tested). Build the tree with
`read-tree`/`commit-tree` instead.

**Whitespace hides real matches.** `xv6-armv7-rpi` (via the deleted
`inaciose/xv6-armv7-a15`) reindented the entire codebase from 2-space to tabs.
A naive line diff made it look like a rewrite; under `diff -w`, `sh.c`,
`ls.c`, `cat.c` and `wc.c` are byte-identical to their x86 originals. For the
same reason `git blame` needs `-C` on that architecture (now `forks/arm/`).

**`git ls-tree` quotes non-ASCII paths.** The LoongArch port ships a PDF with
a Chinese filename; feeding `git ls-tree --name-only` output to `git mv`
failed with `fatal: bad source`, because the name came back escaped. Use
`ls-tree --name-only -z` with `read -r -d ''`.

## Browsing the history

```sh
git log --first-parent main       # the upstream story, 2006 -> today
git log armv7-rpi                 # one architecture's own history (original branch name)
git blame forks/riscv64/kernel/proc.c
git blame -C forks/arm/usr/sh.c   # this port reindented everything; -C needed
git tag -l 'forkpoint/*' 'history/*'
git shortlog -sne main             # .mailmap collapses 129 author strings to 92
```

Plain `git blame` and `git log --follow` cross the `arch/` (now `forks/`)
staging commits on their own — no flags needed, except `forks/arm` as noted.
Bare `git log -- <path>` stops at a rename, which is ordinary git behaviour.

## Verification

```sh
git rev-list --count main                      # ~2,000
git branch --contains 55e95b16db458b7f9abeca96e541acbdf8d7f85b   # every port + main
git cat-file commit fc1a5da2 | grep gpgsig     # signature intact
git blame forks/<any>/…/sh.c | head -1          # -> 1b25f3b0 rsc 2007
git tag -l 'forkpoint/*' 'history/*'           # evidence-bearing tags
```

Per-arch commit counts are in the "Commit counts by fork" table above.

## Known gaps

- **Inferred fork points are approximations.** Those authors likely started
  from locally patched checkouts, not pristine upstream commits. The
  **verified** rows above are exact; the **inferred** ones are best-supported.
- **`arm`'s (armv7-rpi's) match is the weakest** — small header diffs remain
  even ignoring whitespace.
- **Author identities**: 129 raw strings for far fewer people, because xv6
  began in CVS (bare usernames `rsc`, `rtm`, `kaashoek`, `kolya`, no email)
  and contributors committed from many machines. A root `.mailmap` collapses
  these to 92 without touching commits. Only confident merges are included —
  and one earlier guess was avoided and later disproved: `mrm
  <cmpl.error@gmail.com>` (82 commits) looked like Robert Morris but shares an
  address with Keisuke Iida, the author of the aarch64 port. One inference is
  flagged in the file: `Your Name <you@example.com>`, on xv6-rv32's initial
  import, is mapped to Michael Engel, whose repository it is.
- **Old commits check out in root layout**, with no `arch/`/`forks/`
  directory — the staging commits exist only at the tips.
- **No build or boot verification at merge time.** Blobs were byte-identical
  to their sources, so nothing should have broken structurally — but the real
  answer is now tracked separately in
  `docs/claude_notes/done/plan_build_and_test.md`.

## Future work: factoring the architectures together

The intended end state is Linux-style — common code shared, arch-specific code
isolated — rather than near-duplicate trees per port. That work is planned in
detail, and blocked on the build-and-boot effort above, in
`docs/claude_notes/plan_factorization.md` — don't duplicate its rules here,
read it there. The short version of why it's safe to attempt at all: every
port's full tree was imported with real history *before* any unification, so
`git blame -C -C` can still attribute each line of a future shared file to
whichever port it came from, no matter which port's version becomes the base.

## Adding another architecture

1. Clone it; check `git cat-file -e 55e95b16…` to see if it is a real fork.
2. Real fork: `git merge-base` gives the point; no graft needed.
3. Fresh init: find the earliest commit holding the real port, then score
   anchor files (`diff -w` if it reformatted) against upstream candidates near
   its date; sanity-check against an older baseline.
4. Graft as a two-parent merge at that commit, using `git commit-tree`.
5. Add a staging commit moving the tree to `forks/<name>/`.
6. Rebuild the union and re-verify with the commands above.

See CLAUDE.md's "Adding a new arch (Phase 4)" for the separate, later
checklist covering build enablement rather than history construction.

## Licensing

Every port here grants the same MIT terms from the same original authors,
so the top-level `LICENSE` is the default for the whole repo. A
`forks/<name>/LICENSE` exists only where that port genuinely carries its
own copyright holder on top of MIT's - `arm-pi1`, `arm-pi1-bis` and
`arm-pi2`'s University of Otago/Cambridge authors, `arm`'s inaciose, and
`amd64-jserv`'s Jim Huang/Brian Swetland credits - and even those point
back to `LICENSE.common` for the shared permission and warranty text
rather than repeating it. Acknowledgments (John Lions's Commentary, the
contributor list) are consolidated the same way, in `README.common`.
