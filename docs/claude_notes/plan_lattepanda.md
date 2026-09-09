# Plan: LattePanda (amd64-jserv/x86_64) bring-up

**Status:** proposed, not started. Written 2026-09-09, from a research-only
conversation — no code touched yet. Write a
`notes_arch_amd64_lattepanda.txt` (matching every other port's own findings
file) once real bring-up work starts; this file stays a plan.

## Goal

Get `forks/amd64-jserv` (see "Which fork" below — **not** `forks/amd64`)
booting on the user's own physical LattePanda board, the way
`arm-pi1`/`arm-pi2`/`arm-pi3` reached real Raspberry Pi hardware and
[plan_orange_pi.md](plan_orange_pi.md) proposes for the Orange Pi RV2. Same
overall shape as that plan (real firmware/boot-chain quirks stand between
"boots under QEMU" and "boots on the real board"), but meaningfully more
favorable in two ways worth knowing up front:

1. The specific firmware quirk this board has is a **well-worn, previously
   solved problem** in the wider Linux-on-cheap-x86-tablets community, not a
   from-scratch problem like the Orange Pi RV2's total lack of prior art.
2. **QEMU can closely reproduce this board's actual firmware situation**
   (see "QEMU emulation" below) in a way it flatly could not for the Orange
   Pi's SoC-specific hardware.

## The board

"LattePanda — a Win10 Development Board" (the literal product title on the
official listings) is the original **LattePanda V1**: an Intel Atom x5-Z8350
or Z8300 (Cherry Trail generation, 2015–2016), quad-core, real x86-64
(Silvermont-family core, same ISA as any modern Intel chip), 2–4 GB RAM,
32–64 GB onboard eMMC storage, plus an integrated Arduino Leonardo
co-processor (irrelevant to xv6 — a separate microcontroller on the same
board, not part of the x86 side at all).

If your specific unit isn't this one — there are later, unrelated
"LattePanda Alpha/Delta/3 Delta/Sigma/Mu" boards with completely different,
much more modern Intel chips and firmware — none of this plan's
firmware-specific findings carry over; re-check the label on the board.

## Concept glossary: the x86 firmware nuances that matter here

You already know the PC world, so this is shorter than
`plan_orange_pi.md`'s glossary — just the handful of things easy to get
wrong for this specific board.

| Concept | What it is | Why it matters here |
|---|---|---|
| **BIOS vs UEFI, and CSM** | "Legacy BIOS boot" is the old protocol xv6's boot sector was written for: firmware loads 512 bytes from disk to `0x7c00`, jumps there, done. Most PCs since ~2012 instead boot via UEFI, but almost all also include a **CSM** (Compatibility Support Module) emulating the old protocol so legacy boot sectors still work unmodified. | Open question for this board (see "What we found"): budget tablet-class Atom boards from this era frequently shipped UEFI-only firmware with the CSM stripped out entirely to save flash space — not yet the norm on mainstream desktop PCs at the time. |
| **32-bit UEFI on a 64-bit CPU** | Some Cherry Trail devices ship a firmware whose UEFI implementation is itself compiled as a 32-bit (IA32) program, even though the CPU underneath is fully 64-bit-capable (same flash-space motivation). Such firmware can only directly run a 32-bit UEFI application (a PE32 `.efi`) — it cannot directly load and jump to a 64-bit kernel or bootloader. | A real, well-documented headache for this exact chip family. |
| **GOP framebuffer vs. legacy VGA text mode** | xv6's classic console driver writes text directly into memory at `0xB8000` in the old VGA "text mode" format. That's a legacy-BIOS/CSM concept — pure UEFI graphics output (the GOP, Graphics Output Protocol) instead hands you a linear pixel framebuffer, with no text-mode emulation guaranteed once you've left UEFI boot services. | Whether `0xB8000`-style text mode still works on this board under pure UEFI is unconfirmed — the serial port is the safer first console to bring up, exactly as it was for the first steps of every ARM Pi port in this repo. |
| **Boot loader vs. kernel privilege-mode transition** | Two separate jobs, easy to conflate. "Boot loader" = whatever code firmware runs first, that finds and loads the kernel. "Privilege-mode transition" = the CPU work of moving from 16-bit real mode, through 32-bit protected mode, into 64-bit long mode (where xv6-x86_64 actually runs). | `forks/amd64-jserv` already has real, working code for the second job (`kernel/entry.S` + `kernel/entry64.S`). If this board needs a UEFI-native boot path, only the FIRST job changes (who invokes that code and how it finds the kernel image) — not the CPU-mode-transition logic itself. This is exactly why the fix below is smaller than it might first sound. |

## Which fork: `forks/amd64-jserv`, not `forks/amd64`

This matters a lot and is easy to get wrong by picking whichever fork
sounds more "generic". Checked both Makefiles and both ports' own
`notes_arch_*.txt` files:

- **`forks/amd64`** (MIT's own abandoned 2018 x86-64 experiment) boots via
  QEMU's own multiboot `-kernel` ELF loader — there is **no real boot
  sector**, no `bootasm.S`/`bootmain.c`, no `xv6.img` disk image at all (see
  `notes_arch_amd64.txt`: *"qemu cannot load 64bit ELF kernels... implement
  a 32bit multiboot header and shim"*). That loading mechanism is a QEMU
  convenience feature with no real-hardware equivalent — this fork, as it
  stands, structurally cannot boot on any real PC, Cherry Trail or
  otherwise, without inventing a real boot path from nothing.

- **`forks/amd64-jserv`** (jserv/xv6-x86_64) already has everything a real
  PC needs: a real legacy MBR boot sector
  (`kernel/bootasm.S` + `kernel/bootmain.c`, signed via `tools/sign.pl`,
  written to disk with `dd` in the Makefile's `xv6.img` rule) that reads the
  kernel off disk itself via real BIOS disk services — the same boot
  protocol real PCs have used for decades, not a QEMU shortcut. It also
  already has real ACPI table parsing (`kernel/acpi.c`) and MP-table/
  IOAPIC/LAPIC discovery (`kernel/mp.c`, `kernel/ioapic.c`,
  `kernel/lapic.c`) — meaning it already finds its interrupt controllers and
  CPU topology from real firmware tables at boot, unlike the ARM Pi ports in
  this repo, which hard-code peripheral addresses because they never had
  real firmware describing the board to begin with. A genuine asset for
  real-hardware work here, not a gap to fix.

This plan is entirely about `forks/amd64-jserv` from here on.

## What we found: this board's firmware quirk is a known, solved problem

Multiple independent sources (Arch Linux forums, an antiX Linux forum post,
a bios-mods.com thread, and Linuxium's own community-ISO project) describe
the same thing for this Cherry Trail (Z8350/Z8300) board generation:
**32-bit UEFI firmware, no CSM/legacy-BIOS option, on a fully 64-bit-capable
CPU**. One thread's own summary: *"standard Ubuntu ISOs may not boot on
LattePanda V1 with 32-bit UEFI and no CSM/legacy options."*

The existing community fix — already used to boot full 64-bit Linux kernels
on this same hardware — is a small 32-bit (IA32) UEFI "stub" application
(`bootia32.efi`, or GRUB built with `--target=x86-efi` in its 32-bit form)
that the 32-bit firmware **can** run directly, which then loads and jumps to
the real (64-bit) OS.

This is **not independently re-confirmed** against the user's own specific
unit/BIOS revision in this session — the very first real step (Phase A
below) should be checking this board's own BIOS setup screen for a
Legacy/CSM boot-mode toggle before assuming any of the above, since a
firmware revision could differ from what these threads describe.

If confirmed true for this unit, the shape of the fix for
`forks/amd64-jserv` is: replace `kernel/bootasm.S`/`kernel/bootmain.c`'s job
(the classic BIOS-invoked boot sector) with a minimal 32-bit UEFI
application doing the equivalent job under UEFI's own API instead of BIOS
interrupts — locate the kernel image (embedded/appended the way the ARM Pi
ports' own boot images work, or read from the EFI System Partition), get a
memory map from UEFI boot services, call `ExitBootServices`, then jump into
the **same** `entry.S`/`entry64.S` code this fork already has for the
32-bit-protected-mode → 64-bit-long-mode transition — that code doesn't care
who invoked it. Real, non-trivial new code, but not a rewrite of the hard
part; it's the "boot loader" job redone for a different firmware API,
reusing the part that was actually hard to get right the first time.

## QEMU emulation: a genuinely better story than the Orange Pi RV2 plan

`plan_orange_pi.md`'s equivalent section is mostly bad news (no QEMU machine
model exists for that SoC at all). This board's situation is different,
because the hardware itself is much less exotic — it's a generic PC, just
with unusual firmware — and QEMU already has the pieces to reproduce exactly
that:

- `forks/amd64-jserv` already boots normally on QEMU's default PC machine
  with SeaBIOS (legacy BIOS) — needs no new work at all, and stays useful
  as-is for anything not related to the firmware-boot-path question itself.
- Tianocore/EDK2 (the project behind OVMF, QEMU's usual UEFI firmware) also
  builds an **"OVMF32"/"Ia32X64"** firmware variant, specifically meant to
  reproduce this exact situation: a 32-bit UEFI PEI/DXE phase running a
  64-bit-capable CPU, runnable under plain `qemu-system-x86_64` (per
  Tianocore's own wiki: *"the IA32 build of OVMF has more flexibility since
  the X64 processor is compatible with IA32"*). The UEFI-stub work above can
  most likely be developed and iterated in the normal fast QEMU loop — boot
  QEMU's `q35` machine with this 32-bit OVMF build instead of the default
  SeaBIOS, and you have a close reproduction of this board's own firmware
  situation, without the board anywhere nearby. Needs to actually be tried
  (building or finding a prebuilt OVMF32 image is Phase A/B work, not yet
  done in this session) but is a real, documented capability, not a guess.
- What QEMU/OVMF32 **can't** tell you: real eMMC-vs-QEMU's-virtio-disk
  differences, and whether this exact board's shipped firmware actually
  matches the generic Cherry Trail picture above (see "Open risks"). Those
  stay real-hardware-only questions.

## Proposed phases

**Phase A — research + a firsthand look at THIS board's own BIOS/UEFI setup
screen.**
Power on the actual board, enter its firmware setup screen (per the
LattePanda docs: Esc or Del during boot), and check directly for a
Legacy/CSM boot-mode option and whether the firmware identifies itself as
32-bit or 64-bit. This single check determines which of the two very
different boot-path strategies below is needed, and should happen before
writing any code.
- If a CSM/Legacy toggle **does** exist: the existing
  `bootasm.S`/`bootmain.c` path may work with zero changes once Legacy mode
  is selected — try it first, it's nearly free to test.
- If no CSM exists (matching what the community threads describe): proceed
  to the UEFI-stub plan above. Look for whether GRUB's existing `i386-efi`
  (bootia32.efi-style) target can be reused directly as the stub, rather
  than writing UEFI boot-services code from scratch — reusing a
  known-working piece here would meaningfully shrink this phase.

**Phase B — fast QEMU loop.**
Get a 32-bit OVMF ("OVMF32"/"Ia32X64") build, or a prebuilt equivalent,
running under `qemu-system-x86_64`'s `q35` machine, and confirm it
reproduces the "32-bit firmware, 64-bit guest" situation before trusting it
as a stand-in for the real board. Develop the UEFI-stub boot path (if Phase
A determined it's needed) against this QEMU setup — fast iteration, no board
involved yet. In parallel, switch `forks/amd64-jserv`'s own disk story to
the memfs build already wired into its Makefile (`xv6memfs.img`/
`kernelmemfs.elf` targets, `qemu-memfs` rule) — a memory-resident filesystem
baked directly into the kernel image, needing no block-device driver at
all. This sidesteps the real-hardware storage-driver problem entirely for a
first bring-up (see Phase C) and is already a working, tested build mode in
this fork today — free to switch to.

**Phase C — real board bring-up** (slow loop, same shape as every ARM Pi
port here).
Boot the UEFI-stub (or Legacy-mode) kernel on the real board. Bring up the
serial console (`kernel/uart.c`'s existing COM1 driver) as the first success
signal, not VGA text mode — safer given the GOP-vs-text-mode uncertainty
above, and it's exactly the same choice every ARM Pi port in this repo made
for its own first real signal of life. Confirm the memfs kernel actually
runs userspace programs and reaches a shell over that serial console. Only
once that works: decide whether real persistent storage is worth the
effort, and if so what kind — LattePanda V1's onboard storage is eMMC (plus
a microSD slot), neither of which the existing `kernel/ide.c` legacy-PIO-ATA
driver can see at all (there is no legacy IDE controller on this SoC
generation) — a real eMMC/SDHCI driver, or booting instead from a
USB-attached disk via a USB mass-storage driver, would both be real,
from-scratch driver work, on the order of the ARM Pi ports' own USB/
peripheral bring-up effort. Not needed for a first working shell. Write up
each real bug found, same as every `notes_arch_*.txt` file in this repo — a
new `notes_arch_amd64_lattepanda.txt`.

**Phase D — wire into the repo's build system**, once Phase C reaches a real
shell.
Same shape as `plan_orange_pi.md`'s own Phase D: this extends an
already-wired-up arch (amd64) with a second, real-hardware boot target,
rather than onboarding a new arch per `CLAUDE.md`'s "Adding a new arch"
recipe. No CI target is possible for the real-hardware path (same reasoning
as the Orange Pi RV2 plan) — but unlike that plan, the UEFI-stub boot-path
work itself **could plausibly get a QEMU+OVMF32-based CI/test target**,
since Phase B's setup doesn't depend on the physical board at all. Worth
revisiting once Phase B exists.

## Open risks / unknowns

- The single biggest unknown: whether THIS specific board/BIOS revision
  actually matches the "32-bit UEFI, no CSM" picture the community threads
  describe for Cherry Trail LattePanda boards in general. Not independently
  confirmed in this session — Phase A's firsthand BIOS-menu check settles
  it either way, cheaply, before any code is written.
- Whether GOP/pure-UEFI leaves `0xB8000` usable as legacy VGA text mode on
  this board — unconfirmed; the plan above avoids depending on the answer
  by using the serial console first.
- The scope of "write a real eMMC or USB-mass-storage driver" (Phase C's
  optional last step) is not estimated here at all — could be comparable to
  the ARM Pi ports' own USB/peripheral work (real, multi-session effort)
  and is explicitly deferred, not required for a first working shell thanks
  to the memfs option.
- Whether a prebuilt OVMF32/Ia32X64 firmware image is readily available (a
  Linux distro package) or needs building from EDK2 source — not checked
  yet, first thing to verify in Phase B.
- Confirm which exact LattePanda variant this is (V1 Z8350 vs Z8300 — both
  plausible for "Win10 Development Board", differ only in clock speed, not
  in any of the firmware findings above) by reading the sticker/label on
  the board itself, and rule out this being one of the later, unrelated
  Alpha/Delta/Sigma/Mu boards (different SoCs entirely, none of this plan's
  findings would carry over).

## References

- [LattePanda 4G/64GB — Amazon product listing](https://www.amazon.com/LattePanda-2G-32GB-Development-Board/dp/B07P7KRRC3) — the actual product listing this plan's board identification is based on ("a Win10 Development Board", matching the user's own description) — Atom x5-Z8350, 4 GB/64 GB variant shown.
- [Wikipedia — LattePanda](https://en.wikipedia.org/wiki/LattePanda) — general LattePanda V1 vs. later Alpha/Delta board-family overview.
- [LattePandaTeam/LattePanda-Win10-Software — Bios README](https://github.com/LattePandaTeam/LattePanda-Win10-Software/blob/master/Bios/README.md) — official BIOS-image repository for V1.0/1.1/1.2; confirms distinct per-revision BIOS builds exist but documents no Legacy/CSM toggle or 64-bit-UEFI option in the file names/README itself.
- [bios-mods.com — 64-bit EFI for Cherry Trail Mini-PC](https://www.bios-mods.com/forum/Thread-64-bit-EFI-for-Cherry-Trail-Mini-PC) — a Cherry Trail mini-PC (same chip family) owner's real experience with "32-bit EFI only" blocking 64-bit OS installs, solved in that case by flashing a different board's 64-bit firmware — illustrates the failure mode, not a fix applicable to LattePanda specifically.
- antiX-forum thread on a "hybrid 32/64-bit UEFI boot loader" for Cherry Trail tablets (exact page returned 403 on fetch; finding corroborated by the other links here) — the `bootia32.efi`-style stub-loader technique this plan's Phase A/B UEFI-stub approach is modeled on.
- Arch Linux forum and Linuxium-ISO discussions (web-search summary; specific pages not independently fetched) confirming "standard Ubuntu ISOs may not boot on LattePanda V1 with 32-bit UEFI and no CSM/legacy options", and that Linuxium's pre-patched ISOs (adding a 32-bit GRUB stub) are the known community workaround.
- [Tianocore — How to run OVMF](https://github.com/tianocore/tianocore.github.io/wiki/How-to-run-OVMF) and [Testing SMM with QEMU, KVM and libvirt](https://github.com/tianocore/tianocore.github.io/wiki/Testing-SMM-with-QEMU,-KVM-and-libvirt) — Tianocore/EDK2's own docs confirming an IA32/Ia32X64 OVMF build exists and runs under `qemu-system-x86_64`, the basis for this plan's Phase B fast-loop proposal.

## Recommended next step

Phase A's firsthand BIOS-menu check on the real board (five minutes,
determines which of two very different boot strategies this plan needs)
before anything else — it's the one step nothing else in this plan can
substitute for.
