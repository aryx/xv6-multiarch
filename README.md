# xv6-multiarch

xv6 for **13 architectures and eras** in one repository, assembled from 12
separate upstream repos, with history stitched together so that `git blame`
traces every unchanged line back to the original 2006 MIT import — even for
ports that began as a fresh `git init` with no shared history at all.

```
arch/
  x86/          MIT xv6-public            — the original teaching OS (2006-2020)
  riscv/        MIT xv6-riscv             — current MIT teaching OS (2019-present)
  amd64/        MIT xv6-riscv, pre-riscv  — MIT's abandoned x86-64 port (2018)
  x86_64/       jserv/xv6-x86_64          — a maintained x86-64 port (2013-2023)
  mips/         nullpo-head/xv6-mips      — MIPS (2014-2016)
  aarch64/      k-mrm/xv6-aarch64         — 64-bit ARM (2021-2023)
  rpi1/         zhiyihuang/xv6_rpi_port   — ARMv6, Raspberry Pi 1 (2014)
  rpi2/         zhiyihuang/xv6_rpi2_port  — ARMv7, Raspberry Pi 2/3 (2017-2022)
  armv6-rpi/    inaciose/xv6-armv6-rpi    — ARMv6, Raspberry Pi B (2017)
  armv7-rpi/    inaciose/xv6-armv7-rpi    — ARMv7 Cortex-A15/A7, Banana Pi (2017)
  rv32/         michaelengel/xv6-rv32     — 32-bit RISC-V, qemu (2020-2021)
  d1/           michaelengel/xv6-d1       — 64-bit RISC-V, Allwinner D1 (2021-2022)
  loongarch/    SKT-CPUOS/xv6-loongarch-exp — LoongArch (2022-2023)
```

`amd64` and `x86_64` are both x86-64 but unrelated efforts: `amd64` is MIT's
own 2018 experiment, deleted a year later in favour of RISC-V; `x86_64` is
jserv's independent port, still alive in 2023.

Each architecture is also its own branch (`x86`, `riscv`, `amd64`, `x86_64`,
`mips`, `aarch64`, `rpi1`, `rpi2`, `armv6-rpi`, `armv7-rpi`, `rv32`, `d1`,
`loongarch`) if you want one checked out without the other twelve. `main` is
an octopus merge of all thirteen.

## The key property

The common MIT history is stored **once**, at its **original upstream commit
hashes** — not once per architecture. A line untouched since 2007 blames to
the *same commit object* from every port:

```sh
$ git blame arch/x86/sh.c       | head -1
1b25f3b0 sh.c (rsc 2007-08-28 ...) // Shell.
$ git blame arch/loongarch/user/sh.c | head -1
1b25f3b0 sh.c (rsc 2007-08-28 ...) // Shell.
$ git blame arch/rpi1/uprogs/sh.c    | head -1
1b25f3b0 sh.c (rsc 2007-08-28 ...) // Shell.
```

`1b25f3b0` is Russ Cox's real commit in `mit-pdos/xv6-public`, a single node
here shared by all thirteen lineages. Files the ports actually changed blame
to the porters: `arch/rpi1/source/proc.c` is 226 lines of `rsc` and 67 of
`rtm` (MIT, 2006-07) alongside 78 of Zhiyi Huang (2014).

## Fork points

Where each architecture left the shared lineage. Every row has an annotated
tag carrying its evidence — `git show forkpoint/d1`.

| arch | forked from | at | how established |
|---|---|---|---|
| `x86` / `riscv` | — | split at `82638c0` (2019-03-20) | same repository; `git merge-base` |
| `amd64` | riscv line | `0f90388` (2018-10-10) | last purely-amd64 commit before RISC-V replaced it |
| `x86_64` | x86 | `ff27834` (2013-03-04) | **verified** — real fork, `git merge-base` |
| `mips` | x86 | `41f16c2` (2014-10-03) | **verified** — real fork, `git merge-base` |
| `aarch64` | riscv | `a1da53a` (2021-09-01) | **verified** — real fork, `git merge-base` |
| `rpi1` | x86 | `ff27834` (2013-03-04) | inferred — `sh.c` byte-identical |
| `rpi2` | rpi1 tip | `c4af2fe` (2014-07-16) | inferred — README, and `uprogs/sh.c` identical |
| `armv6-rpi` | rpi1 tip | `c4af2fe` (2014-07-16) | inferred — upstream repo description |
| `armv7-rpi` | x86 | `ff27834` (2013-03-04) | inferred — identical under `diff -w` |
| `rv32` | riscv | `050a696` (2020-07-23) | inferred — 45 lines of anchor diff |
| `d1` | riscv | `a1da53a` (2021-09-01) | inferred — 4 lines of anchor diff |
| `loongarch` | riscv | `cd00a82` (2021-10-17) | inferred — last riscv commit before the port began |

**Verified** means the port is a genuine git fork that kept MIT's history, so
the point is computed by `git merge-base` and is fact. **Inferred** means the
port started as a fresh `git init`, so the point was reconstructed from file
content and documentary evidence — a best-supported conclusion, not a
certainty. `docs/PROVENANCE.md` gives the full reasoning for each.

Lineage transitions inside MIT's own history are tagged too:
`history/x86-riscv-split`, `history/amd64-start`, `history/amd64-last`,
`history/riscv-transition`.

## Browsing

```sh
git log --first-parent main   # the upstream story, 2006 -> today
git log rpi1                  # one architecture's own history
git blame arch/riscv/kernel/proc.c
git blame -C arch/armv7-rpi/usr/sh.c   # this port reindented everything; -C needed
git tag -l 'forkpoint/*' 'history/*'
git shortlog -sne main        # .mailmap collapses 129 author strings to 92
```

Plain `git blame` and `git log --follow` cross the `arch/` staging commits on
their own — no flags needed, except `armv7-rpi` as noted. Bare
`git log -- <path>` stops at a rename, which is ordinary git behaviour.

## Structure of the history

Each branch is: real upstream history (original hashes) → that port's own
commits → **one** staging commit moving the tree into `arch/<name>/`. Nothing
below the staging commit is rewritten.

A commit's tree fixes its paths, so shared ancestor commits can hold *one*
layout, not thirteen — some rename must occur between shared history and each
tip. Paying for that as one staging commit per arch, rather than by
duplicating the whole history thirteen times, is what keeps both the log
readable and the hashes original.

## Status and intent

This repo captures **provenance**, not a working unified build. Each
`arch/<name>/` keeps its own Makefile and its own copy of files that are often
identical across ports. Unifying them — Linux-style, with common code factored
out and arch-specific code behind a clean interface — is deliberately left as
separate future work, now that all thirteen copies are in history for
`git blame -C` to draw on.

## Licensing

Each `arch/<name>/LICENSE` carries the terms from that architecture's original
repository, all descending from MIT's xv6 permission notice
(see `arch/x86/LICENSE`). No license text was altered by this merge.
