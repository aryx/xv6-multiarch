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

1. **`docs/claude_notes/plan_build_and_test.md`** - get each of the
   14 `forks/<name>/` ports actually building and booting under
   QEMU on this machine, with the result pinned reproducibly. **Phases 1
   (riscv64), 2 (i386), 3 (Docker), and 5 (CI) are done, and Phase 4's
   original ten-port list is done too** (`forks/d1`, the eleventh item on
   that list, was evaluated and then removed - build-only, no QEMU
   target, no logic `forks/riscv64` didn't already have). Two more ports
   surfaced after Phase 4 was scoped - `arm-pi3` and `arm64-pi4`.
   `arm-pi3` is now wired up (`build-arm-pi3`/`run-arm-pi3`) and boots
   all 4 cores deep into userinit under QEMU - twelve real bugs found
   and fixed (most recently the same missing-`-fno-pic` gap
   `arm-pi2` had, and a trapframe fix so a Data Abort's own diagnostic
   print shows the real fault address), one open: a non-deterministic
   SMP hang/crash, re-characterized (not yet root-caused) in a second
   session - now known to happen as early as right after `tvinit()`,
   not only in the later `memmove()`/`balloc()` path session 1 first
   found - see `notes_arch_arm_pi3.txt`'s own "Open gap" for the full
   diagnosis and the concrete next lead. `arm64-pi4` is now wired up
   **build-only** - the only arch here that is - and builds clean
   (`make build-arm64-pi4` produces both `kernel/kernel` and the
   real-hardware `kernel8.img`): two narrow `-Wno-` flags for
   modern-GCC false positives, plus a real fix replacing its
   `-DRPI4_QEMU` compile-time switch with a runtime `CurrentEL` check,
   so one `kernel8.img` is now correct on both a real Pi 4 (entered at
   EL2 by the firmware's armstub) and under QEMU (entered at EL3), per
   the "prefer runtime detection over `#ifdef`" rule below. It has
   never been booted: QEMU only gained a Pi 4 board (`-M raspi4b`) in
   9.1 and this host is still on 8.2.2 as of 2026-09-09, so
   `run-arm64-pi4` exists but refuses with that explanation, and there
   is deliberately no `test-arm64-pi4`/`test-xv6.py` and no CI entry
   yet. See `notes_arch_arm64_pi4.txt`'s own "Open gap" for the two
   unverified guesses (machine name, `-m` size) to settle first the day
   a newer QEMU lands. See "Adding a new arch" below for the recipe,
   and `docs/claude_notes/notes_arch_*.txt` for what's been verified
   about each arch so far.

   Beyond Phase 4's own "build and boot" bar: `arm-pi1` was taken all
   the way to a genuinely interactive shell under QEMU, with a real
   emulated HDMI framebuffer console (`make run-arm-pi1-qemu-graphics`)
   and a working USB keyboard (`-device usb-kbd`, typed keystrokes
   execute real shell commands, verified end to end) - ten real bugs
   found and fixed across four sessions, see `notes_arch_arm_pi1.txt`.
   `arm-pi1-bis` - same real-hardware board family as `arm-pi1` - was
   taken to the same fully-working state (interactive shell, graphics,
   USB keyboard) via the identical fix pattern, see
   `notes_arch_arm_pi1_bis.txt`. `arm-pi2` (a different, more mature
   board-family member - real ARMv7/Cortex-A7, `hw=rpi2`) reaches full
   Phase-4 parity with its siblings too now: a real interactive shell
   under QEMU and a full `usertests` run reporting **"ALL TESTS
   PASSED"** - nine distinct real bugs found and fixed (an ARM/Thumb
   interworking gap and a PIC/PIE-codegen gap, both in its own
   hand-written `entry.S`/toolchain defaults and independently again in
   its user-space programs' own build, a missing VFP/NEON coprocessor
   enable, the same missing-`.bss`-zeroing bug as its siblings, the
   same QEMU-uses-PL011-not-mini-UART finding as `arm-pi1`/`arm-pi1-bis`
   - both TX and RX - an invalid Non-Secure-to-Monitor-mode switch, and
   a filesystem `MAXFILE` limit too small for a modern-toolchain-built
   `usertests` binary, same fix shape as `amd64-jserv`'s own `fs.h`).
   One sub-test, `mem()` (a malloc/free heap-exhaustion stress loop), is
   skipped rather than fixed - confirmed via gdb to hang for real, the
   same failure independently already found and skipped in `mips`'s own
   `usertests.c`. Fully wired up now - `build`/`run`/`test`/
   `quick-test`/`clean`/`kill-arm-pi2`, folded into every `-all`
   umbrella target - see `notes_arch_arm_pi2.txt`'s own "Session 2" for
   the full bug-by-bug diagnosis.
2. **`docs/claude_notes/plan_factorization.md`** - once ports build and
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
make quick-test-riscv64   # build + boot headless + assert a shell prompt
                          # (fast smoke check, no usertests)
make test-riscv64     # build + boot headless + run that port's own
                      # usertests suite, assert "ALL TESTS PASSED"
make build-all / test-all / stress-test-all / clean-all / kill-all
                      # same, across every wired-up arch - "test-all" is
                      # the quick_test-<arch> form (~1 min total);
                      # "stress-test-all" is the full test-<arch> form
                      # (~25 min on this host - real signal, but too slow
                      # for every iteration)
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

Repeat, in order, for the next arch in `plan_build_and_test.md`'s Phase 4
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
   `forks/<name>/test-xv6.py` importing `QEMU`/`main` from
   `scripts/qemu_console.py` (see `forks/amd64/test-xv6.py` for a short
   worked example) - usertests-only, no crash/log/orphan tiers, since
   only `forks/riscv64` has those test programs at all (its own
   `test-xv6.py` is the one exception not built on `qemu_console.py`,
   for that reason).
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

## Real hardware vs. QEMU: never break the board, prefer runtime detection over `#ifdef`

Several ports here (`arm-pi1`, `arm-pi1-bis`, `arm-pi2`, `arm-pi3`,
`arm64-pi4`) target real physical Raspberry Pi boards, not just QEMU -
the user owns some of this hardware and can still flash and boot a
`kernel.img` on it. That real-hardware boot path is more important than
the QEMU one (which only exists to make this port testable without the
board in hand) and must never regress:

- **Never remove or weaken code that real hardware needs** to fix a
  QEMU-only symptom. If a real value/response only comes back correctly
  on real firmware (e.g. a mailbox response in a different address
  space, or a hardware register bit real silicon sets but an emulated
  device doesn't), add a case for the OTHER form rather than replacing
  the original.
- **Prefer a runtime/dynamic check over a compile-time `#ifdef`** to
  tell real hardware and QEMU apart, so the exact same binary works on
  both - `#ifdef` is a last resort, not a default. The two patterns
  used repeatedly across these ports:
  - Read a real hardware-identity register that genuinely differs (a
    core/controller revision ID, a returned pointer's address range)
    and cache the result - see `forks/arm-pi1/csud/source/hcd/dwc/
    designware20.c`'s `HcdEmulating()` (keys off the dwc2 controller's
    own `VendorId`: QEMU reports a different revision than real
    Broadcom silicon) and `~/principia/kernel/COMPILE/9/bcm/usbdwc.c`'s
    own `emulating()` (same technique, independently used for a
    different kernel).
  - Bounds-check a value against where it's SUPPOSED to live rather
    than trusting a status code - see `forks/arm-pi1/source/console.c`'s
    `fb_ready` (the mailbox call reports "success" either way; only a
    real range check on the returned framebuffer pointer tells you
    whether it's actually usable).
- Every fix should keep working, unmodified, if the user boots the same
  `kernel.img` on real hardware next - and ideally get VERIFIED there
  eventually, not just assumed safe. See `docs/claude_notes/
  notes_arch_arm_pi1.txt` for the fullest worked example of this
  pattern applied repeatedly across one port's bring-up.

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
`notes_arch_*.txt` before assuming otherwise. `test-<arch>`'s own meaning
is unchanged by the quick/stress split below - a single arch's `test-<arch>`
still means the full run, matching what `.github/workflows/docker.yml`'s
Dockerfile calls directly and what CI depends on.

**Quick vs. stress, at the `-all` umbrella level only** (2026-09-09): a
full `stress-test-all` run (every arch's own `test-<arch>`, full
usertests) took 24m34s on this host - too slow to run on every change, so
`test-all` was repointed at each arch's own `quick-test-<arch>` instead (a
fast boot-to-shell-prompt smoke check, ~1 min total; see each fork's own
`test-xv6.py`'s `test_boot()`/`"boot"` CLI mode). `stress-test-all` keeps
the old, full-coverage `test-all` behavior under its own name. Reach for
`test-all`/`quick-test-<arch>` while iterating; run `stress-test-all` (or
a single arch's own `test-<arch>`) before trusting a change is actually
correct - the quick check only proves the kernel boots, not that syscalls
behave.

## Other docs in the tree

- `README.md` - short pitch, current build/boot status, build commands
- `docs/provenance.md` - the evidence behind each fork point
- `docs/claude_notes/plan_build_and_test.md` - the build/boot/CI plan (this file's own "Current status" section tracks progress against it)
- `docs/claude_notes/plan_factorization.md` - the later Linux-style-unification plan, blocked on the above
- `docs/claude_notes/notes_arch_<name>.txt` - real bring-up findings, one per wired-up arch
- `docs/claude_notes/notes_debugging_techniques.txt` - general debugging methodology, grown from real investigations in this repo
