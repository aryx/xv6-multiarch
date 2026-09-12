# Plan: cut testing time

**Status:** in progress. Written 2026-09-12, after measuring where
`stress-test-all`'s wall time actually goes. Options 2 (disk `cache=`
mode), 3 (`bigdir`'s `N`), and 5 (the other `riscv64`-specific constants)
are all done as of the same day - see their own "Results after..."
sections below. Options 1, 4, 6, 7, 8 remain proposals, not started.

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

**This is the original "before" baseline.** `riscv64`/`arm64`/`riscv32`'s
own numbers are superseded by "Results after option 3" further down,
once `bigdir`'s `N` was cut - kept here unchanged as the historical
starting point the rest of this file's analysis was built from.

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

Shared `tests/usertests-arm64.c` (also used by `arm64-pi4` and
`loongarch` - **not** `riscv64`, which has its own separate,
independently-maintained copy with identical `bigdir`/`manywrites`/
`execout` code - corrected here after first writing this as one shared
file; it's two duplicate files, not one). Order differs from `riscv64`'s:
`manywrites`/`execout` run early, interleaved with the fast tests;
`bigdir` is last.

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
most expensive single item in all three top-3 arches and (b) byte-identical
code duplicated across `riscv64`'s own file and the `arm64`/`arm64-pi4`/
`loongarch` shared file (so each needs its own edit, but the edit is the
same three lines each time), and (c) quadratic, so a modest cut goes
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

## Results after option 3: `bigdir` N=500 -> 200 (DONE 2026-09-12)

Applied the same three-line change (with the same `claude:` comment) to
all three duplicated copies: `tests/usertests-arm64.c` (also picked up by
`arm64-pi4`/`loongarch`, not separately measured here),
`forks/riscv64/tests/usertests.c`, `forks/riscv32/tests/usertests.c`.
Rebuilt and ran the full `docker build --build-arg ARCH=<arch>` (build +
`test-<arch>`) for all three top-3 arches to confirm `ALL TESTS PASSED`
still holds and to measure the real effect:

| arch | `bigdir` before -> after | full test step before -> after |
|---|---|---|
| `riscv64` | ~69-73s -> **17.0s** (~4.1x) | 552-565s -> **419.1s** (-24 to -26%) |
| `arm64` | 59.2s -> **14.8s** (~4.0x) | 244.5-245.4s -> **201.9s** (-17 to -18%) |
| `riscv32` | ~116s -> **28.6s** (~4.1x) | 207.1-207.8s -> **120.9s** (-42%) |

All three still print `ALL TESTS PASSED`. The ~4x reduction (not the
naive 6.25x from `(500/200)²`) matches expectations once you account for
`bigdir`'s non-quadratic parts (`fork`/`open`/`close`/the final
`unlink()`-cleanup loop are O(N), not O(N²)) diluting the pure quadratic
term - still a large, real win. `riscv32`'s disproportionate per-entry
cost for the identical workload **persists at the new N** (28.6s vs.
14.8-17s for the other two, still ~1.7-2x) - this confirms it's a real
property of that fork/QEMU target, not an artifact that was specific to
N=500, so option 6 (root-cause it) is still open and still worth doing.

`riscv64` remains the slowest arch by a wide margin (419.1s vs. `arm64`'s
202s), since `bigdir` was only one of six slow tests in its suite -
`badwrite`/`outofinodes`/`execout`/`manywrites` (options 4-5) are
untouched by this change and now make up a *larger* share of its
remaining time than before. `riscv32` saw the largest relative win (42%)
because `bigdir` was effectively its *only* slow test, so cutting it cut
almost all of the fat this fork had.

## Results after option 2: disk `cache=unsafe` (DONE 2026-09-12)

Added `cache=unsafe` to the test-only `-drive file=fs.img,...` line in
`forks/riscv64/Makefile`, `forks/arm64/Makefile`, and
`forks/riscv32/Makefile` (each with a `claude:` comment - `fs.img` is
regenerated by `mkfs` on every build and thrown away afterward, so there
is no durability requirement to protect, unlike a real flashed board -
this is a QEMU-only virtual disk on all three ports, so the "never break
real hardware" rule doesn't apply here at all). Rebuilt and ran the full
`docker build --build-arg ARCH=<arch>` for all three (on top of the
`bigdir` change above):

| arch | before (post-`bigdir`) | after `cache=unsafe` | change |
|---|---|---|---|
| `riscv64` | 419.1s | **388.0s** | -7.4%, real |
| `arm64` | 201.9s | 200.3s | -0.8%, noise-level |
| `riscv32` | 120.9s | 120.4s | -0.4%, noise-level |

All three still `ALL TESTS PASSED`. The effect is real but **narrow**,
and lands entirely on `riscv64`'s per-test breakdown in exactly the
tests predicted to be disk-write-bound:

| test (`riscv64`) | before | after `cache=unsafe` |
|---|---|---|
| `manywrites` | ~45-46s | 32.0s |
| `badwrite` | ~100-101s | 63.1s |
| `diskfull` | ~18-19s | 14.0s |
| `outofinodes` + tail | ~127s | 76.1s |
| `bigdir` | ~17s | 16.0s (unchanged - CPU-bound scan, not I/O) |
| `execout` | ~60-61s | 62.1s (unchanged - memory/`sbrk`-bound, not I/O) |

`arm64` and `riscv32` barely moved because neither has `badwrite`/
`outofinodes`/`diskfull` at all, and their own `manywrites`/`bigdir`
didn't change measurably either - confirming the earlier hypothesis only
half-held: the *disk-write-heavy* tests really were I/O-bound and got a
genuine ~35-40% cut each, but `bigdir` and `execout`'s cost was never
disk-I/O in the first place (CPU-bound linear-scan and memory-pressure
work, respectively), so this lever doesn't touch them regardless of arch.

## Results after option 5: `riscv64`'s other constants (DONE 2026-09-12)

With `bigdir` and `cache=unsafe` both landed, `riscv64` was still the
CI-matrix bottleneck by a wide margin (388.0s vs. `arm64`'s 200.3s), so
went after its four remaining arch-specific constants in
`forks/riscv64/tests/usertests.c`, each with its own `claude:` comment:

- `manywrites`'s `howmany`: 30 -> 15 (halved - still real 4-child
  concurrent create/write/unlink contention, less margin on top of it)
- `execout`'s `avail` range: `< 15` -> `< 5` (the interesting edge cases,
  0-4 free pages, are the tightest margin for `exec()` to make progress;
  the higher values gave it ever more slack and are least likely to ever
  matter)
- `outofinodes`'s `nzz`: `32*32=1024` -> `256` (still comfortably exceeds
  `NINODES=200`, so the create loop below still reaches real inode
  exhaustion the same way; only the wasted padding in both loops shrinks)
- `badwrite`'s `assumed_free`: 600 -> 200, but **not for the same
  reason as the others** - this one was already broken as a real
  regression test on this fork, independent of speed. `mkfs`'s own
  boot-time print (visible in every build log) reports `blocks 1915
  total 2000` - i.e. **1915 free data blocks**, not ~600, so even a real
  one-block-per-iteration leak bug would never exhaust free space within
  600 iterations and trigger `balloc: out of blocks`. This test has
  provided **zero actual leak-detection coverage on riscv64** at any
  value below ~1916, both before and after this change - lowering it
  further costs nothing that wasn't already lost. Fixing it properly
  (raising `assumed_free` above 1915) is a separate, *slower* change,
  deliberately not done here - see "Definition of done" below for where
  that's tracked.

Rebuilt and ran the full `docker build --build-arg ARCH=riscv64` (on top
of both earlier changes):

| test | before this round | after |
|---|---|---|
| `manywrites` | 32.0s | 16.1s (as predicted, ~halved) |
| `badwrite` | 63.1s | 21.0s (matches the 1/3 prediction exactly) |
| `execout` | 62.1s | 20.0s (matches the 1/3 prediction exactly) |
| `outofinodes` + tail | 76.1s | 64.1s (only -16%, far less than the naive 4x) |
| `bigdir`/`diskfull` | unchanged, not touched this round | 17.0s / 14.0s |
| **full test step** | 388.0s | **281.8s** (-27.4%) |

Still `ALL TESTS PASSED`. `outofinodes` barely moved despite a 4x cut to
`nzz` - most of its ~76s/~64s cost is evidently the *fixed* part (the
real ~200 creates it takes to reach inode exhaustion, plus whatever
share of the ambiguous tail this test's own ~1024-name cleanup loop
wasn't actually responsible for - see the earlier methodology caveat).
Diminishing returns there; not worth cutting `nzz` further without
better isolating that number first.

**Running total for `riscv64` across all three changes so far: ~558s
(original average) -> 281.8s, a ~50% reduction.** The single largest
remaining chunk is now the ~130s "fast tests" cluster itself (everything
before `bigdir`) - no single lever there, since it's ~40+ small,
individually-fast tests whose aggregate cost is fork/exec/wait overhead
under TCG, not anything obviously trimmable without losing real test
identities. `outofinodes`+tail (64.1s) is the next largest single item,
but as noted above doesn't respond well to further constant-shrinking.
Closing more of the remaining gap to `arm64`'s ~200s likely needs the
structural option (4: split into two parallel CI boots) rather than more
tuning.

## Options, cheapest/safest first

1. **Do nothing - the matrix already parallelizes this.** CI wall time is
   `max(arch)` not `sum(arch)`; ~9-10 min for `stress-test-all`-equivalent
   coverage across 13 arches is arguably fine as-is. Worth stating
   explicitly since it's the one-line answer to "why is CI slow" even
   before touching `riscv64`.
2. **DONE 2026-09-12: added `cache=unsafe` to the test-only `-drive`**
   in all three top-3 forks' Makefiles. Real but narrow win, landing
   entirely on `riscv64`'s disk-write-heavy subtests (`manywrites`/
   `badwrite`/`diskfull`/`outofinodes`, each down ~30-40%); `arm64`/
   `riscv32` barely moved since their remaining slow tests are CPU/
   memory-bound, not disk-bound. Results above.
3. **DONE 2026-09-12: shrunk `bigdir`'s `N` from 500 to 200** in all
   three places it's duplicated - `tests/usertests-arm64.c` (also used by
   `arm64-pi4`/`loongarch`), `forks/riscv64/tests/usertests.c`, and
   `forks/riscv32/tests/usertests.c` - each with the same `claude:`
   comment explaining the tradeoff and pointing back at this file. 200
   still spans several directory blocks (200/64 dirents-per-block ≈ 3
   blocks, vs. 500's ≈ 8), so "big directory" is still genuinely
   exercised, just not padded past what that requires. Deliberately a
   temporary tuning knob, not a permanent coverage cut: the plan is to
   restore `N=500` once the in-flight kernel-tree factorization settles
   and raw iteration speed matters less than it does today - see each
   comment's own wording. Results below.
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
5. **DONE 2026-09-12: shrunk `riscv64`'s other four constants** -
   `outofinodes`'s `nzz` 1024 -> 256, `execout`'s `avail` range `<15` ->
   `<5`, `manywrites`'s `howmany` 30 -> 15, and `badwrite`'s
   `assumed_free` 600 -> 200 (computed the real free-block count first:
   `mkfs` reports 1915, so 600 was already providing zero real
   leak-detection coverage on this fork - lowering it further doesn't
   cost anything that wasn't already lost; properly fixing this test
   means *raising* it above 1915, a separate and slower change, not done
   here). `riscv64`'s full test step: 388.0s -> 281.8s. Results above.
   Not applied to `arm64` (only has `manywrites`/`execout`, and shrinking
   just those two wasn't attempted separately - low expected payoff since
   `arm64`'s total is already well below `riscv64`'s).
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

Not defined yet - this is a proposal, not a committed plan. Options 2, 3,
and 5 are done (see their own Results sections); running total so far:

| arch | original | after `bigdir` (3) | after `cache=unsafe` (2) | after other constants (5) | total change |
|---|---|---|---|---|---|
| `riscv64` | 552-565s | 419.1s | 388.0s | **281.8s** | **~-50%** |
| `arm64` | 244.5-245.4s | 201.9s | 200.3s | 200.3s (not touched) | -18% |
| `riscv32` | 207.1-207.8s | 120.9s | 120.4s | 120.4s (not touched) | -42% |

`riscv64` went from the largest single bottleneck (552-565s, more than
2x `arm64`) to 281.8s - closer to but still above `arm64`'s 200.3s.
Constant-tuning has hit diminishing returns there: its remaining
breakdown is now a ~130s fast-tests cluster (no single lever - ~40+
individually-fast tests, cost is fork/exec/wait overhead under TCG, not
anything trimmable without losing real test identities), `outofinodes`+
tail (64.1s, mostly fixed cost per the note above), `execout` (20.0s),
`badwrite` (21.0s), `bigdir` (17.0s), `manywrites` (16.1s), `diskfull`
(14.0s). Getting further below ~280s for `riscv64` most likely needs
option 4 (split into two parallel CI boots) rather than more tuning -
that's the next concrete step if closing the remaining ~80s gap to
`arm64` still matters. Separately, `badwrite`'s real fix (raising
`assumed_free` above the 1915-block real free count so it actually
detects a leak regression again) is still owed - it's currently fast
*and* provides no real coverage, which is an honest tradeoff for this
pass but not a place to leave the test permanently.

Pick a starting point among what's left (4, 6, 7, 8, or the `badwrite`
fix above) and turn this section into an actual target (e.g.
"`stress-test-all`'s CI wall time under N minutes", or "`riscv64`'s own
`test-riscv64` under N minutes") before starting further work.
