# Plan: cut testing time

**Status:** proposed, not started. Written 2026-09-12, after measuring where
`stress-test-all`'s wall time actually goes.

## The measurement

Ran all 13 CI-matrix arches' `docker build --build-arg ARCH=<arch>` **in
parallel** on this host (64 cores, 125GB RAM - no resource contention), each
timed independently with a `time`-wrapped background job. Each build does
exactly what CI does for a single-arch job: `./configure`, `make
build-<arch>`, then `make test-<arch>` (the full, non-quick suite - real
usertests boot under `qemu-system-*`, not the fast `quick-test-<arch>` smoke
check). `arm64-pi4` is excluded, same as the real CI matrix, since it needs a
locally-source-built QEMU ≥ 9.1 that Docker/CI images don't have (see
`plan_build_and_test_2.md` item 2).

**Caveat: this run's apt-get/toolchain-install layers were cache-hits** (4
`CACHED` steps in every log, confirmed by inspecting `riscv64.log`) - this
host already had layers from earlier local/CI builds. So the numbers below
are **build-and-boot cost only**, not a cold `docker build --no-cache` /
fresh-runner number, which would add a shared apt-get cost on top of every
row (bigger for `all`/`arm64`'s `ipxe-qemu` and `loongarch`'s
`gcc-14-loongarch64-linux-gnu` than for the lighter arches, but the same
order of magnitude for all of them since they hit one `apt-get update` +
one case-branch install each). If this plan is ever acted on based on a
different host or a cold CI run, re-measure with `--no-cache` first rather
than trusting these numbers verbatim.

Full logs are not committed (they lived in this session's scratchpad and
are not preserved) - re-run the command below to reproduce:

```sh
for a in i386 amd64 amd64-jserv riscv64 riscv32 arm arm-pi1 arm-pi1-bis \
         arm-pi2 arm-pi3 arm64 mips loongarch; do
  ( time docker build --build-arg ARCH="$a" -t "xv6-multiarch:$a" . \
      > "$a.log" 2>&1 ) 2> "$a.time" &
done
wait
```

## Results (2026-09-12, this host)

| arch | build (`make build-<arch>`) | test (`make test-<arch>`, boot+usertests) | total |
|---|---|---|---|
| **riscv64** | 6.6s | **552.0s** | 562s |
| **arm64** | 6.5s | 245.4s | 255s |
| **riscv32** | 5.2s | 207.1s | 216s |
| **arm-pi3** | 10.3s | 109.3s | 123s |
| loongarch | 8.4s | 79.1s | 92s |
| i386 | 7.0s | 77.2s | 87s |
| amd64-jserv | 7.6s | 59.5s | 70s |
| amd64 | 4.4s | 58.1s | 66s |
| arm-pi2 | 6.0s | 22.2s | 32s |
| arm-pi1-bis | 7.8s | 19.7s | 32s |
| arm | 4.4s | 13.0s | 21s |
| arm-pi1 | 8.5s | 10.6s | 22s |
| mips | 4.6s | 10.3s | 18s |

Two things fall out immediately:

1. **Build time is a non-issue.** 4-10s everywhere, essentially flat across
   every ISA and cross-toolchain. All the variance - and all the
   optimization opportunity - is in the QEMU boot + `usertests` step.
2. **`riscv64` is the outlier, not a smooth tail.** It is 2.2x `arm64` (the
   #2 slowest) and larger than the bottom nine arches *combined*. Since
   arches build in parallel as separate CI matrix jobs, **`stress-test-all`'s
   CI wall time is bounded by whichever single arch is slowest, i.e. by
   `riscv64` alone** (~9-10 minutes) - shaving time off any other arch buys
   nothing for the matrix's total wall clock unless `riscv64` also moves.

## Is `riscv64`'s 552s a bug, or real cost?

**Real cost, already diagnosed - not a target for a "speed fix".**
`forks/riscv64/test-xv6.py`'s own `test_usertests()` carries a `claude:`
comment from an earlier session recording exactly this: a passing run
measured ~570s wall-clock (matching this run's 552s), because `riscv64`'s
`usertests.c` is the one fork whose suite runs real disk-stress subtests
(`diskfull`, `outofinodes`) near the end - confirmed present in this run's
own transcript (`test diskfull: balloc: out of blocks` at t=424.9s, `test
outofinodes: ialloc: no inodes` at t=509.0s, only ~43s before `ALL TESTS
PASSED` at 552.0s). An earlier commit (`647cc47`) blanket-dropped every
fork's `test-xv6.py` timeout to 300s without measuring `riscv64`
specifically, which would have made a genuinely-passing run look like a
hang; the timeout was restored to 600s for this reason. **Don't re-attempt
that cut** - see `notes_debugging_techniques.txt` before touching this
timeout again.

So `riscv64` is slow because its usertests suite legitimately does more
I/O-stress work than its siblings, not because something is broken. The
options below are about hiding or amortizing that cost, not eliminating it.

## Per-test breakdown, top 3 slowest arches (2026-09-12)

Getting a real per-test breakdown required a tooling fix first:
`arm64`/`riscv32` use the shared `scripts/qemu_console.py` harness, whose
`wait_for()` printed every console line without flushing - under `docker
build` (not a tty), Python block-buffers stdout, so every `test X: OK`
line landed in the build log at the *same* final second regardless of
when it actually happened, making a breakdown impossible from those logs
as they stood. `riscv64` has its own separate harness
(`forks/riscv64/test-xv6.py`) with the same gap. Both were fixed to flush
after every printed line (commit `5b80633`) - a permanent, low-risk
tooling improvement (useful for any future timing/hang diagnosis, not
just this), not a test-semantics change. Re-running `riscv64` alone after
its own fix reproduced the original numbers almost exactly (565.2s vs.
552.0s originally, same milestones within ~1-4s), confirming the first
pass's numbers were already trustworthy and this is just belt-and-suspenders.

**One methodology caveat that matters for reading the tables below:**
`riscv64`'s own harness only prints lines matching a `progress='test'`
filter (lines starting with `"test "`), so a bare `OK`/`FAILED` line -
which starts with neither - never gets its own timestamp. For every test
except the *last* one in a run, this doesn't matter: the interval between
one `"test X:"` print and the next necessarily spans all of test X's
work (fork, run, wait, print OK, then the next test's own `"test Y:"`
print). But for the **last** test in a run, there is no following
`"test:"` print to close the interval - what's measured instead is that
last test's own invisible completion *plus* whatever the harness does
afterward (`countfree()`'s leaked-page check, then `ALL TESTS PASSED`),
bundled into one number. `arm64`/`riscv32`'s shared harness has no such
filter (it prints unconditionally), so their tables don't have this
blind spot - which is exactly why their own trailing gaps come out tiny
(7.4s, ~0s) while `riscv64`'s does not (see below) - not because
`riscv64`'s `countfree()` is special, but because its last-test tail is
partly hidden inside that same number.

### `riscv64` (552-565s total, slowest)

`slowtests[]`, in `forks/riscv64/tests/usertests.c`:

| test | duration | what it does |
|---|---|---|
| `badwrite` | ~100s | 600x create+write(invalid ptr)+unlink, checking for a block leak |
| `outofinodes` (+ tail) | ~84s + ~43s | creates up to 1024 files (only ~200 succeed, `NINODES=200`), then **unconditionally `unlink()`s all 1024 names** even though ~824 never existed; the +43s tail is this test's own invisible completion plus `countfree()` (see caveat above) |
| `bigdir` | ~69-73s | `N=500`: link 500 names into one directory, then unlink all 500 |
| `execout` | ~60-61s | 15 children, each exhausts all 128MB of RAM via `sbrk`, frees `avail` pages, execs |
| `manywrites` | ~45-46s | 4 children x 30 iterations of create/write/unlink |
| `diskfull` | ~18-19s | fills the disk - already fast, because `FSSIZE=2000` blocks is already small |

### `arm64` (245s total)

Shared `tests/usertests-arm64.c` (also used by `riscv64` for these three
functions - see below). Order differs from `riscv64`'s: `manywrites`/
`execout` run early, interleaved with the fast tests; `bigdir` is last.

| test | duration | what it does |
|---|---|---|
| `bigdir` | 59.2s | same `N=500` code as `riscv64` |
| `execout` | 55.0s | same code as `riscv64` |
| `manywrites` | 15.8s | same code as `riscv64` |
| trailing tail | 7.4s | genuinely small - no `badwrite`/`diskfull`/`outofinodes` in this fork's suite at all, and the harness has no last-test blind spot |

### `riscv32` (207-216s total)

Its own standalone `tests/usertests.c` - **no `manywrites`, no
`execout`, no `diskfull`/`outofinodes`, and `badwrite` is commented out**
in the test array. The only slow entry is `bigdir` (same `N=500` code,
byte-for-byte identical to `riscv64`'s and `arm64`'s):

| test | duration | what it does |
|---|---|---|
| `bigdir` | **~116s** | same `N=500` code as the other two - and yet **1.6-2x slower** than `riscv64`'s or `arm64`'s own `bigdir` |
| everything else (all ~40 fast tests) | ~92s total | actually *faster* to reach than `arm64`'s equivalent point (177.8s) despite `riscv32` being the 32-bit target |
| trailing tail | ~0s | `bigdir`'s own `OK` and `ALL TESTS PASSED` landed in the same read cycle |

**The `bigdir` finding is the single most useful thing this breakdown
turned up.** It's the exact same code (`enum { N = 500 }`, same
`link()`/`unlink()` loop, confirmed byte-identical by diffing the three
forks' own `bigdir()` functions) costing 59s / 69-73s / 116s on three
different arches. That rules out "the test code got bloated on one
fork" - the code isn't different - and points at each arch's own
QEMU/TCG emulation and/or that fork's own `fs.c` codegen as the real
variable. Checked `riscv32/kernel/fs.c` against `riscv64/kernel/fs.c`
directly: the directory-lookup logic (`dirlookup()`, linear scan) is
algorithmically identical between them, differing only in `uint32` vs.
`uint64` pointer types (32-bit vs. 64-bit target) - so this isn't an
`fs.c` algorithm difference either. **Why `riscv32` pays ~1.6-2x more for
the identical `bigdir` workload while running its *other* ~40 tests
faster than `arm64` is an open question**, not yet root-caused - flagged
here rather than guessed at.

One structural fact worth using either way: `dirlookup()` is a **linear
scan**, and `bigdir`'s loop calls `link()` (which calls it) once per
iteration against an ever-growing directory - so its total work is
`1+2+...+N`, i.e. **O(N²)**, not O(N). Unlike the O(N) tests above
(`badwrite`/`outofinodes`/`manywrites`), halving `N` here doesn't halve
the cost, it roughly quarters it. That makes `bigdir`'s `N=500` the
single highest-leverage constant of everything found in this session,
*if* any constant is to be tuned - it's the one test that's both (a) the
most expensive single item in all three top-3 arches and (b) shared,
byte-identical code across at least `riscv64`/`arm64` already (so one
change helps both at once), and (c) quadratic, so a modest cut goes
further than in any of the linear tests.

### What this changes about the earlier constant-by-constant notes

Everything the first pass said about `outofinodes` (shrink `nzz` toward
`NINODES`), `execout` (shrink the `avail` range), `manywrites` (some
slack in `howmany`), and `badwrite` (do **not** lower `assumed_free` -
it may already be undersized relative to `FSSIZE`'s real free-block
count) still stands, and still only applies to `riscv64`/`arm64` (the
only forks that run those tests at all). `bigdir` is the addition: it is
now the *first* thing to try, ahead of those, precisely because it's
common to all three slowest arches and quadratic.

None of the constant changes discussed anywhere in this file have been
applied or measured yet - this is pre-implementation analysis. If
pursued, each change needs its own before/after `stress-test-<arch>`
timing and a sanity check that the test still fails on a deliberately
reintroduced version of the bug it guards (e.g. temporarily break
block-freeing and confirm `badwrite` still panics before trusting a
lowered `assumed_free`).

### The other, orthogonal lever: disk cache mode

Across `badwrite`/`outofinodes`/`bigdir`/`manywrites`, every filesystem-
modifying syscall costs roughly 80-170ms wall-clock under this QEMU/TCG
setup - disk-I/O-bound, not CPU-bound. `forks/riscv64/Makefile:215`'s
`-drive file=fs.img,if=none,format=raw,id=x0` (and the equivalent lines
in `arm64`'s/`riscv32`'s own Makefiles) set no `cache=` mode. Since each
test's `fs.img` is regenerated by `mkfs` on every build and thrown away
afterward - no durability requirement for a throwaway test image, unlike
a real flashed board - a faster cache mode is a **candidate free win
that touches zero test semantics**, and unlike `bigdir` it isn't limited
to one test: it would apply to every filesystem-heavy test on every
arch. Worth measuring before touching any usertests.c constant. Not yet
tried.

## Options, cheapest/safest first

1. **Do nothing - the matrix already parallelizes this.** CI wall time is
   `max(arch)` not `sum(arch)`; ~9-10 min for `stress-test-all`-equivalent
   coverage across 13 arches is arguably fine as-is. Worth stating
   explicitly since it's the one-line answer to "why is CI slow" even
   before touching `riscv64`.
2. **Try a faster disk `cache=` mode on the test-only `-drive`,** across
   all three top-3 forks at once. Free (no coverage change), applies to
   every filesystem-heavy test in every arch, not just one - do this
   before anything else on this list.
3. **Shrink `bigdir`'s `N=500`** (e.g. to ~150-200), in whichever of
   `tests/usertests-arm64.c` / `forks/riscv64/tests/usertests.c` /
   `forks/riscv32/tests/usertests.c` actually needs editing. Highest
   single-constant leverage found: it's the most expensive test in *all
   three* top-3 arches, and its cost is quadratic in `N` (linear-scan
   `dirlookup()` on an ever-growing directory), so a modest cut goes
   further than in any of the linear tests below. Changing the shared
   `tests/usertests-arm64.c` copy helps `riscv64` and `arm64` in one edit;
   `riscv32`'s own copy (not yet folded into that shared cluster) needs
   its own, identical edit. Needs a floor check first: `N` must stay large
   enough to still force the directory past however many entries fit in
   one block, or the test stops meaningfully exercising "big directory".
4. **Split `riscv64`'s own usertests into two parallel CI steps/jobs**: one
   running the bulk of `usertests` (excluding `diskfull`/`outofinodes`),
   one running just the disk-stress tail, each in its own QEMU boot. This
   would cut `riscv64`'s own critical path roughly in half at the cost of
   a second QEMU boot's fixed overhead and a small Makefile/CI change.
   `forks/riscv64/test-xv6.py`'s `test_usertests(test=)` already takes a
   test-name filter (used by `args.testrex`), so the plumbing to run a
   subset already exists - this would mostly be a Makefile/CI wiring
   change, not new test-harness code. Only helps `riscv64`; doesn't touch
   `arm64`/`riscv32`, which don't have this pair of tests at all.
5. **Shrink the other, arch-specific constants** - `outofinodes`'s
   `nzz` toward `NINODES` (`riscv64` only), `execout`'s `avail` range
   (`riscv64`/`arm64`), some slack out of `manywrites`'s `howmany`
   (`riscv64`/`arm64`). Do **not** lower `badwrite`'s `assumed_free` - it
   may already be undersized relative to `FSSIZE`'s real free-block
   count; compute that count before touching it either way. Lower
   priority than `bigdir` since each of these is O(N) (linear payoff for
   the cut, not quadratic) and touches at most two of the three top-3
   arches.
6. **Root-cause why `riscv32`'s `bigdir` costs 1.6-2x `riscv64`'s/`arm64`'s
   for the identical workload**, given its own `fs.c` directory-lookup
   code was checked and found algorithmically identical (only `uint32` vs.
   `uint64` pointer-type differences). Not yet investigated - candidates
   worth checking first: `riscv32`'s specific QEMU machine/CPU model
   flags, and whether its binaries are built at a different optimization
   level than `riscv64`'s. Valuable regardless of whether `bigdir`'s `N`
   is also lowered, since it's a real, unexplained 2x that would persist
   proportionally even after any constant change.
7. **Investigate whether `riscv64`/`arm64`/`riscv32`'s QEMU invocations can
   use a faster accelerator.** They're always TCG-emulated on this x86_64
   host (only `amd64`/`i386`/`amd64-jserv` could ever use KVM, since guest
   ISA must match host ISA) - itself part of why the non-x86 arches
   cluster at the slow end, and not fixable for the actual bottleneck
   arches. Worth confirming `amd64`/`i386` aren't accidentally on TCG too,
   as a separate, low-risk win - it does nothing for `riscv64` itself.
8. **Re-measure cold** (`--no-cache`, or on a fresh CI runner) before
   deciding any of the above is worth the effort - the apt-get/cache
   caveat in "The measurement" above means this session's numbers may
   understate the *shared* fixed cost every arch pays, which none of
   these options touch.

## Definition of done

Not defined yet - this is a proposal, not a committed plan. Options 2
(disk cache mode) and 3 (`bigdir`'s `N`) have the best effort/payoff
ratio - both are cheap, both apply to all three top-3 arches at once, and
neither risks weakening what any test actually proves (cache mode changes
nothing about test semantics; `bigdir`'s coverage-preserving floor just
needs establishing before cutting `N`). Pick a starting point and turn
this section into an actual target (e.g. "`stress-test-all`'s CI wall
time under N minutes", or "`riscv64`'s own `test-riscv64` under N
minutes") before starting work.
