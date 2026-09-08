# xv6-multiarch

**15 xv6 ports, covering 8 instruction sets** — i386, x86-64, ARM32, AArch64,
RV64, RV32, MIPS and LoongArch — in one repository, assembled from 14 separate
upstream repos, with history stitched together so that `git blame` traces every
unchanged line back to the original 2006 MIT import, even for ports that began
as a fresh `git init` with no shared history at all.

Fifteen ports rather than fifteen architectures: several target the same
instruction set independently (`amd64` and `x86_64` are unrelated x86-64
efforts; four separate ARM32 ports exist, and `aarch64`/`pi_mp`/`rpi4` are
three independent AArch64 efforts), and `amd64` is not a fork at all but
a preserved point in MIT's own timeline — their 2018 x86-64 experiment, deleted
a year later when RISC-V replaced it.

```
arch/
  x86/          MIT xv6-public            — the original teaching OS (2006-2020)
  riscv/        MIT xv6-riscv             — current MIT teaching OS (2019-present)
  amd64/        MIT xv6-riscv, pre-riscv  — MIT's abandoned x86-64 port (2018)
  x86_64/       jserv/xv6-x86_64          — a maintained x86-64 port (2013-2023)
                (now forks/amd64-jserv/ - grouped under the "amd64" ISA
                prefix alongside MIT's own forks/amd64, see CLAUDE.md)
  mips/         nullpo-head/xv6-mips      — MIPS (2014-2016)
  aarch64/      k-mrm/xv6-aarch64         — 64-bit ARM (2021-2023)
                (now forks/arm64/ - the AArch64 ISA representative)
  rpi1/         zhiyihuang/xv6_rpi_port   — ARMv6, Raspberry Pi 1 (2014)
                (now forks/arm-pi1/)
  rpi2/         zhiyihuang/xv6_rpi2_port  — ARMv7, Raspberry Pi 2/3 (2017-2022)
                (now forks/arm-pi2/)
  armv6-rpi/    inaciose/xv6-armv6-rpi    — ARMv6, Raspberry Pi B (2017)
                (now forks/arm-pi1-bis/ - independent port of the same
                real board as forks/arm-pi1, see CLAUDE.md)
  armv7-rpi/    inaciose/xv6-armv7-rpi    — ARMv7 Cortex-A15/A7, Banana Pi (2017)
                (now forks/arm/ - the clear winner among the four ARM32
                ports, promoted to the bare ISA name, see CLAUDE.md)
  rv32/         michaelengel/xv6-rv32     — 32-bit RISC-V, qemu (2020-2021)
                (now forks/riscv32/ - the RV32 ISA representative)
  d1/           michaelengel/xv6-d1       — 64-bit RISC-V, Allwinner D1 (2021-2022)
  loongarch/    SKT-CPUOS/xv6-loongarch-exp — LoongArch (2022-2023)
  pi_mp/        patha454/xv6_pi_mp        — AArch64 MP, Raspberry Pi 3 (2019)
                (now forks/arm64-pi3/)
  rpi4/         k-mrm/xv6-rpi4            — AArch64, real Raspberry Pi 4 hardware (2022)
                (now forks/arm64-pi4/)
```

`amd64` and `x86_64` are both x86-64 but unrelated efforts: `amd64` is MIT's
own 2018 experiment, deleted a year later in favour of RISC-V; `x86_64` is
jserv's independent port, still alive in 2023.

Each architecture is also its own branch (`x86`, `riscv`, `amd64`, `x86_64`,
`mips`, `aarch64`, `rpi1`, `rpi2`, `armv6-rpi`, `armv7-rpi`, `rv32`, `d1`,
`loongarch`) if you want one checked out without the others — `pi_mp` and
`rpi4` don't have standalone branches yet, only their `forks/` subtree on
`main`. `main` is an octopus merge of all fifteen.

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
here shared by all thirteen lineages. Files the ports actually changed blame
to the porters: `forks/arm-pi1/source/proc.c` is 226 lines of `rsc` and 67 of
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
| `pi_mp` | rpi2 mid-lineage | `243c0e5` (2018-03-07) | **verified** — real fork, `git merge-base` against zhiyihuang/xv6_rpi2_port (whose own commits were themselves re-parented once already) |
| `rpi4` | aarch64 mid-lineage | `2c8131b` (2022-02-19) | **verified** — real fork, `git merge-base` against k-mrm's own xv6-aarch64 |

**Verified** means the port is a genuine git fork that kept MIT's history, so
the point is computed by `git merge-base` and is fact. **Inferred** means the
port started as a fresh `git init`, so the point was reconstructed from file
content and documentary evidence — a best-supported conclusion, not a
certainty. `docs/PROVENANCE.md` gives the full reasoning for each.

Lineage transitions inside MIT's own history are tagged too:
`history/x86-riscv-split`, `history/amd64-start`, `history/amd64-last`,
`history/riscv-transition`.

## What each fork contributed

Commits each port added *after* leaving the shared lineage — i.e. the actual
porting work, excluding the xv6 history it inherited. This is why the repo is
2,050 commits rather than 13 × 1,700: almost everything is shared, and each
port is a comparatively small delta on top.

| arch | own commits | years | principal authors |
|---|---|---|---|
| `riscv` | 703 | 2018-2026 | Robert Morris, Frans Kaashoek |
| `x86_64` | 124 | 2012-2023 | Brian Swetland, Jim Huang |
| `aarch64` | 84 | 2021-2023 | Keisuke Iida |
| `rpi2` | 44 | 2017-2022 | Zhiyi Huang |
| `pi_mp` | 41 | 2019 | Harley Paterson |
| `rpi4` | 30 | 2022 | Keisuke Iida |
| `amd64` | 22 | 2018 | Frans Kaashoek |
| `mips` | 22 | 2014-2016 | Yuichi Nishiwaki, Takaya Saeki |
| `loongarch` | 20 | 2022-2023 | luoszu, zhangxi |
| `rpi1` | 14 | 2014 | Zhiyi Huang |
| `d1` | 9 | 2021-2022 | Michael Engel, Daniel Maslowski |
| `x86` | 4 | 2019-2020 | Frans Kaashoek, James Houghton |
| `rv32` | 3 | 2020-2021 | Michael Engel |
| `armv6-rpi` | 2 | 2017 | inaciose |
| `armv7-rpi` | 2 | 2017 | inaciose |

Some of these need reading with care:

- **`riscv`'s 703** is not one port — it is MIT's whole post-2019 line,
  including the amd64 experiment and its removal, and continues to today. The
  `amd64` row's 22 commits are a *segment of that same lineage*, not separate
  work: amd64 is a tip on MIT's own history, not a fork of it.
- **`x86` only has 4** because the x86 repo was retired shortly after RISC-V
  took over; its real substance is the 1,000 commits of shared history every
  other row is built on.
- **`x86_64` starts in 2012**, before its own fork point, because it carries
  Brian Swetland's restructuring commits with their original author dates.
- **`rpi1`'s 14 and `rpi2`'s 44** understate the family: `armv6-rpi` and
  `rpi2` both build on `rpi1`, so the ARM Raspberry Pi lineage is ~60 commits
  of shared effort across three repositories and two authors.
- **The small numbers are not small ports.** `armv6-rpi` and `armv7-rpi` each
  landed an entire working ARM port in a single "initial import" commit. Commit
  count measures how the work was *published*, not how much of it there was.
- **`pi_mp` and `rpi4` are real forks *of* other forks in this repo**, not of
  MIT directly — `pi_mp` off `rpi2` mid-lineage, `rpi4` off `aarch64`
  mid-lineage. Both fork points are verified by real `git merge-base`, not
  inferred from content, even though neither port's own commit chain touches
  MIT's history at all.

## Browsing

```sh
git log --first-parent main   # the upstream story, 2006 -> today
git log rpi1                  # one architecture's own history
git blame forks/riscv64/kernel/proc.c
git blame -C forks/arm/usr/sh.c         # this port reindented everything; -C needed
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

Each `forks/<name>/LICENSE` carries the terms from that architecture's original
repository, all descending from MIT's xv6 permission notice
(see `forks/i386/LICENSE`). No license text was altered by this merge.
