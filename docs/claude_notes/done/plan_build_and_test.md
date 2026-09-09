# Plan: building and testing all thirteen architectures

**Status: DONE** (2026-09-09), which is why this file now lives under
`docs/claude_notes/done/`. Everything below is the plan **as written**
before any of it was attempted, kept verbatim as the record of what was
predicted; the outcome, and where reality departed from the prediction,
is summarised in the next section. The residue that is *not* done — a
handful of skipped `usertests` sub-tests, one port's boot kept out of
CI, and real-hardware verification — moved to
[plan_build_and_test_2.md](../plan_build_and_test_2.md).

This was a prerequisite for
[plan_factorization.md](../plan_factorization.md) — merging files you
cannot build is how the gitlab predecessor of this project died. That
gate is now open.

## Outcome (written 2026-09-09, after the fact)

All **14** `forks/<name>/` ports build, boot to a real shell under QEMU,
and pass their own `usertests` suite. `make test-all` is green on this
host. Per-port findings are in each `docs/claude_notes/notes_arch_*.txt`;
the honest status matrix the "Definition of done" below asks for is in
the root `README.md`.

Four things went differently from the plan below, all deliberate:

- **The `tools/` harness was never built.** The shape the "Test harness"
  section sketches (`tools/arches/<name>.conf`, `build.sh`, `boot.sh`,
  `smoke.py`, `matrix.sh`) was replaced by `./configure` +
  `Makefile.config` + a top-level `Makefile` with per-arch
  `build-`/`run-`/`test-`/`quick-test-`/`clean-`/`kill-` targets, plus a
  per-fork `test-xv6.py` built on `scripts/qemu_console.py`. Same job,
  but it reuses each port's own build system instead of describing it
  twice in a manifest, and `make` gives the umbrella targets for free.
  Consequently there is no `tools/matrix.sh`: the matrix is maintained
  in `README.md` and checked by `make test-all` / CI.
- **`test-all` was split** into `test-all` (each arch's
  `quick-test-<arch>`, boot-to-prompt, ~1 min total) and
  `stress-test-all` (each arch's full `test-<arch>`, ~25 min). A single
  arch's `test-<arch>` still means the full run.
- **`d1` was dropped, not brought up** — see the note by its own row in
  the per-architecture table below.
- **Two ports were added after the fact** that the Phase 4 list never
  named: `arm-pi3` (`forks/pi_mp`) and `arm64-pi4` (`forks/rpi4`), both
  taken to the same bar as their siblings.

The plan's own expectation — "this will not be thirteen green ticks" —
turned out to be too pessimistic. Every port reached `boots`, none
stayed at `builds` or `broken`. It cost dozens of real, individually
diagnosed bugs across the tree to get there — the per-port bug counts are
in each `notes_arch_*.txt`'s own header, and `arm-pi3` alone accounts for
fifteen.

## Which machine?

**Use the arm64 Linux machine.** It is the better host, and not merely by
convenience — it solves a problem macOS cannot.

Worth knowing first: emulation is *not* the gap on either host. The Mac
already has QEMU with every target these ports need, running natively on
arm64:

```
qemu-system-i386  qemu-system-x86_64  qemu-system-arm  qemu-system-aarch64
qemu-system-riscv32  qemu-system-riscv64  qemu-system-mips  qemu-system-mipsel
qemu-system-loongarch64
```

What is missing on macOS is (a) cross-compilers and (b) a GNU userland:

- `make` is **GNU Make 3.81** (2006 — Apple won't ship GPLv3).
- `sed` and `awk` are BSD variants; xv6's build and `runoff` scripts assume
  GNU behaviour.
- `mkfs`, the *host* tool that builds `fs.img`, compiles with the host
  compiler. Old xv6 `mkfs.c` collides with macOS headers — the xv6-d1 author
  hit exactly this and committed "changed types - remove _t to avoid clash
  with macos includes for mkfs".
- Every port documents a Linux build in its own README. Following upstream
  instructions verbatim beats porting thirteen build systems to macOS.

All of that is simply absent on Linux: `apt install` the toolchains and the
GNU userland is already correct. Containers stop being a workaround and become
optional — still worth adding later to *pin* the toolchain for CI, but not
needed to start.

### The one thing the Linux machine genuinely fixes

Both hosts are arm64, so moving OS does **not** by itself solve the LoongArch
toolchain problem — Loongson's CLFS cross-tools ship as **x86_64-Linux
binaries**, and an arm64 host cannot run them natively either way.

But Linux can make that invisible:

```sh
sudo apt install qemu-user-static binfmt-support
```

With `binfmt_misc` registered, x86_64 ELF binaries execute transparently, so
the Loongson toolchain simply runs — no wrapper, no separate container, no
`--platform` juggling. The compile is emulated and therefore slow, which is
fine. On macOS the equivalent is an emulated amd64 Docker container, which is
clumsier and slower. This alone justifies the move.

Check for a native package first — `apt-cache search loongarch` — since recent
Debian/Ubuntu may carry a `loongarch64-linux-gnu` cross-gcc built for arm64,
which would avoid emulation entirely.

### Toolchain packages

| target | Debian/Ubuntu package | native on arm64? |
|---|---|---|
| riscv64 / riscv32 bare metal | `gcc-riscv64-unknown-elf` | yes |
| ARM 32-bit bare metal | `gcc-arm-none-eabi` | yes |
| aarch64 | `gcc-aarch64-linux-gnu` | yes |
| MIPS | `gcc-mipsel-linux-gnu`, `gcc-mips-linux-gnu` | yes |
| i386 / x86-64 | `gcc-i686-linux-gnu`, `gcc-x86-64-linux-gnu` | yes |
| LoongArch | check `apt-cache search loongarch`; else Loongson CLFS via `binfmt_misc` | probably not |

The ports want *bare-metal* ELF toolchains; where only a `-linux-gnu` cross
exists, `-ffreestanding -nostdlib -static` generally substitutes. xv6's
Makefiles auto-detect via `TOOLPREFIX` and will need that list extended.

### Two practical notes

- It is a **work machine**. Installing a dozen cross-toolchains and registering
  `binfmt_misc` handlers is intrusive; if that is awkward, do it in a container
  *there* — on Linux, containers are near-native, so you keep the speed and the
  binfmt trick still works from the host.
- Getting the repo across: push to GitHub first, then clone. Avoid copying the
  working tree by hand — the point of this repo is its history, and a `scp` of
  the checkout would lose it.

## Set expectations: this will not be thirteen green ticks

These are 2013-2023 kernels being fed a 2020s compiler. Expect breakage from
`-fno-pie` / PIE-by-default, stack protector defaults, stricter inline asm,
`-Werror` on newly-warned constructs, and changed section/linker behaviour.
Modern xv6-public famously needs flag surgery on current GCC.

The deliverable is therefore **an honest matrix**, not universal success:

| status | meaning |
|---|---|
| `boots` | builds and reaches the shell under QEMU, `ls` works |
| `builds` | compiles and links, but no usable emulation target |
| `builds-with-patch` | needs toolchain flags or small fixes, recorded |
| `broken` | does not build; failure captured verbatim |
| `hardware-only` | targets a real board with no QEMU machine |

Record the result per arch and keep it in the repo. A truthful matrix is
worth more than a forced pass.

## Per-architecture targets

| arch | toolchain | QEMU machine | expectation |
|---|---|---|---|
| `x86` | i686 ELF | `qemu-system-i386` | boots — the best-trodden path |
| `amd64` | x86_64 ELF | `qemu-system-x86_64 -kernel` | boots; MIT switched it to grub/`-kernel` |
| `x86_64` | x86_64 ELF | `qemu-system-x86_64` | boots; maintained to 2023, likely the healthiest |
| `riscv` | riscv64 ELF | `qemu-system-riscv64 -M virt` | boots; **ships its own `test-xv6.py`** |
| `rv32` | riscv32 ELF | `qemu-system-riscv32 -M virt` | boots; author documents qemu 5.0 |
| `aarch64` | aarch64 | `qemu-system-aarch64 -M virt` | boots |
| `mips` | mipsel | `qemu-system-mipsel` | unknown; 2016 code |
| `loongarch` | loongarch64 | `qemu-system-loongarch64` | author ships a `qemu-loongarch-runenv` |
| `rpi1` | arm-none-eabi | `-M raspi0` / `raspi1ap` | uncertain; targets BCM2835 |
| `rpi2` | arm-none-eabi | `-M raspi2b` | uncertain |
| `armv6-rpi` | arm-none-eabi | `-M raspi0` | uncertain |
| `armv7-rpi` | arm-none-eabi | Banana Pi A20 — try `cubieboard` | likely build-only |
| `d1` | riscv64 ELF | none (Allwinner D1/Nezha) | **build-only**; boots from 0x40000000 via `xfel`, uses a ramdisk |

> claude: `forks/d1` was removed (commit c233d3b) rather than brought up -
> no QEMU machine model exists for real Allwinner D1 hardware, and it's
> RISC-V64 forked from the same lineage `forks/riscv64` already covers.
> Its own history was checked for anything worth porting to
> `forks/riscv64` first; nothing was found - every non-hardware-specific
> difference was an upstream MIT xv6-riscv commit `forks/riscv64` already
> has (Sstc timers, the `user.ld`/`eh_frame` fix, `MENVCFG_ADUE`), and
> the rest (clock/GPIO/UART bring-up, a ramdisk instead of virtio) is
> genuinely Allwinner D1-specific with no QEMU application.

## Test harness

Keep it small and uniform. Per arch, a manifest declaring: toolchain prefix,
build command, QEMU binary and machine, boot arguments, expected banner, and a
smoke script.

```
tools/
  arches/<name>.conf     # TOOLPREFIX, QEMU, MACHINE, EXTRA, BANNER
  build.sh <arch>        # builds one arch with its toolchain
  boot.sh <arch>         # boots under QEMU, -nographic, with a timeout
  smoke.py <arch>        # expect-style: wait for shell, run `ls`, assert, exit
  matrix.sh              # runs everything, emits the status table
```

`smoke.py` is the only interesting piece: spawn QEMU with a pty, wait for
`init: starting sh` or a `$` prompt, send `ls\n`, assert the output contains
`README` and `cat`, then send the port's exit escape or kill after a timeout.
**Model it on `arch/riscv/test-xv6.py`**, which MIT actively maintains — the
most recent upstream commits in this whole repo are hardening exactly that
script (line-buffering, non-blocking reads, timeouts). Reuse its approach
rather than inventing one.

## Phases

**Phase 0 — scaffolding.** Create `tools/` and the per-arch manifests. No
builds yet. On the Linux box, install the toolchains from the table above and
record the exact package versions used, so the set is reproducible later.

**Phase 1 — one arch end to end: `riscv`.** Highest chance of success, and it
brings its own test script. Getting `build.sh riscv && smoke.py riscv` green
proves the whole harness shape before it is replicated.

**Phase 2 — one arch from the other family: `x86`.** The x86 and riscv
families differ in layout and build system (see the factorization plan), so
proving both shapes early prevents designing the harness around one of them.

**Phase 3 — pin the toolchain.** Capture the working toolchain set from phases
1-2 as a Dockerfile, so CI and any second machine reproduce it exactly. This is
about reproducibility, not capability — the native Linux install already works
by this point. Set up `binfmt_misc` here if LoongArch needs the emulated
x86_64 Loongson toolchain.

**Phase 4 — the remaining ten**, cheapest first: `x86_64`, `amd64`, `rv32`,
`aarch64`, `loongarch`, `mips`, then the four ARM board ports, then `d1` as
build-only (`d1` was later dropped entirely rather than brought up - see
the note by its own row in the matrix above). Record every outcome in
the matrix, including failures, verbatim.

**Phase 5 — CI.** A GitHub Actions workflow running `matrix.sh` and publishing
the table. Once this exists, factorization can proceed with a safety net.

## Definition of done

- `tools/matrix.sh` runs unattended and produces the status table.
- Every arch has a recorded status with evidence — a build log or a boot
  transcript — committed alongside.
- At least one architecture per family (`x86`-family and `riscv`-family) is at
  `boots`, so factorization work in either family is verifiable.
- The matrix is in the README, so anyone can see what actually works before
  trusting the tree.

## Deliberately out of scope

Fixing the ports. If `mips` does not build under a modern toolchain, that is a
recorded fact, not a task for this phase. Repairs are separate work with their
own commits, and they must not be smuggled into the harness.
