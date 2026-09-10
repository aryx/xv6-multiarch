# Plan: run the ports under TinyEMU, not just QEMU

**Status:** proposed, not started. Written 2026-09-10, after a
measurement session against a locally-built TinyEMU (see "Evidence"
below - every claim in this file that says *measured* was actually run
on this host, not recalled).

**Filename note:** asked for as `plan_tinyemu.txt`; written as `.md` to
match the standing convention that forward-looking `plan_*` docs are
Markdown and only the backward-looking `notes_*` files are `.txt`.

**Does not block anything, and must not run alongside one thing:**
[plan_factorization.md](plan_factorization.md) is the active work.
`forks/riscv64`'s Phase 0 layout move under it landed while this plan
was being written (`2437049`, `13b78e1`, `221fb72`), so the paths in
this file are the post-move ones (`kernel/`, `user/`, `ulib/`, `tests/`,
`tools/`). Do not start Phase B below while any *later* factorization
step is touching `forks/riscv64` - see
[Sequencing](#8-sequencing-against-the-factorization-effort).

## 0. Why bother, when QEMU already works

Three reasons, in decreasing order of how much they actually matter to
this repo:

1. **A second implementation is a bug detector.** Every port here was
   brought up against exactly one emulator. `notes_arch_*.txt` is full
   of cases where the port was quietly relying on QEMU-specific
   behaviour and nobody could tell (`arm`'s SP804 timer that turned out
   to be a `create_unimp()` stub; `arm64-pi4`'s `TCR_TBI0`; the
   `preempt()` family). TinyEMU is a *completely* independent RISC-V
   implementation with a deliberately minimal device set, so it fails
   loudly on assumptions QEMU's `virt` machine indulges. It is the
   cheapest available approximation of "does this port work on hardware
   we did not develop against", short of the real boards.
2. **It is small enough to read.** ~15k lines total, one file per
   machine, no TCG, no QOM, no device tree plumbing you cannot follow.
   When a port misbehaves you can read the emulator's implementation of
   the register in question in about a minute - `riscv_machine.c` is
   1050 lines and contains the CLINT, the PLIC, the HTIF console and
   the whole board. Compare with tracking the same question through
   QEMU's `hw/intc/sifive_plic.c` plus `target/riscv/csr.c`. That makes
   it a genuinely better teaching target, which is the same reason this
   repo exists.
3. **It compiles to JavaScript.** TinyEMU is the engine behind JSLinux;
   `Makefile.js` + emscripten produces a browser build. "xv6 booting in
   a browser tab, from this repo" is a real and not-very-distant
   deliverable once a port runs under `temu` natively.

Non-reason: speed. TinyEMU is an interpreter and is slower than QEMU's
TCG. It will not replace `make test-all`.

## 1. What TinyEMU is, and the pinned copy

Fabrice Bellard, MIT licence, last release **2019-12-21**. A copy is
already on this machine at `~/TESTS/tinyemu-2019-12-21.tar.gz` (note:
despite the name it is an uncompressed tar - `tar xf`, not `tar xzf`).
Upstream: <https://bellard.org/tinyemu/>.

Measured: it builds here (an **aarch64** host) with two lines commented
out of its `Makefile` - `CONFIG_FS_NET` (wants libcurl + OpenSSL) and
`CONFIG_SDL` - and produces a working `temu` in a few seconds. No other
patches. Nothing to package, nothing to install; `./configure` in this
repo can point at a build directory the same way it already points at
the locally-built `qemu-system-aarch64 11.1.50` for `arm64-pi4`.

Machines it implements: `riscv32`, `riscv64`, `riscv128` (one class,
`riscv_machine.c:1043`) and `pc` (`x86_machine.c`). Configuration is a
JSON-ish file, not command-line flags:

```
{
    version: 1,
    machine: "riscv64",
    memory_size: 256,        /* MB of RAM at 0x80000000 */
    bios: "kernel.bin",      /* raw binary, loaded at 0x80000000 */
    drive0: { file: "fs.img" },
}
```

The `bios` file is copied **raw** to `0x80000000` and entered in
**machine mode** at `0x80000000` with `a0 = mhartid`, `a1 = &fdt`, by a
five-instruction trampoline TinyEMU synthesises at `0x1000`
(`riscv_machine.c` `copy_bios`). That is the same contract as
`qemu -bios none -kernel kernel`, except that it is a flat binary rather
than an ELF - so the port needs one extra `objcopy -O binary` step and
nothing else. `-ctrlc` makes Ctrl-C quit the emulator instead of going
to the guest; otherwise the escape is **Ctrl-A x** (same as QEMU).

## 2. Evidence: TinyEMU's RISC-V board vs. what xv6 assumes

Everything in this section was read out of the 2019-12-21 source and,
where marked *(measured)*, confirmed by running code under `temu`.

### 2.1 Physical memory map

| what | QEMU `virt` (what `forks/riscv64/kernel/memlayout.h` hardcodes) | TinyEMU (`riscv_machine.c:65-76`) |
|---|---|---|
| RAM base | `0x80000000` | `0x80000000` - same |
| low RAM / boot ROM | `0x1000` | `0x0` + 64KB, trampoline at `0x1000` |
| CLINT | `0x02000000` | `0x02000000` - same |
| PLIC | `0x0c000000` | **`0x40100000`** |
| console | 16550 UART at `0x10000000`, IRQ 10 | **no UART at all**: HTIF at `0x40008000`, virtio-console at `0x40010000` IRQ 1 |
| virtio disk | `0x10001000`, IRQ 1 | **`0x40011000`, IRQ 2** (the console takes slot 0) |
| framebuffer | - | `0x41000000` (`simplefb`) |

Device slots are allocated in registration order at
`VIRTIO_BASE_ADDR + n*0x1000` with `IRQ = 1 + n`
(`riscv_machine.c:884-930`): console, then net, then block, then 9p,
then input. So the disk's address and IRQ depend on whether a console
is configured. This is worth pinning explicitly in whatever the port's
`memlayout.h` equivalent ends up being, with a comment, because it is
exactly the kind of thing that silently shifts.

Accesses to unmapped physical addresses do **not** trap - TinyEMU prints
`unsupported device read access: addr=...` to the host stderr and
returns 0 (`riscv_cpu.c:426,514`). So an unmodified xv6 poking the
nonexistent UART is not a crash, it is silence plus host-side noise.

### 2.2 The interrupt controller is a stub

TinyEMU's PLIC implements exactly **two registers**: claim at
`PLIC + 0x200000 + 4` and complete at the same address
(`riscv_machine.c:253-300`). Priority, pending, enable and threshold
writes all land in the `default:` case and are discarded. There is one
context, not the M/S pair per hart that QEMU's `virt` exposes, so
`forks/riscv64/kernel/plic.c`'s `PLIC_SCLAIM(hart)` (`+0x201004`)
addresses the wrong thing; it needs `+0x200004`. Priority/enable writes
can stay as they are - they are inside the 4MB PLIC window and are
ignored harmlessly.

Consequence for the port: `plicinit`/`plicinithart` become no-ops, and
`plic_claim`/`plic_complete` change address. `plic_update_mip` raises
`MIP_MEIP | MIP_SEIP` together, level-triggered off "any pending and
unserved IRQ", which is close enough to the real thing that xv6's
`devintr()` loop works unchanged.

### 2.3 One hart. Only one

`RISCVMachine` holds a single `RISCVCPUState *cpu_state`
(`riscv_machine.c:47`), and every IRQ path pokes it directly. There is
no SMP. `CPUS=1` for every TinyEMU run - which also means TinyEMU
exercises none of the locking that `test-riscv64` currently exercises at
`CPUS=3`, and is therefore a *complement* to the QEMU test, never a
replacement.

### 2.4 The CSR set is privileged spec 1.10, and five of xv6's writes trap

This is the real work item, and it was **measured**, not inferred: a
20-instruction assembly stub was assembled, linked at `0x80000000`, run
as a TinyEMU `bios`, with `mtvec` pointing at a handler that prints `!`
and skips the faulting instruction. Output was `a!b!c!d!e!fgh`:

| instruction | CSR | result under TinyEMU |
|---|---|---|
| `csrw pmpaddr0` | `0x3b0` | **illegal instruction** |
| `csrw pmpcfg0` | `0x3a0` | **illegal instruction** |
| `csrw menvcfg` | `0x30a` | **illegal instruction** |
| `csrr time` | `0xc01` | **illegal instruction** |
| `csrw stimecmp` | `0x14d` | **illegal instruction** |
| `csrs mip, SSIP` | `0x344` | ok |
| `sfence.vma` | - | ok |

Unimplemented CSRs return `-1` from `csr_read`/`csr_write`
(`riscv_cpu.c:844-853`, `:1013-1018`), which the decoder turns into an
illegal-instruction trap. There is no lenient mode.

That maps onto current `forks/riscv64/kernel/start.c` as follows - note
that the *older* `forks/riscv32` fork predates all of this and is
therefore **much closer to TinyEMU already** (see section 4):

- **PMP** (`w_pmpaddr0`/`w_pmpcfg0`): just delete for this board.
  TinyEMU implements no PMP at all, so there is no default-deny to work
  around - the opposite of the situation `forks/riscv32`'s own
  `start.c` documents at length for qemu 8.2.2.
- **`menvcfg` ADUE** (hardware A/D bit updates): delete. TinyEMU sets
  `PTE_A`/`PTE_D` in hardware unconditionally
  (`riscv_cpu.c:275-280`), which is what the bit was asking for anyway.
- **`menvcfg` STCE + `stimecmp` + `time`** (the Sstc extension): this is
  the one that costs real code. xv6 has used supervisor-mode `stimecmp`
  since 2023; TinyEMU only has the CLINT. The port has to go back to the
  pre-Sstc scheme: `mtimecmp` at `CLINT + 0x4000`, `mtime` at
  `CLINT + 0xbff8`, an M-mode `timervec` in `kernelvec.S` with an
  `mscratch` scratch area, and `mtvec` pointing at it. That code still
  exists verbatim in `forks/riscv32/kernel/` and in any pre-2023 xv6 -
  it is a copy, not a design job.
  - **One twist:** old xv6's `timervec` raises the S-mode software
    interrupt by storing to the CLINT's `msip` register, and TinyEMU's
    CLINT implements *only* `mtimecmp` and `mtime` -
    `riscv_machine.c:193-238`, everything else falls through `default:`.
    So `msip` writes vanish and no interrupt is ever delivered. The fix
    is one instruction: set the bit directly with `csrs mip, 2`
    (`MIP_SSIP`), which TinyEMU explicitly permits from M-mode
    (`mip` write mask is `MIP_SSIP | MIP_STIP`, `riscv_cpu.c:1005-1012`,
    and confirmed by the probe above). S-mode clears it with
    `csrc sip, 2` exactly as xv6 already does, because `sip` writes are
    masked by `mideleg` (`riscv_cpu.c:931-934`) and xv6 delegates
    everything.
- `mcounteren` (`0x306`) **is** implemented, so that line can stay.

`misa` reports `RV64IMAFDQC` with both `S` and `U`
(`riscv_cpu.c:1306-1317`), so supervisor and user mode are there; it is
only the 1.11/1.12-era additions that are missing.

### 2.5 virtio is modern (v2), but not QEMU

`virtio.c` reports `VIRTIO_MMIO_VERSION = 2` (`:621`) with the split
desc/avail/used registers, which is what current xv6's `virtio_disk.c`
already drives - `-global virtio-mmio.force-legacy=false` in the
riscv64 Makefile means the port is already on the modern path. Two
mismatches only:

- `VENDOR_ID` is `0xffff` (`virtio.c:292`), not `0x554d4551` ("QEMU").
  `virtio_disk_init`'s `panic("could not find virtio disk")` fires on
  the vendor check. Relax it - checking magic + version + device id is
  the meaningful part.
- `QUEUE_NUM_MAX` is 16 (`MAX_QUEUE_NUM`, `virtio.c:96`). xv6's `NUM` is
  8, so `max < NUM` passes. Do not raise `NUM` above 16 for this board.

### 2.6 The console: HTIF out, virtio-console in

**Output is trivial and was measured working.** HTIF at `0x40008000`:
write the low 32 bits of `tohost`, then the high 32 bits (the high write
is what triggers the command). `tohost = (1<<56)|(1<<48)|ch` prints one
character; `tohost = 1` powers the machine off with `Power off.` and
`exit(0)` (`riscv_machine.c:128-150`). A 20-line assembly stub printing
`HI` and powering off ran on the first try. That is `uartputc_sync()`
and a free `poweroff()` in about a dozen instructions, available before
any allocator or lock exists - which makes early-boot `panic()` output
work from instruction one, better than the UART path.

**Input is the awkward part.** TinyEMU's HTIF keyboard polling exists but
is `#if 0`'d out (`riscv_machine.c:178-192`), so nothing ever writes
`fromhost`. Real console input has to come from the **virtio-console**
device: device id 3, one config field, queue 0 = receive (marked
`manual_recv`), queue 1 = transmit (`virtio.c:1355`), at
`0x40010000` / IRQ 1. So the port needs a small virtio-console driver -
call it 120-150 lines, the same shape as `virtio_disk.c` but simpler
(no request headers, no sector maths), and once it exists it can carry
output too and HTIF can stay as the pre-init/panic path.

There is a shortcut worth naming and then not taking: un-`#if 0` the
eleven lines of `htif_poll` in a local TinyEMU patch and poll `fromhost`
from `clockintr`. That gets an interactive shell in an afternoon, but it
means the port only runs on *our* TinyEMU, which throws away reason (1)
in section 0 - the entire point is to run on an implementation we did
not adjust to suit us. Use it as a debugging crutch if the virtio
console misbehaves; do not ship it.

## 3. Plan for `forks/riscv64` (the primary target)

Each phase ends in something runnable. Do not start the next until the
previous one prints what it claims to print.

**Phase A - pin the emulator.**
Extract `~/TESTS/tinyemu-2019-12-21.tar.gz` somewhere durable (`~/tmp/`
or a sibling of the qemu build already used for `arm64-pi4`), comment
out `CONFIG_FS_NET`/`CONFIG_SDL`, `make`. Teach `./configure` to find
`temu` the same way it finds the local `qemu-system-aarch64`: check
`PATH`, then a few known build locations, write `TEMU` into
`Makefile.config`, and make every TinyEMU target degrade to a clear
"not configured" message rather than a build failure when it is absent.
Record the tarball's provenance in `notes_arch_riscv64.txt` (or a new
`notes_tinyemu.txt`) so the next person does not have to re-derive that
`.tar.gz` is not gzipped.

**Phase B - a board axis in the fork.**
Introduce `BOARD=qemu-virt` (default) / `BOARD=tinyemu` in
`forks/riscv64/Makefile`. Per the standing rule that per-arch *files*
beat `#ifdef`, the board-specific pieces are separate files selected by
the Makefile, not conditionals inside the shared ones:

```
kernel/board/qemu-virt/{memlayout.h, uart.c,     plic.c, timer.c}
kernel/board/tinyemu/ {memlayout.h, htifcons.c,  plic.c, timer.c, virtio_console.c}
```

The exact shape should follow whatever `plan_factorization.md`'s Phase 0
settles on for `arch/` - this is a *board* under an arch, not a new
arch, and it is the first case in the repo that distinguishes the two.
That is useful information for the factorization design and is the main
reason to do this work at all right now (section 8).

**Phase C - first output.**
`objcopy -O binary` the kernel, write `xv6-tinyemu.cfg`, boot with
`start()` reduced to the minimum (no PMP, no `menvcfg`, no timer) and
`uartputc_sync` replaced by HTIF. Success = xv6's banner on stdout.
Expect to reach `main()` and then hang for want of a timer; that is the
correct intermediate state.

**Phase D - timer.**
Restore `timervec`/`mscratch` from `forks/riscv32/kernel/kernelvec.S`,
point `mtvec` at it, program `mtimecmp`, and replace the `msip` store
with `csrs mip, 2`. Replace `clockintr`'s `w_stimecmp(r_time()+...)`
with a CLINT `mtimecmp` write from the M-mode handler. Success = `ticks`
advances; a spin loop in the shell is preemptible.

**Phase E - disk.**
Move `VIRTIO0` to `0x40011000`, `VIRTIO0_IRQ` to 2, drop the vendor-id
check, fix `PLIC_SCLAIM` to `+0x200004`. Success = the kernel mounts
`fs.img` and `init` execs `sh`, with output still on HTIF.

**Phase F - input.**
Write `virtio_console.c`: receive queue with a handful of 1-byte
buffers, feed `consoleintr()` from the IRQ-1 path. Optionally move
`uartputc` to the transmit queue and leave HTIF only for
`uartputc_sync`/`panic`. Success = an interactive shell.

**Phase G - test harness.**
`forks/riscv64/test-xv6.py` is the one harness in the repo not built on
`scripts/qemu_console.py`. Rather than teach it a second emulator, add
TinyEMU support to `scripts/qemu_console.py` (a spawn-command
abstraction: argv + escape sequence + "how do I kill it") and give the
board its own thin runner. Wire up `run-riscv64-tinyemu`,
`quick-test-riscv64-tinyemu`, `test-riscv64-tinyemu`,
`kill-riscv64-tinyemu`.

**Phase H - the `-all` question.**
Do **not** fold the TinyEMU targets into `test-all`/`build-all` by
default. Precedent: `arm64-pi4`, whose boot needs a locally-built QEMU
and is therefore deliberately absent from `test-all`, the Dockerfile and
the CI matrix while its *build* is in `build-all`. TinyEMU is the same
situation but more so - it is not packaged by any distro. Either add
`test-tinyemu-all` as its own umbrella, or build TinyEMU from source
inside the Docker image (it is a 5-second build with no dependencies
once `CONFIG_FS_NET`/`CONFIG_SDL` are off, so this is genuinely
feasible - unlike the qemu 9.1 case) and only then consider CI.

**Rough size:** phases C-F are on the order of 400 lines of new code,
most of it copied from `forks/riscv32` or adapted from `virtio_disk.c`,
plus deletions in `start.c`. The unknown-unknowns budget is in Phase F.

## 4. `forks/riscv32` - the second target, and possibly the easier one

TinyEMU's `machine: "riscv32"` is the same board with `max_xlen = 32`
and Sv32, and the `forks/riscv32` port is a **pre-2023 xv6**, which
means it does not have the two hardest incompatibilities:

- it already uses the CLINT + `timervec` + `mscratch` scheme, so Phase D
  reduces to the one-instruction `msip` -> `csrs mip, 2` change;
- it never touches `menvcfg`, `stimecmp` or `time`.

What it *does* need: drop the `w_pmpaddr0`/`w_pmpcfg0` pair that was
added to it (with a long `claude:` comment explaining why qemu 8.2.2
needs it - TinyEMU needs its absence, which is a nice illustration of
why this belongs in a board file); the same PLIC/virtio/console work as
riscv64. Its `virtio_disk.c` is the legacy (version 1, `QUEUE_PFN`)
driver, and **TinyEMU is version-2 only** - so unlike riscv64, the rv32
port needs its disk driver modernised, which cancels out much of what it
saves on the timer. Do riscv64 first regardless; it is the port everyone
reads.

## 5. x86 and amd64 under TinyEMU: no, and here is the exact reason

Short answer: **not possible with the released TinyEMU, and not on this
host at all.** This is worth writing down carefully because the source
tree looks extremely encouraging right up until it doesn't.

**What looks encouraging.** `x86_machine.c` is a complete and rather
faithful PC: i440FX + PIIX3 PCI (`:2064`), 8259 PIC pair at `0x20/0xa0`,
8254 PIT, CMOS/RTC at `0x70`, PS/2 keyboard and mouse, VGA with an
option-ROM slot, a 16550 serial port at **`0x3f8` IRQ 4** (`:2070`), and
an IDE controller at **`0x1f0`/`0x3f6` IRQ 14** (`:2096`). That is,
essentially exactly, the hardware `forks/i386` and `forks/amd64` drive -
xv6's `ide.c` talks PIO to `0x1f0` and its console uses CGA + PS/2 +
`0x3f8`. A BIOS image can be supplied and is mapped both at the top of
4GB and mirrored below 1MB (`:2011-2032`), which is the classic layout
a real BIOS boots from.

**Why it does not work.** `x86_cpu.c` in the public release is a
**96-line stub**. Every entry point is:

```c
X86CPUState *x86_cpu_init(PhysMemoryMap *mem_map)
{
    fprintf(stderr, "x86 emulator is not supported\n");
    exit(1);
}
```

The actual x86 interpreter is the JS/Linux one and was never released in
C form; the `CONFIG_X86EMU=y` in the Makefile compiles the stub. So the
only working x86 path is `USE_KVM` (`x86_machine.c:46`) - i.e. TinyEMU
is not emulating x86 at all, it is a ~2000-line VMM around
`/dev/kvm`. That requires an **x86 host**. This machine is `aarch64`
(the Thelio Astra); `/dev/kvm` here is an ARM hypervisor and will not
run x86 guests. So on this machine the answer is a hard no regardless of
what we write.

**And if you did have an x86 box?** Then it is genuinely interesting but
still not free:

- The natural boot path is the Linux one: `copy_kernel()` implements the
  Linux 32-bit boot protocol at `0x100000` with a `struct linux_params`
  (`:2213-2500`), and the comment says "we don't support older
  protocols". xv6 is not a bzImage. You would either fake enough of the
  protocol header to be loaded that way (ugly, and it hands you a
  machine already in 32-bit protected mode, so xv6's `bootasm.S` never
  runs - which is precisely the part of xv6 a student is reading), or
  supply a real BIOS ROM (SeaBIOS) and let it boot the MBR off the IDE
  drive, which is the honest route and would exercise `bootblock`
  properly. The BIOS-only path is untested here and its reset behaviour
  under the KVM setup needs verifying before anyone budgets time for it.
- No SMP (single vcpu), so `forks/amd64`'s AP startup would be dead
  code.
- Because it is KVM, you get **native execution**, not emulation. That
  kills reason (2) in section 0 entirely - there is no readable
  implementation of the x86 CPU to consult, and no determinism - and
  weakens reason (1), since the CPU is the same silicon QEMU/KVM would
  give you. The devices differ, which is a real if narrow benefit.

**Verdict:** record it as understood and closed. If someone wants xv6
i386 under a small readable emulator, the target is not TinyEMU; the
honest options are Bochs (readable, slow, complete) or writing to
`forks/i386` under QEMU as today. Revisit only if Bellard ever releases
the C x86 interpreter.

## 6. So what else in `forks/` could run under TinyEMU?

| port | ISA | TinyEMU? |
|---|---|---|
| `forks/riscv64` | rv64 | **yes** - primary target, section 3 |
| `forks/riscv32` | rv32 | **yes** - `machine: "riscv32"`, section 4 |
| `forks/i386` | x86-32 | no - x86 CPU is a stub; KVM-only, x86 host only (section 5) |
| `forks/amd64`, `forks/amd64-jserv` | x86-64 | no - same, plus TinyEMU's `pc` machine is 32-bit-oriented |
| `forks/arm`, `arm-pi1`, `arm-pi1-bis`, `arm-pi2`, `arm-pi3` | ARM32 | no - TinyEMU has no ARM target |
| `forks/arm64`, `arm64-pi4` | AArch64 | no - same |
| `forks/mips` | MIPS | no |
| `forks/loongarch` | LoongArch | no |

Two out of fourteen. That is the honest scope: **TinyEMU is a RISC-V
emulator with a vestigial x86 machine**, and this repo's RISC-V surface
is two ports.

## 7. Things that fall out of this work for free (or nearly)

- **Spike.** The HTIF console from Phase C is the same protocol
  `riscv-isa-sim` uses; Spike derives the `tohost`/`fromhost` addresses
  from ELF symbols rather than fixing them at `0x40008000` (TinyEMU's
  own README calls this out as its one deliberate deviation), so a
  symbol-defined `tohost`/`fromhost` pair plus the same putchar loop
  gets xv6 running under the reference ISA simulator too. Spike *is*
  packaged, has an interactive debugger and instruction-level tracing,
  and would be a genuinely useful third opinion for `usertests`
  failures. Worth an explicit look after Phase C.
- **xv6 in a browser.** `Makefile.js` + emscripten builds TinyEMU to
  JS/wasm (the JSLinux stack). A `riscv64` port that runs under `temu`
  runs under that build with no further kernel work - the deliverable is
  a static page with `kernel.bin` and `fs.img`. This is the single
  highest-visibility thing this repo could produce and it is downstream
  of Phase F, not of anything hard.
- **rv128, for the mischief of it.** TinyEMU implements RV128 (that is
  what `CONFIG_INT128` is for), and Bellard's README notes there is no
  128-bit toolchain and no 128-bit OS, so `rv128test.bin` may be the
  only 128-bit RISC-V code in existence. Porting xv6 to RV128 would make
  it the first 128-bit operating system. There is no toolchain, which is
  the whole difficulty; note it as a curiosity, not a plan.

## 8. Sequencing against the factorization effort

`plan_factorization.md` is the active work. Its Phase 0 layout move has
landed for `forks/mips` (`f53c972`, `ff85fed`) and for `forks/riscv64`
(`2437049`, `13b78e1`, `221fb72` - done while this plan was being
written, by a concurrent session). Phase B of this plan reorganises the
same directory again, so it must not run concurrently with any further
factorization step on `forks/riscv64`.

Two ways to sequence, and the recommendation is the second:

1. **After factorization.** Safest, but late: by then the `arch/`
   layout will have been designed without a single example of two boards
   sharing one arch, and retrofitting a board axis is exactly the sort of
   thing that is cheap on paper and expensive in practice.
2. **Now that `forks/riscv64`'s Phase 0 move has landed, and before
   the shared `kernel/` merge starts.** TinyEMU support is the
   repo's only concrete instance of "same ISA, different board" that
   isn't also a different fork - `arm-pi1` vs `arm-pi2` are separate
   *trees*, whereas this is one tree that must build two ways. Building
   it early gives the factorization design a real constraint to satisfy
   instead of a hypothetical one, and it does so in the one port
   (`riscv64`) that is furthest along. **Recommended**, with the caveat
   that it must not become a second front - if it is not producing a
   boot within its budget, park it.

The blame-preservation rule applies unchanged: board files split out of
`start.c`/`uart.c`/`plic.c` must be created with `git mv`-shaped commits
where possible so `git blame -C -C` still reaches the original xv6
commits.

## 9. Definition of done

- `make run-riscv64-tinyemu` gives an interactive shell.
- `make test-riscv64-tinyemu` runs that port's own `usertests` at
  `CPUS=1` and reports `ALL TESTS PASSED`, with any skipped sub-test
  written up the way every other port's skips are.
- `make build-riscv64` (QEMU, `CPUS=3`) and `make test-riscv64` are
  **unchanged and still green** - the board axis must not cost the
  existing path anything.
- A `notes_tinyemu.txt` recording what was actually found: the CSR
  table above with whatever else the bring-up turns up, the device-slot
  ordering trap, and any place where TinyEMU disagreed with QEMU about
  something the port had been getting away with. Those disagreements are
  the deliverable; the boot is just how you find them.
- Same for `riscv32`, or an explicit note saying why it was dropped.

## 10. Open questions

- Does the virtio-console receive queue's `manual_recv` flag change what
  the driver must do, or is it purely internal to TinyEMU's polling?
- Is there any way to get a non-blocking "is there input" answer without
  the virtio console - i.e. is the shortcut in 2.6 avoidable in a way
  that stays unpatched?
- `memory_size` vs `PHYSTOP`: xv6 hardcodes `PHYSTOP = 0x88000000`
  (128MB). TinyEMU registers one RAM range at `0x80000000` of
  `memory_size` MB and puts every device *below* it, so
  `memory_size: 128` or more should be all that is needed - but confirm
  that nothing in the FDT/initrd area TinyEMU builds at low addresses is
  expected to survive, since xv6 ignores `a1` entirely.
- Should the board split live in `forks/riscv64` at all, or wait and
  land directly in the factored `arch/riscv64/board-*/`? Section 8
  argues for the former; the factorization plan's own Phase 0 outcome
  may argue otherwise.

## Appendix: the CSR probe

The measurements in section 2.4 came from this. Assemble with
`riscv64-linux-gnu-as`, link at `0x80000000`, `objcopy -O binary`, run
as a TinyEMU `bios`. Output was `a!b!c!d!e!fgh` followed by
`Power off.` - a `!` means that instruction trapped. It is worth keeping
because it is the shape of every future "does this emulator have X"
question: a putchar macro, `mtvec` pointing at a handler that prints and
skips, and one instruction per letter.

```asm
    .macro PUTC ch
    li   t0, 0x40008000        # HTIF
    li   t1, \ch
    sw   t1, 0(t0)             # tohost low
    li   t2, 0x01010000        # device 1, cmd 1 (putchar)
    sw   t2, 4(t0)             # tohost high - triggers the command
    .endm
    .section .text
    .globl _start
_start:
    la   t3, trap
    csrw mtvec, t3
    PUTC 'a'
    csrw pmpaddr0, zero      # 0x3b0
    PUTC 'b'
    csrw pmpcfg0, zero       # 0x3a0
    PUTC 'c'
    csrwi 0x30a, 0           # menvcfg
    PUTC 'd'
    csrr t4, time            # 0xc01
    PUTC 'e'
    csrwi 0x14d, 0           # stimecmp
    PUTC 'f'
    li   t5, 2
    csrs mip, t5             # SSIP - the timervec replacement
    PUTC 'g'
    sfence.vma
    PUTC 'h'
    li   t0, 0x40008000      # tohost = 1 -> power off
    li   t1, 1
    sw   t1, 0(t0)
    sw   zero, 4(t0)
1:  j 1b
trap:
    PUTC '!'
    csrr t6, mepc
    addi t6, t6, 4
    csrw mepc, t6
    mret
```
