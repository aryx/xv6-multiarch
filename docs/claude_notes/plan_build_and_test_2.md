# Plan: the build-and-test residue

**Status:** proposed, not started. Written 2026-09-09, the day
[done/plan_build_and_test.md](done/plan_build_and_test.md) reached its own
definition of done and moved under `done/`.

That plan is finished in the sense that mattered: all 14 `forks/<name>/`
ports build, boot to a real shell under QEMU, and pass their own
`usertests`, and `make test-all` is green (`EXIT=0`, thirteen ports
boot-checked in ~1 minute on this host — `arm64-pi4` builds but boots
separately, see item 2). This file collects what it left behind, so the
leftovers are tracked somewhere other than a paragraph in `CLAUDE.md`.

**Nothing here blocks [plan_factorization.md](plan_factorization.md).**
Every item below is a *deepening* of coverage on ports that already build
and boot; the factorization gate ("don't merge what you can't build") is
open. If the choice is between this file and starting factorization,
start factorization — these are the kind of items that are cheapest to
revisit when the trees are shared anyway (item 1 especially: three ports
skipping overlapping sub-tests is an argument for one shared `usertests`,
not three).

## 1. The skipped `usertests` sub-tests

Three ports reach "ALL TESTS PASSED" with sub-tests commented out. Each
skip is recorded in the port's own `usr/usertests.c` with a `claude:`
comment and diagnosed in its `notes_arch_*.txt`, so none of this is
hidden — but it is the one place where the matrix in `README.md` is
"passes, with an asterisk".

| port | skipped | where it's written up |
|---|---|---|
| `forks/mips` | ~~`preempt`~~, ~~`mem`~~ (both fixed 2026-09-09), `sbrktest`, `validatetest`, `exitwait`, `forktest` | `notes_arch_mips.txt` Bug 7 / Bug 8 |
| `forks/arm` | ~~`preempt`~~ (fixed 2026-09-09), `sbrktest` | `notes_arch_arm.txt`, "Gap 3" + Bug 14/15 |
| `forks/arm-pi2` | ~~`mem`~~ (fixed 2026-09-09) — **none left** | `notes_arch_arm_pi2.txt` Bug 12 |

**`preempt()` and `mem()` are both gone from every port.** None of the
three was what it had been written off as. `forks/arm` had no timer
interrupt at all; `forks/mips` had the interrupt but an always-false
yield condition; `mem()` was not a hang and not a kernel bug at all, but
quadratic user-space allocator fragmentation. Five skips remain, all in
`mips` except `arm`'s `sbrktest`, and `forks/arm-pi2` is now clean.

The skips are **not** independent — they cluster into two families, which
is the useful part and the reason to attack them as one task rather than
nine:

- **`preempt()` — no timer interrupt reaching the scheduler.** Skipped in
  `arm` and `mips`. It was *root-caused* exactly once, in
  `forks/arm64-pi4`: the timer PPI was enabled on CPU0 only, because
  GICv2 banks `ISENABLER0`/`IPRIORITYR` per CPU interface for INTIDs
  0-31, so cores 1-3 ran with no timer and a spinning user process was
  never preempted (measured 108/0/0/0 IRQs per CPU before the fix,
  382/351/351/350 after). That fix is the template: **before debugging a
  `preempt()` hang, count interrupts per CPU** (`-d int`, or a per-CPU
  counter) and confirm the tick actually fires, rather than reading
  scheduler code. (`CLAUDE.md` and `notes_arch_arm64_pi4.txt` both say
  three sibling ports skipped this test and name `arm-pi2` among them;
  that is wrong, and corrected in both — `arm-pi1`,
  `arm-pi1-bis`, `arm-pi2` and `arm-pi3` all run `preempt()`
  uncommented. Only `arm` and `mips` skip it.)
  - **`forks/arm`: DONE 2026-09-09.** Exactly as sketched above. `-d int`
    measured **one** IRQ across a full boot plus 150s of usertests, i.e.
    no preemption at all, because the SP804 "ARM timer" this port
    programs is `create_unimp()`'d in QEMU's `bcm2835_peripherals.c` and
    never interrupts. Replaced with the BCM2835 System Timer channel 3
    (`SYSTIMER`, 1MHz, compare-3, GPU IRQ 3) — the same tick source
    `forks/arm-pi2` already uses on this same board
    (`source/timer.c`'s `timer3init()`/`timer3intr()`), which is exactly
    why `arm-pi2` ran `preempt()` successfully and `arm` did not. Real
    hardware, not a QEMU workaround; the SP804 code stays in the tree,
    correct but no longer registered, since ticking both would
    double-count on a real Pi. Enabling it uncovered a second, latent
    bug in `device/gic.c`'s dispatcher — it tested the whole BASIC
    pending word as "ARM timer fired", but bits 8/9 of that register
    mean "GPU pending 1/2 is non-empty", so every System Timer IRQ was
    also dispatched to `isrs[PIC_TIMER0]` ("unhandled interrupt: 0");
    fixed to test bit 0. `preempt()` is uncommented and
    `make test-arm` is green: `preempt: kill... wait... preempt ok`,
    `ALL TESTS PASSED`, `EXIT=0`. Written up as Bug 14/Bug 15 in
    `notes_arch_arm.txt`. **`sbrktest` in `arm` is unchanged** — it was
    explicitly retested on a clean rebuild after this fix and still
    hangs, so it is a genuinely separate problem, not a leftover of this
    one.
  - **`forks/mips`: DONE 2026-09-09.** Different cause, same symptom, and
    a warning about the measurement: `-d int` on QEMU's MIPS target does
    **not** log external interrupt delivery, so it reports a convincing
    "zero interrupts" that is simply false. Counting at the interrupt
    controller instead (`-trace memory_region_ops_write`, then the i8259
    region) showed **346 specific-EOIs for IRQ 0 in 15s of idle** — the
    tick was working the whole time. The bug was one always-false
    condition: `trap.c` ended with
    `if(... && tf->cause == T_IRQ0+IRQ_TIMER) yield();`, inherited
    verbatim from x86 xv6 where `tf->trapno` really does hold that. On
    MIPS `tf->cause` is the CP0 Cause register — a bitfield with the
    exception code at bits 6:2 — so it never equals 32 and `yield()` was
    never called. Fixed with an `istimer` flag set inside the `EXC_INT`
    switch where `picgetirq()` already knows the IRQ, the same shape
    `forks/arm`'s `pic_dispatch()` uses. `preempt()` uncommented,
    `make test-mips` green (`EXIT=0`, `preempt: kill... wait... preempt
    ok`). Written up as Bug 7 in `notes_arch_mips.txt` and as items
    40/42/43 in `notes_debugging_techniques.txt`.
    - `exitwait()` and `forktest()` were each **re-enabled and retested**
      after this fix and each still hangs on its own, so they are
      genuinely separate bugs, not symptoms of the missing preemption.
      `mem()`, `validatetest()` and `sbrktest()` unchanged. Five of the
      original six `mips` skips remain.
- **`mem()`: DONE 2026-09-09.** The hint above was right that it was one
  shared bug in the inherited `umalloc.c`, and wrong that it "hangs for
  real": it terminates, quadratically. `morecore()` asks `sbrk` for
  `max(nunits, 4096)` units, and `malloc(10001)` (1252 units) does not
  divide 4096 — three blocks fit and **340 units are stranded per
  chunk**, too small to reuse and impossible to coalesce because the next
  `sbrk` chunk is separated from the remainder by the blocks just handed
  out. The free list gained one dead fragment per chunk and `malloc()`
  rescanned it before every `morecore()`. The variable between ports is
  RAM, not the architecture: `forks/arm` passes unmodified at 128MB,
  `arm-pi2` sizes memory from the firmware mailbox and gets ~960MB.
  gdb settled it — the process was in USER space inside `malloc`'s
  free-list walk (`r4=1252`, `r2=340`), never in the kernel, so the
  "allocuvm/kalloc under memory pressure" theory was wrong from the
  start. Fixed by rounding the chunk up to a whole multiple of the
  request; a no-op for any size dividing 4096 (every power of two) and
  for requests above 4096. `mem()` went from not finishing in 600s to
  ~4s; `make test-arm-pi2` is 30s wall clock end to end. Applied to both
  `arm-pi2` and `mips` and verified on each. Written up as Bug 12
  (`notes_arch_arm_pi2.txt`) and Bug 8 (`notes_arch_mips.txt`).
  - *Latent elsewhere:* every other fork carries the same unmodified
    `morecore()` and passes `mem()` only by having less RAM. Fixed only
    where a real failure was observed; worth doing once for all ports
    when factorization unifies `umalloc.c`.

`sbrktest`, `validatetest`, `exitwait` and `forktest` (all `mips`, plus
`sbrktest` in `arm`) are root-caused only as far as *where* they hang,
never *why*; `notes_arch_mips.txt` has the gdb backtraces.

**Definition of done:** every `usertests.c` in the tree has zero
`claude:`-commented-out test calls, and `stress-test-all` still passes.
Partial credit is fine and should be committed per port.

## 2. `arm64-pi4` boots nowhere but this machine

`forks/arm64-pi4` is fully working — interactive shell on 4 cores, full
unmodified `usertests`, nothing skipped — but only against a **locally
source-built QEMU** (11.1.50 here). QEMU only grew `-M raspi4b` in 9.1;
this Ubuntu 24.04 packages 8.2.2. So today:

- `build-arm64-pi4` is in `build-all`, `clean-arm64-pi4` in `clean-all` —
  compiling needs only the aarch64 cross-compiler every host here already
  has.
- its **boot** targets are deliberately absent from
  `test-all`/`stress-test-all`, from the Dockerfile, and from the CI
  matrix (13 arches, not 14).

Options, cheapest first:

1. **Wait for the distro.** A newer base image (Ubuntu 25.04+ / Debian
   trixie) ships QEMU ≥ 9.1; bumping the Dockerfile's base may be a
   one-line fix, but it re-qualifies *every other* port against a new
   QEMU at the same time — run `stress-test-all` in the new image before
   trusting it.
2. **Build QEMU in the image.** Correct and self-contained, but adds a
   long compile to every CI run; if taken, build only
   `qemu-system-aarch64` and cache the layer.
3. **Leave it.** Documented, honest, and the status quo. This is fine as
   long as `README.md` keeps saying so.

Note the asymmetry worth preserving either way: **build** coverage and
**boot** coverage are separate axes here, and the split is deliberate —
don't "fix" it by dropping `build-arm64-pi4` from `build-all`.

## 3. None of it has been verified on real hardware

Five ports target physical Raspberry Pi boards the user actually owns
(`arm-pi1`, `arm-pi1-bis`, `arm-pi2`, `arm-pi3`, `arm64-pi4`). Every fix
in those trees was written to keep the real-hardware path working — see
`CLAUDE.md`'s "never break the board, prefer runtime detection over
`#ifdef`" rule, and `notes_arch_arm_pi1.txt` for the fullest worked
example — and each was reasoned about and re-checked against the
`kernel.img` build, but **none has been booted on the board**. That is an
assumption, not a result, and it is the single largest untested claim in
the repo.

The `#ifdef`-to-runtime-detection work in `arm64-pi4` (a `CurrentEL`
check replacing `-DRPI4_QEMU`, so one image is correct on both firmware
paths) means there is now exactly one binary per port to flash, which
makes this cheap to actually do: flash, boot, capture the serial log,
append it to that port's `notes_arch_*.txt`.

Adjacent, already-written plans for the same class of work on other
boards: [plan_lattepanda.md](plan_lattepanda.md) (x86-64, `amd64-jserv`)
and [plan_orange_pi.md](plan_orange_pi.md) (riscv64).

## 4. Smaller loose ends

- **The matrix in `README.md` is maintained by hand.** The original plan
  wanted a `tools/matrix.sh` to emit it; the repo went with
  `./configure` + `make` instead and never replaced that piece. A script
  that runs `test-all` and regenerates the table would keep the README
  from drifting — worth doing only if the table starts being wrong.
- **Graphics coverage is partial.** `run-<arch>-qemu-graphics` exists for
  six ports (`i386`, `amd64`, `amd64-jserv`, `arm-pi1`, `arm-pi1-bis`,
  `arm-pi3`) and `make test-all-graphics` regression-tests them (needs
  `$DISPLAY`, so it is not in CI). The other ports have no framebuffer
  path at all, which for most of them is a property of the port, not a
  gap — check before "fixing" one.
- **Boot flakiness has no watchdog.** `arm-pi3` booted about 1 time in 6
  before its SMP handshake was fixed, and was only trusted again after
  20/20 consecutive boots. Nothing in the tree enforces that: `test-all`
  boots each port exactly once, so a port that regresses to 5-in-6 would
  look green most runs. A `make flake-test-<arch> N=20` target would make
  that measurable, and is the cheapest item in this file.
- **`forks/d1` stays dropped.** Recorded here so it is not re-litigated:
  removed in commit `c233d3b`, no QEMU model for Allwinner D1, and its
  history was checked for anything worth porting to `forks/riscv64`
  first — nothing was found. See the note by its row in
  `done/plan_build_and_test.md`'s own per-architecture table.
