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

## Where this is going

Two sequential goals:

1. **Get every port building and booting under QEMU**, with the result
   pinned reproducibly (toolchain, QEMU version, a real usertests run per
   arch — not a mock). Most ports are there; the rest are being brought up
   one at a time. See
   [`docs/claude_notes/plan_build_and_test.md`](docs/claude_notes/plan_build_and_test.md)
   for exactly which and what's left.
2. **Factor the near-duplicate trees into a Linux-style layout** —
   `user/`, `kernel/`, `include/` shared, `arch/<name>/` per port —
   once (1) is done. Blocked on (1): merging files you haven't verified
   still build is how this repo's abandoned predecessor
   (`gitlab.com/xv6-multiarch`) died. See
   [`docs/claude_notes/plan_factorization.md`](docs/claude_notes/plan_factorization.md).

## Build and run

```sh
./configure          # detects each wired-up arch's toolchain + qemu-system-*
make build-riscv64    # build one arch
make run-riscv64      # build + boot interactively (Ctrl-A X to quit QEMU)
make test-riscv64     # build + boot headless + run that port's usertests suite
make build-all / test-all / clean-all   # same, across every wired-up arch
make build-docker [ARCH=riscv64]        # same, inside the pinned Dockerfile
```

Same shape for every other wired-up arch. See `./configure --help` or
`Makefile` for the current list, or `CLAUDE.md` for the full recipe used to
bring up a new one.

## Layout

```
forks/<name>/     one port's full tree, own Makefile, own LICENSE
docs/             provenance evidence, build/factorization plans, per-arch notes
scripts/          archived, unmaintained record of how the git history was built
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
