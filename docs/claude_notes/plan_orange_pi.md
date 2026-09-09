# Plan: Orange Pi RV2 (riscv64) bring-up

**Status:** proposed, not started. Written 2026-09-09, from a research-only
conversation — no code touched yet. Write a `notes_arch_riscv_opi_rv2.txt`
(matching every other port's own findings file) once real bring-up work
starts; this file stays a plan.

## Goal

Get `forks/riscv64` (or `forks/riscv32`) booting on the user's own physical
Orange Pi RV2 board, the way `arm-pi1`/`arm-pi2`/`arm-pi3` reached real
Raspberry Pi hardware. This is harder than a normal Phase 4 "add a new arch"
pass (see `CLAUDE.md`'s "Adding a new arch" recipe) — **nobody has ported
xv6 to this board or SoC before** (checked: no hits anywhere for xv6 +
SpacemiT K1 / Ky X1 / Orange Pi RV2), and unlike the ARM Pi ports there's no
earlier port in this repo to crib boot-up patterns from. This is closer to a
from-scratch bring-up.

## The board

Orange Pi RV2: a ~$35–60 credit-card-sized SBC (Raspberry-Pi-shaped: GPIO
header, HDMI, USB, Ethernet), built around the "Ky X1" SoC — a rebrand/
variant of SpacemiT's "K1" chip (same chip family also sold as the Banana Pi
BPI-F3 and Milk-V Jupiter, under the SpacemiT K1/M1 name). 8 RISC-V cores, up
to 1.6 GHz, 2–8 GB RAM.

Don't confuse it with the **Orange Pi R2S**: same Ky X1/K1 chip, but a
different, smaller router-shaped board (no GPIO header, no display output, 4
Ethernet ports instead of 2). Some of the Linux device-tree patches found
below are for the R2S specifically, not the RV2 — useful as a reference
(same SoC-level bring-up: clocks, UART, PLIC) but the two boards' device
trees will differ in peripheral layout.

## Concept glossary: RISC-V/OrangePi world → PC/RaspberryPi world

You know PCs and the Raspberry Pi already, not RISC-V, so here's a rough
translation table. None of these mappings are exact — RISC-V's boot model is
genuinely a third design, not just relabeled x86 or ARM — but they should
give a mental foothold.

| Concept | What it is | Rough PC/RPi analogy |
|---|---|---|
| **M-mode / S-mode / U-mode** | RISC-V's three CPU privilege levels: Machine (most privileged), Supervisor (normal OS kernel), User. | x86 ring 0 vs ring 3 — but x86 has no built-in third level above ring 0 the way RISC-V has M-mode above S-mode (x86 hypervisors bolt on VMX root mode instead). ARM's EL3 (Secure Monitor) / EL1 (kernel) / EL0 (user) is the closer match: EL3 ≈ M-mode. |
| **OpenSBI** | RISC-V's Supervisor Binary Interface firmware: runs in M-mode, stays resident for the machine's entire life, answers `ecall`-based "SBI calls" from the OS (arm a timer, start another core, reboot). | Not UEFI/BIOS, which mostly exits after boot — closer to a permanent runtime service layer. The Raspberry Pi ports in this repo have nothing structurally equivalent: they run the kernel directly, with no separate resident privileged firmware to call back into. |
| **FSBL** (First-Stage BootLoader) | The very first code the chip's BootROM loads, before OpenSBI exists in memory. Vendor-specific; job is narrow — bring up just enough of the chip (mainly DRAM) to load the next stage. | A PC's earliest UEFI "PEI phase", or the RPi's `bootcode.bin`. |
| **U-Boot** | A generic, scriptable bootloader: reads a kernel image + device tree off SD/eMMC/USB/network, lets you pick boot options at a serial console, jumps to the kernel. | GRUB, roughly — much more capable than the Raspberry Pi's own `config.txt`-driven boot flow used elsewhere in this repo. |
| **Device tree** | A `.dtb` data structure describing hardware (memory ranges, peripheral MMIO addresses, interrupt lines), handed to the kernel at boot. | What ACPI + PCI enumeration give a PC kernel. The ARM Pi ports here don't use one at all — they hard-code addresses via linker `--defsym` flags (predates device tree being standard for them). |
| **PLIC** (Platform-Level Interrupt Controller) | RISC-V's standard external-interrupt router. `forks/riscv64` already knows how to drive one (architectural standard, not QEMU-specific) — only the base address and IRQ numbers change per board. | A PC's APIC, or the Pi's BCM interrupt controller. |

`forks/riscv64`'s current QEMU setup never uses the M/S-mode distinction at
all: it boots with `-bios none`, so xv6 itself starts in M-mode (see
`kernel/entry.S`/`kernel/start.c`) and immediately drops itself to S-mode by
hand (writes a few CSRs, then `mret`) — the whole OS acting as its own tiny
one-shot M-mode firmware. Real hardware doesn't allow skipping this:
something else (OpenSBI) already owns M-mode by the time your OS runs.

Device tree: xv6 booting on the Orange Pi RV2 could go either way —
hard-code this one board's known addresses (simplest, matches this repo's
existing ARM-port style) or actually parse the `.dtb` OpenSBI/U-Boot hands
you (more "proper", more work, unnecessary for a single fixed board).
**Recommend hard-coding**, at least for a first working version.

## What we already know

- `forks/riscv64` currently only ever boots QEMU's generic `-machine virt`
  platform with `-bios none`: xv6 starts directly in M-mode and does its own
  one-shot firmware job by hand in `kernel/start.c` (delegate traps via
  `medeleg`/`mideleg`, open up PMP, enable the Sstc timer extension via
  `menvcfg`, then `mret` into S-mode). No board in this repo has ever run
  under a *real* firmware layer underneath it.

- This port already uses the modern **sstc** extension for timer interrupts
  (`kernel/trap.c`'s `clockintr()` just writes the `stimecmp` CSR directly
  from S-mode) — there is no M-mode timer trap handler anywhere in the
  kernel. Good news: the M-mode footprint is entirely confined to the
  one-shot `kernel/start.c` function, not spread through the runtime.

- Booting under a real OpenSBI (as the Orange Pi RV2 requires) means xv6
  never runs in M-mode at all — OpenSBI already did `start.c`'s job and
  hands off directly to S-mode. Two real blockers, both small and
  mechanical, not deep rewrites:
  - `kernel/entry.S` reads the hart ID via `csrr a1, mhartid` — `mhartid` is
    an M-mode-only CSR, and reading it from S-mode would fault immediately
    as the very first instruction. OpenSBI's S-mode handoff convention
    instead passes hartid in register `a0` (and the device-tree pointer in
    `a1`) — `entry.S` needs a variant that reads `a0` instead of the CSR.
  - `kernel/start.c`'s `r_mhartid()` call (used to seed `tp`) has the same
    problem and the same fix.
  - `kernel/start.c`'s CSR writes themselves
    (`mstatus`/`medeleg`/`mideleg`/`pmp`/`menvcfg`/`mret`) become simply
    **unnecessary** under OpenSBI, not wrong — skip them, don't port them.
  - **Open question**, not answerable from source alone: whether this
    board's OpenSBI build actually enables the Sstc extension
    (`menvcfg.STCE`) for its S-mode payload the way QEMU's does. If not,
    `clockintr()`'s direct `stimecmp` write will fault, and the timer needs
    to fall back to the legacy SBI TIME extension (an `ecall` to OpenSBI to
    arm the next interrupt) instead.

- **No QEMU machine model exists** for the K1/Ky X1 SoC or this board
  (checked QEMU's riscv machine list: `virt`, `sifive_u`,
  `microchip-icicle-kit`, `shakti_c`, `k230`, a few others — nothing
  K1-shaped, no work-in-progress patches found either). So there is no way
  to emulate the *whole* real board. But the fast-loop idea splits in two:
  - The OpenSBI-handoff/S-mode-entry rework above **can** be developed and
    iterated in QEMU: boot `virt` with a real OpenSBI firmware image
    (`-bios default` instead of this repo's current `-bios none`).
    OpenSBI's payload-entry calling convention (hartid in `a0`, dtb in `a1`,
    already-S-mode) is the same on `virt` as it will be on real K1
    hardware — only the peripherals differ. Real, fast-loop-testable work,
    safe to do before the board is involved at all.
  - The K1-specific peripheral drivers (real UART registers, real PLIC base
    address, any clock/pinmux setup the UART needs before it'll talk) have
    **no emulation target**. Real-hardware-only, exactly like
    `arm-pi1`/`arm-pi2`'s own peripheral bring-up — no shortcut available.

- SpacemiT K1's boot chain (mainline path, from `riscv/meta-riscv`'s own
  docs): `BootROM → FSBL.bin (vendor) → OpenSBI → U-Boot → kernel + device
  tree`. Upstream OpenSBI has K1 support, but is **not** compatible with the
  *vendor* U-Boot out of the box (device-tree differences) — SpacemiT
  maintains a patched U-Boot branch (`k1-bl-v2.2.y-opensbi`) to bridge that
  gap. Armbian's build scripts track updates to this vendor OpenSBI/U-Boot
  bundle, likely the most convenient prebuilt source of a working firmware
  image to test against, rather than building SpacemiT's tree from scratch.

- Confirmed from a FreeBSD forum thread of someone else's real first-boot
  attempt on this exact board: the vendor U-Boot (built by Xunlong, Orange
  Pi's manufacturer) is scriptable the normal U-Boot way
  (`bootcmd`/`bootdelay` env vars), and someone already booted a *non-Linux*
  OS (FreeBSD) from a USB stick via U-Boot's `load` + `bootefi` commands.
  Data points for our own bring-up:
  - Serial console works over a USB-TTL adapter at **115200 baud** — our
    primary debugging channel, exactly like the ARM Pi ports' UART consoles.
  - The vendor U-Boot intentionally shortens/hides the `bootdelay`
    countdown — reaching an interactive U-Boot prompt takes some fighting
    (Ctrl-C at the right moment, or any keypress).
  - SD-card boot was reported unreliable ("hangs during device
    enumeration") by that same user — USB-stick boot worked instead. Worth
    re-verifying ourselves, but plan for USB-stick boot as the fallback.
  - That thread used `bootefi` (an EFI-application boot path), implying a
    real UEFI shim for that OS — xv6 has no EFI stub and shouldn't need
    one; U-Boot's plainer `bootm`/`booti` (raw kernel image, no EFI) is more
    likely the right command for us, still to be confirmed on real hardware.

## Proposed phases

**Phase A — research, no hardware/board time needed.**
Find (or extract from the Linux kernel's own K1 device-tree source, and/or
SpacemiT's SDK/U-Boot source) the real, K1-specific addresses this port will
need: UART base MMIO address + register layout, PLIC base address, any
clock-enable step the UART needs before it'll transmit. The upstream Linux
DT patches for the sibling R2S board are a good starting point even though
they're for a different board SKU — same SoC-level bring-up. Decide (and
record here) whether to hard-code addresses or parse the `.dtb` —
recommendation above is hard-code.

**Phase B — fast QEMU loop, OpenSBI-handoff logic only.**
Branch `forks/riscv64`'s `entry.S`/`start.c`: add an OpenSBI-payload entry
variant (hartid from `a0`, no M-mode CSR writes, jump straight to `main()`)
alongside the existing bare-M-mode one, following this repo's general
"prefer runtime detection over `#ifdef`" guidance where practical — though
note this specific choice is genuinely a *build-time* one (which firmware
you're handed off by is decided before the kernel binary is even
built/linked for a target), unlike the ARM Pi ports' QEMU-vs-real-hardware
runtime detection. Boot it on QEMU `virt` with `-bios default` (a real
OpenSBI, not `-bios none`) to confirm the handoff itself works — console
output can keep using `virt`'s existing 16550 UART driver for this step,
since the point is only to validate the M-mode/S-mode boundary, not the K1
peripherals yet. Confirm or refute the Sstc/`menvcfg.STCE` assumption here;
if QEMU's default OpenSBI doesn't enable it either, get the SBI-TIME
fallback working here first, where iteration is fast.

**Phase C — real board bring-up** (slow loop, matches `arm-pi1`/`arm-pi2`'s
own pattern).
Get *something* booting via the vendor U-Boot first — almost certainly
Xunlong's stock Linux image, just to confirm the board, SD card/USB stick,
serial adapter, and power supply all actually work, before touching xv6 at
all. Get a real OpenSBI+U-Boot combination that will boot a raw, non-Linux
kernel image (probably: prebuilt Armbian/SpacemiT firmware bundle, then a
plain U-Boot `bootm` of an xv6 image built with Phase B's entry point) —
reusing Phase B's work unchanged. Write a real UART driver for K1 (register
layout from Phase A) — console output is the first real signal that
anything is alive on this board, exactly as for every ARM Pi port here.
Update the PLIC base address for K1; verify timer/external interrupts fire
for real. From there: same shape as every other bring-up in this repo — boot
as far as possible, write down each real bug found and fixed in a new
`notes_arch_riscv_opi_rv2.txt`, stopping to write up an open problem rather
than guessing past it.

**Phase D — wire into the repo's build system**, once Phase C reaches a real
shell.
This extends an *already wired-up* arch (riscv64) with a second boot target,
rather than adding a wholly new arch per `CLAUDE.md`'s "Adding a new arch"
recipe — closer in shape to how `arm-pi2`/`arm-pi3` added a second `hw=`
board selector to an existing arm Makefile than to onboarding a new ISA.
Likely shape: a `board=opi-rv2` (or similar) selector in `forks/riscv64`'s
Makefile, a `build-riscv64-opi-rv2`/`run-...` pair, folded into the umbrella
targets the same way every other target is. **No QEMU target is possible**
for this specific board — `run-riscv64-opi-rv2` would necessarily mean
"flash real hardware", unlike every other `run-<arch>` target in this repo.
Worth flagging clearly in the Makefile/README so it isn't mistaken for a
CI-able target — it cannot be added to `build-docker`/CI the way
riscv64/i386 were, for the same reason `arm-pi1`'s real-hardware-only paths
aren't in CI either.

## Open risks / unknowns

- Nobody has done this before (checked). No forum thread, blog post, or
  repo mentions xv6 + K1/Ky X1/Orange Pi RV2/R2S. Every step here is
  first-of-its-kind for this board, not a known recipe — treat time
  estimates accordingly.
- "Ky X1" is described by multiple sources as a rebrand/variant of SpacemiT
  K1, not confirmed byte-for-byte identical silicon — register addresses
  pulled from K1/BPI-F3/Milk-V-Jupiter references should be treated as a
  strong starting guess, not ground truth, until verified against something
  Xunlong-specific. Public datasheet/TRM availability for K1 is
  unconfirmed — may need to reverse the real addresses out of the Linux
  device-tree source and/or vendor SDK sources rather than a proper
  datasheet.
- Whether the Sstc timer extension is enabled for us by this board's
  OpenSBI (see Phase B) — unresolved, needs testing, has a known fallback
  (legacy SBI TIME extension) either way.
- Whether the vendor U-Boot will boot an arbitrary raw kernel image at all,
  or only signed/expected images — the FreeBSD thread's use of `bootefi`
  hints at one working path (EFI stub) but xv6 has no EFI stub; U-Boot's
  plainer `bootm`/`booti` is untested for this board by anyone we found and
  needs to be tried against the real board.

## References

- [riscv/meta-riscv — Orange Pi RV2 mainline boot doc](https://github.com/riscv/meta-riscv/blob/master/docs/orangepi-rv2-mainline.md) — the mainline boot-chain doc this plan's "boot chain" section is based on: BootROM → FSBL → OpenSBI → U-Boot → kernel/dtb.
- [Aurélien Jarno — Running upstream OpenSBI on SpacemiT K1](https://blog.aurel32.net/upstream-opensbi-spacemit-k1.html) — real-hardware notes on running upstream (non-vendor) OpenSBI on K1-family boards (Banana Pi BPI-F3, Milk-V Jupiter), no QEMU involved.
- [Armbian PR #9422 — SpacemiT OpenSBI/U-Boot update](https://github.com/armbian/build/pull/9422) — Armbian's tracking of SpacemiT's vendor OpenSBI/U-Boot bundle updates (`k1-bl-v2.2.y-opensbi` branch), likely the most convenient prebuilt firmware source for Phase C.
- [FreeBSD Forums — Orange Pi RV2 first steps](https://forums.freebsd.org/threads/orange-pi-rv2-first-steps.97923/) — someone else's real first-boot log on this exact board: serial console at 115200 baud over USB-TTL, vendor U-Boot `bootcmd`/`bootdelay` quirks, USB-stick boot working where SD card didn't.
- [CNX Software — Orange Pi RV2 specs](https://www.cnx-software.com/2025/03/08/orange-pi-rv2-low-cost-risc-v-sbc-ky-x1-octa-core-soc-2-tops-ai-accelerator/) — board specs: Ky X1/SpacemiT K1 SoC, 8 cores up to 1.6 GHz, 2–8 GB LPDDR4X, 26-pin GPIO header.
- [QEMU RISC-V System emulator docs](https://www.qemu.org/docs/master/system/target-riscv.html) — full riscv machine list, used to confirm no K1/Orange-Pi-specific machine model exists.
- [Linux kernel patch thread — spacemit: Add OrangePi R2S board device tree](https://lkml.iu.edu/2511.1/00856.html) — upstream Linux device-tree patches for the sibling Orange Pi R2S board (same Ky X1/K1 SoC, different board) — a starting reference for peripheral addresses even though it's not the RV2's own device tree.

## Recommended next step

Phase A: pin down the K1 UART and PLIC addresses from the Linux
device-tree sources (R2S patches above, plus any mainline K1 `.dtsi` that's
landed by the time this is picked up), before writing any code. Phase B
(QEMU-only OpenSBI handoff work) can start in parallel and doesn't depend on
Phase A at all, if you'd rather get something runnable quickly.
