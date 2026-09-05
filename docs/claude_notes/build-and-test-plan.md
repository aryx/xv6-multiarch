# Plan: building and testing all thirteen architectures

**Status:** proposed, not started. This is a prerequisite for
[factorization-plan.md](factorization-plan.md) — merging files you cannot
build is how the gitlab predecessor of this project died.

## Do we need to move to a Linux machine?

**No — but we do need Linux containers, and Docker is already installed.**

The surprising finding is that emulation is not the gap. Your Mac already has
QEMU with every target these ports need:

```
qemu-system-i386  qemu-system-x86_64  qemu-system-arm  qemu-system-aarch64
qemu-system-riscv32  qemu-system-riscv64  qemu-system-mips  qemu-system-mipsel
qemu-system-loongarch64
```

These run natively on arm64 macOS and are fine to drive from the host. What is
missing is (a) cross-compilers, and (b) a GNU userland. On macOS specifically:

- `make` is **GNU Make 3.81** (2006 — Apple won't ship GPLv3). Several of
  these Makefiles use newer syntax.
- `sed` and `awk` are BSD variants; xv6's build and `runoff` scripts assume GNU
  behaviour.
- `mkfs`, the *host* tool that builds `fs.img`, is compiled with the host
  compiler. Old xv6 `mkfs.c` collides with macOS headers — the xv6-d1 author
  already hit this and committed "changed types - remove _t to avoid clash with
  macos includes for mkfs".
- Every one of these ports documents a Linux build in its own README.
  Following upstream instructions verbatim is much cheaper than porting the
  build to macOS thirteen times.

So the recommendation is: **build inside a Debian container, run QEMU either
inside the container or on the host.** That keeps your machine as-is, gives a
reproducible toolchain set, and turns directly into CI later. Moving to a
dedicated Linux box would also work but buys nothing a container doesn't.

### The Apple Silicon wrinkle

You are on arm64. Debian arm64 provides most cross-toolchains as native arm64
binaries, so they run at full speed:

| target | Debian package | native on arm64? |
|---|---|---|
| riscv64 / riscv32 bare metal | `gcc-riscv64-unknown-elf` | yes |
| ARM 32-bit bare metal | `gcc-arm-none-eabi` | yes |
| aarch64 | `gcc-aarch64-linux-gnu` | yes |
| MIPS | `gcc-mipsel-linux-gnu`, `gcc-mips-linux-gnu` | yes |
| i386 / x86-64 | `gcc-i686-linux-gnu`, `gcc-x86-64-linux-gnu` | yes |
| **LoongArch** | Loongson CLFS tarball is **x86_64-Linux binaries** | **no** |

LoongArch is the one exception. Either find an arm64 loongarch64 toolchain (a
recent Debian `gcc-*-loongarch64-linux-gnu` may exist — check first), or run
just that build in a `--platform linux/amd64` container under emulation. It's
a compile, so slow is acceptable.

Note the ports want *bare-metal* ELF toolchains; where only a `-linux-gnu`
cross exists, `-ffreestanding -nostdlib -static` generally substitutes. xv6's
Makefiles usually auto-detect via `TOOLPREFIX`, and will need that list
extended.

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

## Test harness

Keep it small and uniform. Per arch, a manifest declaring: toolchain prefix,
build command, QEMU binary and machine, boot arguments, expected banner, and a
smoke script.

```
tools/
  arches/<name>.conf     # TOOLPREFIX, QEMU, MACHINE, EXTRA, BANNER
  build.sh <arch>        # builds in the container
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
builds yet. Decide container base image and pin it.

**Phase 1 — one arch end to end: `riscv`.** Highest chance of success, and it
brings its own test script. Getting `build.sh riscv && smoke.py riscv` green
proves the whole harness shape before it is replicated.

**Phase 2 — one arch from the other family: `x86`.** The x86 and riscv
families differ in layout and build system (see the factorization plan), so
proving both shapes early prevents designing the harness around one of them.

**Phase 3 — container image.** Fold the toolchains discovered in phases 1-2
into a pinned Dockerfile. Add the amd64-emulated LoongArch path only if the
arm64 toolchain search fails.

**Phase 4 — the remaining ten**, cheapest first: `x86_64`, `amd64`, `rv32`,
`aarch64`, `loongarch`, `mips`, then the four ARM board ports, then `d1` as
build-only. Record every outcome in the matrix, including failures, verbatim.

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
