# Provenance and methodology

How `xv6-multiarch` was assembled from 12 separate repositories, why each fork
point was chosen, the git techniques involved, four pitfalls that silently
break the result, and what is still approximate.

The abandoned `gitlab.com/xv6-multiarch` project is *not* merged in — it is a
fresh `git init` with nothing recoverable — but its `kernel/i386`,
`kernel/arm` layout is the precedent for the `arch/<name>/` convention here.

## Upstream repo -> current `forks/` path

Everything below this point (fork points, evidence, commit counts) refers to
ports by their *original* identity - the upstream repo, or the name this
repo first gave the fork - since that is what the historical analysis is
about and it does not change. But `forks/<name>/` directory names have since
moved for six of them (see `CLAUDE.md`'s own "Architecture naming
convention" section for why - two different kinds of rename, both done
history-preservingly, verified via `git log --follow`/`git blame -C` after
each). This table is the current answer to "where did `<upstream>` end up":

| upstream repo | original name here | current `forks/` path |
|---|---|---|
| mit-pdos/xv6-public | `x86` | `forks/i386` |
| mit-pdos/xv6-riscv | `riscv` | `forks/riscv64` |
| MIT xv6-riscv, pre-riscv | `amd64` | `forks/amd64` (unchanged) |
| jserv/xv6-x86_64 | `x86_64` | `forks/amd64-jserv` |
| nullpo-head/xv6-mips | `mips` | `forks/mips` (unchanged) |
| k-mrm/xv6-aarch64 | `aarch64` | `forks/arm64` |
| zhiyihuang/xv6_rpi_port | `rpi1` | `forks/arm-pi1` |
| zhiyihuang/xv6_rpi2_port | `rpi2` | `forks/arm-pi2` |
| inaciose/xv6-armv6-rpi | `armv6-rpi` | `forks/arm-pi1-bis` |
| inaciose/xv6-armv7-rpi | `armv7-rpi` | `forks/arm` |
| michaelengel/xv6-rv32 | `rv32` | `forks/riscv32` |
| michaelengel/xv6-d1 | `d1` | `forks/d1` (unchanged) |
| SKT-CPUOS/xv6-loongarch-exp | `loongarch` | `forks/loongarch` (unchanged) |
| patha454/xv6_pi_mp | `pi_mp` | `forks/arm64-pi3` |
| k-mrm/xv6-rpi4 | `rpi4` | `forks/arm64-pi4` |

Standalone branches (`git branch -a`) still use the *original* names listed
above, not the current `forks/` paths - `git log armv7-rpi`, not
`git log arm`. See the root README's own "Browsing" section.

## Two kinds of upstream

The single most useful thing to establish about any port is whether it is a
**real git fork** (it kept MIT's commit objects) or a **fresh `git init`**
(someone unpacked a tarball and started over). It decides all the work:

| | real fork | fresh init |
|---|---|---|
| ports | `x86_64`, `mips`, `aarch64`, `rpi4` (and `x86`/`riscv`/`amd64`, one lineage) | `rpi1`, `rpi2`, `armv6-rpi`, `armv7-rpi`, `rv32`, `d1`, `loongarch` |
| fork point | computed: `git merge-base` | reconstructed from file content |
| certainty | fact | best-supported inference |
| work needed | none — just a staging commit | graft as a two-parent merge |

Check it in one command:

```sh
git -C <clone> cat-file -e 55e95b16db458b7f9abeca96e541acbdf8d7f85b && echo "real fork"
```

Three of the four architectures added in the second pass (`x86_64`, `mips`,
`aarch64`) turned out to be real forks, so they needed no grafting at all.
`rpi4` (added later still) is also a real fork — of `aarch64` itself, not of
MIT directly.

**A third case, found with `pi_mp`:** a real fork (genuine `git merge-base`,
not content inference) of a port that was itself a *fresh init* relative to
MIT. `pi_mp`'s fork point checks out exactly against `rpi2` — but `rpi2`'s own
upstream commits were already re-parented once (its graft onto `rpi1`), so
their hashes changed. `pi_mp`'s own commits needed the same re-parenting
treatment as `rpi2`'s own commits, single-parent (no two-parent merge needed,
since the ancestry is real, not inferred) — just onto the recreated hash
rather than the vanished original upstream one. See its row below.

## x86, amd64 and riscv are one repository

`xv6-riscv` and `xv6-x86` are not similar forks — they are literally the same
repository: same root (`55e95b1`, 2006-06-12), same commit objects, diverging
only at `82638c019ced44651d65c99aaa7c698676c217be` (2019-03-20). Afterwards
`xv6-x86` got 4 final commits; `xv6-riscv` got 700+, including a complete
**x86-64 port** later deleted wholesale:

- amd64 begins at `ab0db651af6f1ffa8fe96909ce16ae314d65c3fb` ("Checkpoint port
  of xv6 to x86-64", Kaashoek, 2018-09-23).
- Last purely-amd64 state: `0f90388c893d1924e89e2e4d2187eda0004e9d73`
  ("No T_SYSCALL", 2018-10-10).
- `2ec1959fd1016a18ef3b2d154ce7076be8f237e4` (2019-05-31) starts deleting
  `mmu.h`/`msr.h` and adding `riscv.h`.

So `arch/x86`, `arch/riscv` and `arch/amd64` are **three tips of one
lineage**. No splicing — only choosing three commits as branch heads. This is
also why `amd64` (MIT's dead experiment) is kept distinct from `x86_64`
(jserv's independent, maintained port of the same ISA).

## The layout constraint (why one rename is unavoidable)

A commit's tree fixes its files' paths, so shared ancestor commits can hold
exactly one layout, not thirteen. Since each architecture needs its files at a
distinct path to coexist, **some rename must occur between shared history and
each tip.** The only choice is how to pay for it:

| approach | shared history | log |
|---|---|---|
| rewrite every commit into `arch/<name>/` (`filter-repo --to-subdirectory-filter`) | duplicated per arch under new hashes | every old message repeated once per arch |
| **one staging commit per arch (used here)** | **stored once, original hashes** | **2,049 commits for 13 arches** |

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

**4. One staging commit** moving the tree into `arch/<name>/`.

**5. Union via plumbing.** `git read-tree --prefix=arch/<name>/` per tip, then
`git commit-tree` with all thirteen tips as parents.

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
same reason `git blame` needs `-C` on that architecture.

**`git ls-tree` quotes non-ASCII paths.** The LoongArch port ships a PDF with
a Chinese filename; feeding `git ls-tree --name-only` output to `git mv`
failed with `fatal: bad source`, because the name came back escaped. Use
`ls-tree --name-only -z` with `read -r -d ''`.

## Fork-by-fork evidence

| Arch | Source | Graft commit | Second parent | Evidence |
|---|---|---|---|---|
| `x86` | mit-pdos/xv6-public | — | — | shares history with xv6-riscv to `82638c0` |
| `riscv` | mit-pdos/xv6-riscv | — | — | as above |
| `amd64` | xv6-riscv @ `0f90388` | — | — | last commit before `2ec1959` removes amd64 |
| `x86_64` | jserv/xv6-x86_64 | — | — | **real fork**; merge-base `ff278344`. Carries Brian Swetland's 2013 restructuring (userspace into `user/`) |
| `mips` | nullpo-head/xv6-mips | — | — | **real fork**; merge-base `41f16c21` |
| `aarch64` | k-mrm/xv6-aarch64 | — | — | **real fork**; merge-base `a1da53a5`, the same commit `d1` used, independently |
| `rpi1` | zhiyihuang/xv6_rpi_port | `9f5d9ac2` | `x86 @ ff278344` | README cites MIT xv6 2012; `sh.c` byte-identical |
| `rpi2` | zhiyihuang/xv6_rpi2_port | `50a6ec81` | `rpi1` tip | README: "x86 to armv6 **and then to armv7**"; `uprogs/sh.c` identical |
| `armv6-rpi` | inaciose/xv6-armv6-rpi | `204d40a5` | `rpi1` tip | upstream description: "based on zhiyihuang/xv6_rpi_port" |
| `armv7-rpi` | inaciose/xv6-armv7-rpi | `ab5bfce1` | `x86 @ ff278344` | commit message names `xv6-armv7-a15`; identical under `diff -w` |
| `rv32` | michaelengel/xv6-rv32 | `2c829c08` | `riscv @ 050a6961` | README links mit-pdos/xv6-riscv; 45 lines / 6 files |
| `d1` | michaelengel/xv6-d1 | `ecb94ece` | `riscv @ a1da53a5` | README_D1.txt: "Changes to MIT's xv6 RISC-V version"; 4 lines / 6 files |
| `loongarch` | SKT-CPUOS/xv6-loongarch-exp | `356286f0` (root) | `riscv @ cd00a823` | README links mit-pdos/xv6-riscv, uses riscv `kernel/`+`user/` layout; last riscv commit before the port began |
| `pi_mp` | patha454/xv6_pi_mp | `243c0e5b` (this repo's re-parented copy of upstream `b90b42b9`) | `rpi2` mid-lineage, "Tidy up framebuffer code in console.c" (2018-03-07) | **verified** — real `git merge-base` against zhiyihuang/xv6_rpi2_port; own history includes a real two-parent merge (`7ff48ab`), preserved topologically rather than flattened |
| `rpi4` | k-mrm/xv6-rpi4 | — | `aarch64 @ 2c8131bd`, "init" (2022-02-19) | **verified** — real fork; `2c8131bd` is a byte-identical commit object already present in this repo under `arch/aarch64`, so no re-parenting at all was needed, only a staging commit |

`inaciose/xv6-armv7-a15`, the immediate parent of `armv7-rpi`, returns 404.
`inaciose/xv6-armv7-a7` was cloned as corroboration but is itself a fresh
`git init`, so `armv7-rpi` grafts straight onto x86.

## Verification

```sh
git rev-list --count main                      # 2049
git branch --contains 55e95b16db458b7f9abeca96e541acbdf8d7f85b   # all 13 + main
git cat-file commit fc1a5da2 | grep gpgsig     # signature intact
git blame arch/<any>/…/sh.c | head -1          # -> 1b25f3b0 rsc 2007
git tag -l 'forkpoint/*' 'history/*'           # 16 evidence-bearing tags
```

Each architecture contributes only its own commits: riscv 1708 (incl.
staging), then x86 +5, amd64 +1, x86_64 +125, mips +23, aarch64 +85, rpi1 +15,
rpi2 +45, armv6-rpi +3, armv7-rpi +3, rv32 +4, d1 +10, loongarch +21, pi_mp
+41, rpi4 +30.

## Known gaps

- **Inferred fork points are approximations.** Those authors likely started
  from locally patched checkouts, not pristine upstream commits. The
  **verified** rows are exact; the **inferred** ones are best-supported.
- **`armv7-rpi`'s match is the weakest** — small header diffs remain even
  ignoring whitespace.
- **Author identities**: 129 raw strings for far fewer people, because xv6
  began in CVS (bare usernames `rsc`, `rtm`, `kaashoek`, `kolya`, no email)
  and contributors committed from many machines. A root `.mailmap` collapses
  these to 92 without touching commits. Only confident merges are included —
  and one earlier guess was avoided and later disproved: `mrm
  <cmpl.error@gmail.com>` (82 commits) looked like Robert Morris but shares an
  address with Keisuke Iida, the author of the aarch64 port. One inference is
  flagged in the file: `Your Name <you@example.com>`, on xv6-rv32's initial
  import, is mapped to Michael Engel, whose repository it is.
- **Old commits check out in root layout**, with no `arch/` directory — the
  staging commits exist only at the tips.
- **No build or boot verification.** Blobs are byte-identical to their
  sources, so nothing should have broken, but that is an assumption.

## Future work: factoring the architectures together

The intended end state is Linux-style — common code shared, arch-specific code
isolated — rather than thirteen near-duplicate trees.

**Preserve blame while factoring.** When creating a shared file, delete the
per-arch copies and add the shared file **in the same commit**. Then
`git blame -C -C` can attribute each line to whichever port it came from. This
is the same single-commit-rename mechanism that makes the grafts work, and it
is why all thirteen copies were imported first: a unification that picked one
arch as the base would make the others' authorship permanently unrecoverable.

**Prefer per-arch files over `#ifdef`.** Linux deliberately keeps
`#ifdef CONFIG_<ARCH>` out of common code, using per-arch implementations
behind a common interface. For a teaching OS whose value is readability,
`#ifdef`-laced common files would be worse than the present duplication.

Start with files already byte-identical across several ports (`sh.c`, `ls.c`,
`cat.c`, `wc.c`) before attempting `proc.c`, `vm.c` or `trap.c`.

## Adding another architecture

1. Clone it; check `git cat-file -e 55e95b16…` to see if it is a real fork.
2. Real fork: `git merge-base` gives the point; no graft needed.
3. Fresh init: find the earliest commit holding the real port, then score
   anchor files (`diff -w` if it reformatted) against upstream candidates near
   its date; sanity-check against an older baseline.
4. Graft as a two-parent merge at that commit, using `git commit-tree`.
5. Add a staging commit moving the tree to `arch/<name>/`.
6. Rebuild the union and re-verify with the commands above.
