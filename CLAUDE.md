# CLAUDE.md

This file provides guidance to Claude Code when working with code in this
repository.

## What this is

`xv6-multiarch` unifies 14 independent xv6 ports - covering eight
instruction sets - into one repository, with git history stitched together
so `git blame -C -C` traces every unchanged line back to its real original
commit (even across ports that started life as a fresh `git init` with no
shared history). See the root `README.md` for the short pitch and current
build/boot status, and `docs/provenance.md` for the full fork-point table
and the evidence behind each "inferred" fork point. Don't duplicate that
material here - read it there.

## Current status and goal

Two sequential efforts, in order:

1. **`docs/claude_notes/build-and-test-plan.md`** - get each of the
   14 `forks/<name>/` ports actually building and booting under
   QEMU on this machine, with the result pinned reproducibly. **Phases 1
   (riscv64), 2 (i386), 3 (Docker), and 5 (CI) are done, and Phase 4's
   original ten-port list is done too** (`forks/d1`, the eleventh item on
   that list, was evaluated and then removed - build-only, no QEMU
   target, no logic `forks/riscv64` didn't already have). Two more ports
   surfaced after Phase 4 was scoped - `arm-pi3` and `arm64-pi4` - and
   are still being wired up; see "Adding a new arch" below for the
   recipe, and `docs/claude_notes/notes_arch_*.txt` for what's been
   verified about each arch so far.
2. **`docs/claude_notes/factorization-plan.md`** - once ports build and
   boot, factor the near-duplicate trees into a Linux-style layout
   (`user/`, `kernel/`, `include/` shared; `arch/<name>/` per-port).
   **Blocked on (1)** - do not start merging files across ports before
   they're each independently verified to build and boot; that is
   exactly how this repo's abandoned predecessor
   (`gitlab.com/xv6-multiarch`) died.

Read both plans before doing substantial work in this repo - they encode
real decisions (why riscv64 first, why per-arch files beat `#ifdef`, the
blame-preservation rule for the eventual merge) that are easy to
re-litigate by accident otherwise.

## Build and run

```sh
./configure          # detects each wired-up arch's toolchain + qemu-system-*,
                      # writes Makefile.config (generated, gitignored)
make build-riscv64    # build one arch
make run-riscv64      # build + boot interactively (-nographic; see below
                      # for how to exit)
make test-riscv64     # build + boot headless + run that port's own
                      # usertests suite, assert "ALL TESTS PASSED"
make build-all / test-all / clean-all / kill-all   # same, across every
                      # wired-up arch
make build-docker [ARCH=riscv64]   # same, inside the pinned Dockerfile -
                      # ARCH defaults to "all"
```

Same shape for every other wired-up arch (`i386` today; extend as Phase 4
lands more). `make kill-<arch>`/`make kill-all` cleans up an orphaned
`qemu-system-*` process left running after an interrupted `run-<arch>`/
`test-<arch>`.

**Exiting an interactive `run-<arch>` session:** `-nographic` attaches the
guest's serial console directly to your terminal - ordinary Ctrl-C/Ctrl-D
go to the xv6 shell, not QEMU. Use **Ctrl-A then X** to quit QEMU, or
**Ctrl-A then C** to reach the QEMU monitor first.

## Architecture naming convention

Makefile targets and `./configure`'s own `TOOLPREFIX_<ARCH>`/`QEMU_<ARCH>`
variables use the bare ISA name (`riscv64`, `i386`, ...), matching
`~/goken/include/arch/` and `~/c--`'s `CC<ARCH>`/`RUN_<ARCH>` convention.
For most ports this still differs from the `forks/<name>/` directory
name - those directories are deliberately **not** renamed to arbitrary
short names: that would disturb the directory-content history the
eventual factorization phase depends on, for no benefit to this phase.
Two distinct kinds of rename ARE done, both preserving history (verified
via `git log --follow`/`git blame -C` after each one):

1. **Single-ISA-representative promotion.** `forks/riscv`, `forks/x86`,
   `forks/rv32`, `forks/aarch64`, and `forks/armv7-rpi` were renamed to
   `forks/riscv64`/`forks/i386`/`forks/riscv32`/`forks/arm64`/`forks/arm`
   outright, each because it's the single, unambiguous fork for that
   ISA/name - true from the start for the first four; true for
   `forks/arm` only once it beat the other three ARM32 candidates and
   was promoted (2026-09-08, see this file's own "Adding a new arch"
   section on why `armv7-rpi` specifically earned it), once the
   directory and target names could agree.
2. **ISA-prefixed grouping**, added 2026-09-08 and NOT a representative
   promotion (each still has its own separate official representative -
   `forks/arm`, `forks/arm64`, `forks/amd64`): `forks/rpi1`, `forks/rpi2`,
   `forks/armv6-rpi`, `forks/pi_mp`, `forks/rpi4`, and `forks/x86_64`
   were renamed to `forks/arm-pi1`, `forks/arm-pi2`, `forks/arm-pi1-bis`,
   `forks/arm-pi3`, `forks/arm64-pi4`, and `forks/amd64-jserv` - real,
   distinct peer/derivative ports grouped under a shared ISA prefix
   purely to make the eventual factorization-phase merge easier to
   reason about (related forks now sort together). `forks/arm-pi1` and
   `forks/arm-pi1-bis` target the literal same real board (ARMv6
   Raspberry Pi 1/Model B - zhiyihuang's and inaciose's independent
   ports, hence "-bis" for the second one); `forks/pi_mp`'s upstream
   README describes it as "AArch64" but its Makefile only ever builds
   32-bit ARM code (`arm-none-eabi-`, `-mcpu=cortex-a7`) - real
   multiprocessing on Pi 3 hardware, in AArch32 compatibility mode on
   its ARMv8 chip, not a 64-bit kernel - so it joined the `arm` prefix
   as `forks/arm-pi3`, not `arm64-pi3` as first (incorrectly) renamed
   2026-09-08 (caught and fixed the same day); `forks/arm64-pi4` (real
   Pi 4, genuinely AArch64) stayed under `arm64`; `forks/amd64-jserv` is
   jserv's independent, fully-working x86-64 port grouped alongside
   MIT's own `forks/amd64`, not a derivative of it. See
   `docs/provenance.md` for the upstream-repo -> current-forks-path
   mapping table.

## Adding a new arch (Phase 4)

Repeat, in order, for the next arch in `build-and-test-plan.md`'s Phase 4
list (`x86_64`, `amd64`, `rv32`, `aarch64`, `loongarch`, `mips`, the four
ARM Raspberry Pi ports, then `d1` build-only):

1. Read that port's own `Makefile` - its `TOOLPREFIX`/`CROSS_COMPILE` and
   `QEMU` auto-detect logic, if any, and whether it already ships its own
   test harness (`forks/riscv64/test-xv6.py` does; most don't).
2. Add a detection block to `./configure` (a `detect_toolprefix`/
   `detect_qemu_system` call pair - see the existing riscv64/i386 blocks)
   and a `build-<arch>`/`run-<arch>`/`test-<arch>`/`clean-<arch>`/
   `kill-<arch>` set to the top-level `Makefile`, folded into the
   `-all` umbrella targets.
3. Build. **Fix, don't route around, `-Werror` failures from modern GCC
   warnings that didn't exist when the port was last touched** - triage
   false-positive-vs-real-bug first (see
   `docs/claude_notes/notes_debugging_techniques.txt` item 1), then apply
   the narrowest fix: a named `-Wno-<warning>` for a confirmed false
   positive, or an actual code/constant fix for a real bug - each with a
   `claude:`-tagged comment explaining why. **`forks/<name>/` is fair
   game to edit** - the plan of record is to eventually factor these
   trees together, they will not stay as-is, so don't hold back changes
   trying to preserve them unchanged.
4. If the port has no test harness of its own, write a minimal
   `forks/<name>/test-xv6.py` modeled on `forks/riscv64/test-xv6.py`'s
   `QEMU` class (see `forks/i386/test-xv6.py` for the pared-down shape:
   usertests-only, no crash/log/orphan tiers, since older forks don't
   have those test programs at all).
5. Add a `docs/claude_notes/notes_arch_<name>.txt` recording what was
   actually found - toolchain gaps, GCC false positives fixed, real bugs
   found and why the chosen fix was safe, anything benign in the boot
   transcript worth not mistaking for a regression later (see the
   existing `notes_arch_riscv64.txt`/`notes_arch_i386.txt` for the
   shape - these are findings, written after doing the work, not a plan).
6. Add a `.github/workflows/docker.yml` matrix entry and a Dockerfile
   `ARCH` case, matching the existing riscv64/i386 ones - **verify
   locally with the exact `--build-arg ARCH=<name>` first** before
   pushing (see `notes_debugging_techniques.txt` item 6 for why: this
   caught a real missing-dependency bug and a too-tight test timeout
   that would otherwise have only shown up as a confusing CI failure).

## Testing conventions

Every wired-up arch's `test-<arch>` target boots the real kernel under
`qemu-system-*` (never a mock) and asserts a real, arch-specific success
string appears in the console output after sending a real command (see
`docs/claude_notes/notes_debugging_techniques.txt` and the `notes_arch_*`
files for exactly what each port's `usertests.c` does and why "ALL TESTS
PASSED" is produced differently in different ports - a direct `printf` in
some, an `exec("echo", ...)` trick in others). A benign trap/fault message
appearing mid-suite (a test deliberately provoking one to verify the
kernel handles it) is not a regression - check the relevant
`notes_arch_*.txt` before assuming otherwise.

## Other docs in the tree

- `README.md` - short pitch, current build/boot status, build commands
- `docs/provenance.md` - the evidence behind each fork point
- `docs/claude_notes/build-and-test-plan.md` - the build/boot/CI plan (this file's own "Current status" section tracks progress against it)
- `docs/claude_notes/factorization-plan.md` - the later Linux-style-unification plan, blocked on the above
- `docs/claude_notes/notes_arch_<name>.txt` - real bring-up findings, one per wired-up arch
- `docs/claude_notes/notes_debugging_techniques.txt` - general debugging methodology, grown from real investigations in this repo
