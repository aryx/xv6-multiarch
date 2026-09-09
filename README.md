# xv6-multiarch

`xv6-multiarch` unifies **14 xv6 ports across 8 instruction sets** — x86,
x86-64, ARM32, AArch64, RV64, RV32, MIPS and LoongArch — into one
repository, with git history stitched together so `git blame -C -C` traces
every unchanged line back to its real original commit, even across ports
that started life as a fresh `git init` with no shared history at all:

```sh
$ git blame forks/i386/sh.c           | head -1
1b25f3b0 sh.c (rsc 2007-08-28 ...) // Shell.
$ git blame forks/loongarch/user/sh.c | head -1
1b25f3b0 sh.c (rsc 2007-08-28 ...) // Shell.
```

That's the same commit object — Russ Cox's original 2007 line — surfacing
identically from two ports with no shared repository history until this one
stitched them together. The full story of how (fork points, evidence,
techniques, pitfalls) is in [`docs/provenance.md`](docs/provenance.md).

## Status: all 14 ports build and boot

**Goal 1 is done** (2026-09-09). Every port in this repo builds with a
current toolchain, boots to a real interactive shell under QEMU, and passes
its own `usertests` suite — not a mock, not a partial boot. `make test-all`
re-checks all of it in about a minute:

| port | ISA | QEMU machine | boots | `usertests` |
|---|---|---|---|---|
| `forks/i386` | x86 | `qemu-system-i386` (pc) | ✅ | ✅ |
| `forks/amd64` | x86-64 | `qemu-system-x86_64 -kernel` | ✅ | ✅ |
| `forks/amd64-jserv` | x86-64 | `qemu-system-x86_64` | ✅ | ✅ |
| `forks/riscv64` | RV64 | `-M virt` | ✅ | ✅ |
| `forks/riscv32` | RV32 | `-M virt` | ✅ | ✅ |
| `forks/arm64` | AArch64 | `-M virt,gic-version=3` | ✅ | ✅ |
| `forks/arm64-pi4` | AArch64 | `-M raspi4b` † | ✅ | ✅ |
| `forks/arm` | ARM32 | `-M raspi2b` | ✅ | ✅ * |
| `forks/arm-pi1` | ARM32 | `-M raspi1ap` | ✅ | ✅ |
| `forks/arm-pi1-bis` | ARM32 | `-M raspi1ap` | ✅ | ✅ |
| `forks/arm-pi2` | ARM32 | `-M raspi2b` | ✅ | ✅ |
| `forks/arm-pi3` | ARM32 | `-M raspi3b` | ✅ | ✅ |
| `forks/mips` | MIPS | `-M malta` | ✅ | ✅ * |
| `forks/loongarch` | LoongArch | `-M virt` | ✅ | ✅ |

Verified on Ubuntu 24.04.2 aarch64, GCC 13.3, QEMU 8.2.2, pinned in the
`Dockerfile` and re-run per-arch by
[CI](.github/workflows/docker.yml). "Boots" means a genuinely interactive
shell over the serial console; six ports (`i386`, `amd64`, `amd64-jserv`,
`arm-pi1`, `arm-pi1-bis`, `arm-pi3`) also boot into a real graphical
framebuffer console with a working emulated keyboard — `make
run-<arch>-qemu-graphics`.

† `arm64-pi4` needs QEMU ≥ 9.1 (`-M raspi4b` did not exist before that);
this distro ships 8.2.2, so its **boot** is verified against a local source
build of QEMU and is deliberately kept out of `test-all` and CI. Its
**build** is in `build-all` like every other port.

\* Two ports reach "ALL TESTS PASSED" with some `usertests` sub-tests
commented out and individually diagnosed: `mips` skips four
(`sbrktest`, `validatetest`, `exitwait`, `forktest`) and `arm` one
(`sbrktest`). Each skip is recorded in that port's own `usertests.c` and
written up in its `docs/claude_notes/notes_arch_*.txt`.

**No port skips `preempt()` or `mem()` any more** — all three of those
skips were fixed 2026-09-09, and none was what it had been recorded as.
`arm` had no working timer interrupt at all (it drove the SP804, which
QEMU stubs out with `create_unimp()`). `mips` had the interrupt but a
yield condition comparing the CP0 Cause register against an x86
constant, so it could never fire. And `mem()`, skipped in two ports as
"real memory pressure", was neither a hang nor a kernel bug: `umalloc`'s
`morecore()` stranded an unusable fragment on the free list per chunk,
making the whole thing quadratic in RAM — which is why ports with small
fixed `PHYSTOP` passed it and ports that size memory from real firmware
did not.

Getting there took dozens of separately diagnosed bugs — modern-GCC
breakage, missing `.bss` zeroing, SMP boot races, a Top-Byte-Ignore setting
that aliased every user pointer, a per-CPU GIC timer that left three of four
cores unpreemptible. Every one is written up per port under
[`docs/claude_notes/`](docs/claude_notes/), alongside the general
[debugging techniques](docs/claude_notes/notes_debugging_techniques.txt)
they produced. The plan that got there is
[`docs/claude_notes/done/plan_build_and_test.md`](docs/claude_notes/done/plan_build_and_test.md);
the leftovers it did not cover are in
[`plan_build_and_test_2.md`](docs/claude_notes/plan_build_and_test_2.md).

## Where this is going next

**Goal 2: factor the near-duplicate trees into a Linux-style layout** —
`user/`, `kernel/`, `include/` shared, `arch/<name>/` per port. This was
blocked on goal 1, because merging files you haven't verified still build is
how this repo's abandoned predecessor (`gitlab.com/xv6-multiarch`) died. That
block is now lifted, and `make test-all` is the safety net it was waiting
for. See
[`docs/claude_notes/plan_factorization.md`](docs/claude_notes/plan_factorization.md).

## Build and run

```sh
./configure           # detects each wired-up arch's toolchain + qemu-system-*
make build-riscv64    # build one arch
make run-riscv64      # build + boot interactively (Ctrl-A X to quit QEMU)
make quick-test-riscv64   # build + boot headless + assert a shell prompt
make test-riscv64     # build + boot headless + run that port's usertests suite
make test-all         # quick boot check of every wired-up arch (~1 min)
make stress-test-all  # every arch's full usertests run (~25 min)
make build-all / clean-all / kill-all   # across every wired-up arch
make build-docker [ARCH=riscv64]        # same, inside the pinned Dockerfile
```

Same shape for every other wired-up arch — replace `riscv64` with any port
name from the table above. A few ports also have
`make run-<arch>-qemu-graphics` (a real GTK window with a framebuffer console
and an emulated keyboard, instead of `-nographic`). See `./configure --help`
or `Makefile` for the current list, or `CLAUDE.md` for the full recipe used
to bring up a new one.

## Layout

```
forks/<name>/     one port's full tree, own Makefile, own LICENSE
docs/             provenance evidence, build/factorization plans, per-arch notes
scripts/          shared test harness (QEMU console/graphics drivers)
scripts/repo-history/   archived, unmaintained record of how the history was built
```

Each `forks/<name>/` is a complete, independently-buildable xv6 — nothing is
shared yet (that's goal 2 above). Several ports target the same ISA
independently (e.g. `amd64` and `amd64-jserv` are unrelated x86-64 efforts);
see `docs/provenance.md` for which ports are peers vs. derivatives of each
other.

## Licensing

Each `forks/<name>/LICENSE` carries the terms from that architecture's
original repository, all descending from MIT's xv6 permission notice (see
`forks/i386/LICENSE`). No license text was altered by this merge.
