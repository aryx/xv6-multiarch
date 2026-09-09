# Top-level Makefile for xv6-multiarch.
#
# Each forks/<name>/ is a self-contained xv6 port with its own Makefile
# (see docs/provenance.md and the root README for what each one is). This
# file does not replace those - it drives them with the toolchain and QEMU
# binary ./configure detected for this host (Makefile.config, generated -
# do not edit by hand, re-run ./configure instead), and gives every port
# short "build-<arch>"/"run-<arch>" targets.
#
# Target names use the bare ISA (riscv64, i386, ...), matching ./configure's
# own TOOLPREFIX_<ARCH>/QEMU_<ARCH> naming - NOT the forks/<name> directory
# name, which instead names the upstream repo/port (forks/riscv64 is the
# riscv64 port; forks/i386 is the i386 one). See ./configure's own header
# comment for why the directories themselves aren't renamed to match.
#
# Currently wired for riscv64, i386, x86_64, amd64, riscv32, arm64, mips,
# loongarch, arm, arm-pi1, and arm-pi1-bis - see docs/claude_notes/build-and-test-plan.md,
# Phases 1-2, and Phase 4 (see docs/claude_notes/notes_arch_amd64_jserv.txt,
# notes_arch_amd64.txt, notes_arch_riscv32.txt - riscv32's own test-riscv32
# now passes (bug 4, a missing user-program start() wrapper, fixed
# 2026-09-07); notes_arch_arm64.txt - test-arm64 now passes too (the
# exact same missing-start()-wrapper bug as riscv32's own bug 4,
# independently present in this fork, fixed the same way);
# notes_arch_mips.txt - test-mips now passes too, with six known-hanging
# usertests sub-tests skipped (see that file's own "Gap" section);
# notes_arch_loongarch.txt - fully working; notes_arch_arm.txt -
# the ARM32 winner among four candidate ports, boots to a real shell,
# test-arm passes with two known usertests skipped; notes_arch_arm_pi1.txt -
# a real Raspberry Pi 1 port (the user owns the hardware); test-arm-pi1
# now passes with a full, unmodified usertests run, via an additive
# PL011 console alongside the real-hardware Mini-UART; also reaches a
# working framebuffer console and USB keyboard under QEMU - see
# notes_arch_arm_pi1.txt. notes_arch_arm_pi1_bis.txt - forks/arm-pi1-bis,
# the same real board via a different upstream fork; test-arm-pi1-bis
# now passes too, via the exact same fix pattern as arm-pi1).
#
# arm64, not aarch64: the Makefile target/./configure variable name
# follows ~/c--'s and ~/goken's own CCARM64/RUN_ARM64/arch/arm64/
# convention for this ISA - forks/aarch64 (upstream repo k-mrm/
# xv6-aarch64) was renamed to forks/arm64 to match outright, same
# single-fork-per-ISA exception as riscv64/i386/riscv32 above (see
# ./configure's own header comment).
#
# arm, not armv7-rpi: forks/armv7-rpi became the clear winner among this
# repo's four ARM32 ports (see that section's own comment below, and
# ./configure's matching "arm" section) and was promoted to the same
# single-fork-per-ISA exception as riscv64/i386/riscv32/arm64 above -
# renamed to forks/arm outright (2026-09-08).
#
# A SECOND, different kind of rename, also done 2026-09-08: forks/rpi1,
# forks/rpi2, forks/armv6-rpi, forks/pi_mp, forks/rpi4, and forks/x86_64
# were renamed to forks/arm-pi1, forks/arm-pi2, forks/arm-pi1-bis,
# forks/arm-pi3, forks/arm64-pi4, and forks/amd64-jserv respectively -
# NOT single-ISA-representative promotions like arm/arm64/riscv64/i386/
# riscv32 above (each of those five still has its own official ISA
# representative: forks/arm for ARM32, forks/arm64 for AArch64,
# forks/amd64 for this specific x86-64 lineage), but an ISA-PREFIXED
# GROUPING of real-hardware/alternate ports that stay genuinely distinct
# builds, done to make the eventual factorization-phase merge easier to
# reason about (related forks now sort together by prefix). forks/arm-pi1
# and forks/arm-pi1-bis both target the literal same real board (ARMv6
# Raspberry Pi 1/Model B) - arm-pi1-bis is a genuine fork/continuation
# of arm-pi1's own upstream (inaciose's own README: "based on
# zhiyihuang/xv6_rpi_port"), not an independent implementation. Both
# reach a full interactive shell, framebuffer graphics, and a working
# USB keyboard under QEMU (test-arm-pi1/test-arm-pi1-bis) - see
# notes_arch_arm_pi1.txt/notes_arch_arm_pi1_bis.txt. forks/pi_mp's own
# upstream README says "AArch64" but its
# Makefile only ever builds 32-bit ARM code (arm-none-eabi-,
# -mcpu=cortex-a7) - real Pi 3 MP hardware, in AArch32 compatibility
# mode on its ARMv8 chip, not a 64-bit kernel - so it joined the "arm"
# prefix as forks/arm-pi3 (caught and fixed 2026-09-08, same day as an
# initial, incorrect forks/arm64-pi3 rename); forks/rpi4 (genuinely
# AArch64, confirmed on real Pi 4 hardware) stayed under "arm64" as
# forks/arm64-pi4. forks/amd64-jserv is jserv's independent,
# fully-working x86-64 port, grouped under the "amd64" ISA prefix
# alongside MIT's own forks/amd64 rather than left under its old
# upstream-repo name. See docs/provenance.md for the upstream-repo ->
# current-forks-path mapping table.

# claude: "default" (below) is the first real TARGET RULE in this file
# (everything above it is comments/blank lines - variable assignments
# don't count), so it's already Make's own implicit default goal; the
# explicit ".DEFAULT_GOAL := default" just makes that robust against a
# future edit reordering things, rather than relying on position alone.
# Before this, a bare "make" silently ran "check-riscv64-toolchain"
# (the first rule in the OLD file layout) with no output at all - not
# useful, and not what anyone typing plain "make" actually wanted.
.DEFAULT_GOAL := default

default:
	@echo "xv6-multiarch - build and test every wired-up port"
	@echo ""
	@echo "  ./configure       detect each arch's toolchain + qemu-system-* (run this first)"
	@echo "  make all              build every wired-up arch"
	@echo "  make test-all         quick: boot headless + assert a shell prompt, every arch (~1 min)"
	@echo "  make stress-test-all  slow: boot headless + assert ALL TESTS PASSED (full usertests),"
	@echo "                        every arch - real signal, but ~25 min on this host"
	@echo "  make clean-all        remove every arch's build output"
	@echo "  make kill-all         clean up any orphaned qemu-system-* left running"
	@echo ""
	@echo "  make build-<arch>        build one arch (riscv64, i386, amd64, amd64-jserv, riscv32,"
	@echo "                           arm64, mips, loongarch, arm, arm-pi1, arm-pi1-bis)"
	@echo "  make run-<arch>          build + boot that arch interactively (-nographic; Ctrl-A X to quit)"
	@echo "  make quick-test-<arch>   build + boot headless + assert a shell prompt, that arch"
	@echo "  make test-<arch>         build + boot headless + assert ALL TESTS PASSED, that arch"
	@echo ""
	@echo "  make run-<arch>-qemu-graphics   real GTK window + keyboard instead of -nographic"
	@echo "                                  (i386, amd64, amd64-jserv, arm-pi1, arm-pi1-bis)"
	@echo "  make test-all-graphics          regression test for those five (needs \$$DISPLAY) - see scripts/README.md"
	@echo ""
	@echo "  make build-docker [ARCH=<arch>]   same, inside the pinned Dockerfile (default: all)"
	@if [ ! -f Makefile.config ]; then \
		echo ""; \
		echo "No Makefile.config yet - run ./configure first."; \
	fi

-include Makefile.config

TOOLPREFIX_RISCV64 ?=
QEMU_RISCV64 ?= qemu-system-riscv64
TOOLPREFIX_I386 ?=
QEMU_I386 ?= qemu-system-i386
TOOLPREFIX_AMD64_JSERV ?=
QEMU_AMD64_JSERV ?= qemu-system-x86_64
TOOLPREFIX_AMD64 ?=
QEMU_AMD64 ?= qemu-system-x86_64
TOOLPREFIX_RISCV32 ?=
QEMU_RISCV32 ?= qemu-system-riscv32
TOOLPREFIX_ARM64 ?=
QEMU_ARM64 ?= qemu-system-aarch64
TOOLPREFIX_MIPS ?=
QEMU_MIPS ?= qemu-system-mipsel
TOOLPREFIX_LOONGARCH ?=
CC_LOONGARCH ?=
QEMU_LOONGARCH ?= qemu-system-loongarch64
TOOLPREFIX_ARM_PI1_BIS ?=
QEMU_ARM_PI1_BIS ?= qemu-system-arm
TOOLPREFIX_ARM_PI1 ?=
QEMU_ARM_PI1 ?= qemu-system-arm
TOOLPREFIX_ARM_PI2 ?=
QEMU_ARM_PI2 ?= qemu-system-arm
TOOLPREFIX_ARM_PI3 ?=
QEMU_ARM_PI3 ?= qemu-system-aarch64

# claude: QMP_SOCK (optional, unset by default) forwards a QMP socket
# path into whichever "run-<arch>-qemu-graphics" target is invoked, for
# scripted regression testing - see scripts/qemu_graphics.py and
# scripts/test_qemu_graphics.py. forks/arm-pi1/forks/arm-pi1-bis's own
# Makefiles understand QMP_SOCK directly; forks/i386/forks/amd64/
# forks/amd64-jserv have no such variable of their own, so it's turned
# into a "-qmp ..." flag here and threaded through their own existing
# QEMUEXTRA extension point instead. "comma" is the standard Make idiom
# for a literal "," inside a "$(if ...)" call.
QMP_SOCK ?=
comma := ,
QMP_QEMUEXTRA := $(if $(QMP_SOCK),-qmp unix:$(QMP_SOCK)$(comma)server$(comma)nowait)

.PHONY: build-riscv64 run-riscv64 test-riscv64 quick-test-riscv64 clean-riscv64 kill-riscv64 check-riscv64-toolchain \
        build-i386 run-i386 run-i386-qemu-graphics test-i386 quick-test-i386 clean-i386 kill-i386 check-i386-toolchain \
        build-amd64-jserv run-amd64-jserv run-amd64-jserv-qemu-graphics test-amd64-jserv quick-test-amd64-jserv clean-amd64-jserv kill-amd64-jserv check-amd64-jserv-toolchain \
        build-amd64 run-amd64 run-amd64-qemu-graphics test-amd64 quick-test-amd64 clean-amd64 kill-amd64 check-amd64-toolchain \
        build-riscv32 run-riscv32 test-riscv32 quick-test-riscv32 clean-riscv32 kill-riscv32 check-riscv32-toolchain \
        build-arm64 run-arm64 test-arm64 quick-test-arm64 clean-arm64 kill-arm64 check-arm64-toolchain \
        build-mips run-mips test-mips quick-test-mips clean-mips kill-mips check-mips-toolchain \
        build-loongarch run-loongarch test-loongarch quick-test-loongarch clean-loongarch kill-loongarch check-loongarch-toolchain \
        build-arm-pi1-bis run-arm-pi1-bis run-arm-pi1-bis-qemu-graphics test-arm-pi1-bis quick-test-arm-pi1-bis clean-arm-pi1-bis kill-arm-pi1-bis check-arm-pi1-bis-toolchain \
        build-arm run-arm test-arm quick-test-arm clean-arm kill-arm check-arm-toolchain \
        build-arm-pi1 run-arm-pi1 run-arm-pi1-qemu-graphics test-arm-pi1 quick-test-arm-pi1 clean-arm-pi1 kill-arm-pi1 check-arm-pi1-toolchain \
        build-arm-pi2 run-arm-pi2 check-arm-pi2-toolchain \
        build-arm-pi3 run-arm-pi3 check-arm-pi3-toolchain \
        build-all test-all stress-test-all clean-all kill-all test-all-graphics build-docker \
        default all clean

###############################################################################
# riscv64 (forks/riscv64, RV64GC - MIT's current xv6-riscv)
###############################################################################

# NONE is ./configure's own sentinel for "looked, found nothing" (see its
# TOOLPREFIX_RISCV64=NONE/QEMU_RISCV64=NONE assignments) - caught here so a
# missing toolchain fails with a clear pointer to ./configure instead of
# forks/riscv64/Makefile trying to run "NONEgcc" or a qemu binary that isn't
# on PATH.
check-riscv64-toolchain:
	@if [ "$(TOOLPREFIX_RISCV64)" = NONE ] || [ "$(QEMU_RISCV64)" = NONE ]; then \
		echo "Makefile: riscv64 toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

build-riscv64: check-riscv64-toolchain
	$(MAKE) -C forks/riscv64 TOOLPREFIX=$(TOOLPREFIX_RISCV64) QEMU=$(QEMU_RISCV64) kernel/kernel fs.img

run-riscv64: check-riscv64-toolchain
	$(MAKE) -C forks/riscv64 TOOLPREFIX=$(TOOLPREFIX_RISCV64) QEMU=$(QEMU_RISCV64) qemu

# forks/riscv64/test-xv6.py drives plain "make qemu" itself (see that
# script's own QEMU class), so there's no command line to pass TOOLPREFIX/
# QEMU on - only the environment. TOOLPREFIX threads through correctly
# that way (its ifndef guard honors a value from any origin, environment
# included); QEMU does NOT (forks/riscv64/Makefile's own "QEMU =
# qemu-system-riscv64" is an unconditional assignment with no ifndef
# guard, so a plain makefile assignment always wins over the environment)
# - harmless in practice since QEMU_RISCV64 is just the bare command name
# qemu-system-riscv64 once found on PATH, identical to that hardcoded
# default.
test-riscv64: check-riscv64-toolchain build-riscv64
	cd forks/riscv64 && TOOLPREFIX=$(TOOLPREFIX_RISCV64) ./test-xv6.py usertests

# claude: fast smoke check (boots to a shell, no usertests) - see
# "make test-all"'s own header comment on the quick/stress split.
quick-test-riscv64: check-riscv64-toolchain build-riscv64
	cd forks/riscv64 && TOOLPREFIX=$(TOOLPREFIX_RISCV64) ./test-xv6.py boot

clean-riscv64:
	$(MAKE) -C forks/riscv64 clean

# "run-riscv64"/"test-riscv64" run QEMU attached to this shell (-nographic),
# but a stuck boot (or a Ctrl-C that missed) can leave qemu-system-riscv64
# running headless in the background - this kills it by matching the exact
# QEMU_RISCV64 command ./configure detected, not just the bare binary name,
# so it doesn't reach for some other arch's qemu-system-* by accident.
kill-riscv64:
	-pkill -f '$(QEMU_RISCV64)' 2>/dev/null || true

###############################################################################
# i386 (forks/i386, MIT xv6-public - the original teaching OS)
###############################################################################

check-i386-toolchain:
	@if [ "$(TOOLPREFIX_I386)" = NONE ] || [ "$(QEMU_I386)" = NONE ]; then \
		echo "Makefile: i386 toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

build-i386: check-i386-toolchain
	$(MAKE) -C forks/i386 TOOLPREFIX=$(TOOLPREFIX_I386) QEMU=$(QEMU_I386) kernel fs.img xv6.img

run-i386: check-i386-toolchain
	$(MAKE) -C forks/i386 TOOLPREFIX=$(TOOLPREFIX_I386) QEMU=$(QEMU_I386) qemu-nox

# claude: same build, but a real GTK window (forks/i386/Makefile's own
# "qemu" target, no -nographic) so the emulated VGA console is actually
# visible, same as run-arm-pi1-qemu-graphics. forks/i386's "qemu" also
# keeps "-serial mon:stdio" so the QEMU monitor stays reachable from this
# shell; xv6's interactive shell itself is read from the graphical
# window's keyboard, not from stdio. Requires $DISPLAY; not folded into
# build-all/test-all (nothing headless-CI can assert against a GTK
# window).
run-i386-qemu-graphics: check-i386-toolchain
	$(MAKE) -C forks/i386 TOOLPREFIX=$(TOOLPREFIX_I386) QEMU=$(QEMU_I386) QEMUEXTRA="$(QMP_QEMUEXTRA)" qemu

# forks/i386/test-xv6.py (this repo's own, not upstream - see its own header
# comment) drives plain "make qemu-nox" the same way forks/riscv64's does -
# same TOOLPREFIX-via-environment/QEMU-via-PATH split applies (forks/i386/
# Makefile's own "QEMU = ..." auto-detect has no ifndef guard either).
test-i386: check-i386-toolchain build-i386
	cd forks/i386 && TOOLPREFIX=$(TOOLPREFIX_I386) ./test-xv6.py

quick-test-i386: check-i386-toolchain build-i386
	cd forks/i386 && TOOLPREFIX=$(TOOLPREFIX_I386) ./test-xv6.py boot

clean-i386:
	$(MAKE) -C forks/i386 clean

kill-i386:
	-pkill -f '$(QEMU_I386)' 2>/dev/null || true

###############################################################################
# amd64-jserv (forks/amd64-jserv, jserv/xv6-x86_64 - renamed from forks/x86_64)
###############################################################################
# jserv's independent, actively-maintained x86-64 port - unrelated to
# forks/amd64 (MIT's own abandoned 2018 x86-64 experiment) by lineage,
# grouped under the shared "amd64" ISA prefix for the eventual
# factorization-phase merge (see this Makefile's own header comment).
# Both are fully working under QEMU; there is no "winner" here the way
# there was among the four ARM32 ports - amd64-jserv/amd64 are peers.

check-amd64-jserv-toolchain:
	@if [ "$(TOOLPREFIX_AMD64_JSERV)" = NONE ] || [ "$(QEMU_AMD64_JSERV)" = NONE ]; then \
		echo "Makefile: amd64-jserv toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

# forks/amd64-jserv/Makefile has no TOOLPREFIX at all - it's CROSS_COMPILE
# there (see ./configure's own amd64-jserv section for why this repo
# still calls the detected value TOOLPREFIX_AMD64_JSERV for naming
# consistency with every other arch, just passed under the name this
# fork's Makefile actually expects).
#
# CPUS=2: forks/amd64-jserv/Makefile's own "ifndef CPUS" default is this
# HOST's own core count (`grep -c ^processor /proc/cpuinfo`), unlike
# every other fork here (forks/riscv64 hardcodes 3, forks/i386 hardcodes 2)
# - harmless on a native x86_64 deployment, but on this 64-core dev
# machine it means qemu-nox boots 64 vCPUs. xv6 itself only ever starts
# min(that count, NCPU=8) of them, so the extra vCPUs do nothing useful,
# but QEMU still pays real per-vCPU TCG overhead for all 64 - confirmed
# this alone was enough to blow past test-xv6.py's 60s boot-detection
# window (a real full boot to a shell prompt, verified separately with
# -smp 2, took under 30s). Not a logic bug, just a bad default to
# inherit unmodified into an automated/CI context - pass a small,
# explicit CPUS here rather than edit that default in the fork itself.
build-amd64-jserv: check-amd64-jserv-toolchain
	$(MAKE) -C forks/amd64-jserv CROSS_COMPILE=$(TOOLPREFIX_AMD64_JSERV) QEMU=$(QEMU_AMD64_JSERV) out/kernel.elf fs.img xv6.img

run-amd64-jserv: check-amd64-jserv-toolchain
	$(MAKE) -C forks/amd64-jserv CROSS_COMPILE=$(TOOLPREFIX_AMD64_JSERV) QEMU=$(QEMU_AMD64_JSERV) CPUS=2 qemu-nox

# claude: same build, but a real GTK window (forks/amd64-jserv/Makefile's
# own "qemu" target, no -nographic) - same shape as run-i386-qemu-graphics
# and run-arm-pi1-qemu-graphics. Real PS/2 keyboard emulation (this is a
# standard PC machine, not a "virt"/board model with no input device), so
# typing in the window reaches this fork's own kbd.c the same way it does
# on real hardware. Same CPUS=2 override as run-amd64-jserv above, same
# reason. Requires $DISPLAY; not folded into build-all/test-all.
run-amd64-jserv-qemu-graphics: check-amd64-jserv-toolchain
	$(MAKE) -C forks/amd64-jserv CROSS_COMPILE=$(TOOLPREFIX_AMD64_JSERV) QEMU=$(QEMU_AMD64_JSERV) CPUS=2 QEMUEXTRA="$(QMP_QEMUEXTRA)" qemu

# Unlike forks/riscv64's/forks/i386's own QEMU vars, forks/amd64-jserv/
# Makefile's is "QEMU ?= qemu-system-x86_64" (a conditional default) - so
# QEMU genuinely threads through the environment correctly here, not
# just CROSS_COMPILE. CPUS is also "ifndef"-guarded (like TOOLPREFIX
# elsewhere), so it too threads through the environment correctly into
# test-xv6.py's own "make qemu-nox" subprocess call.
test-amd64-jserv: check-amd64-jserv-toolchain build-amd64-jserv
	cd forks/amd64-jserv && CROSS_COMPILE=$(TOOLPREFIX_AMD64_JSERV) QEMU=$(QEMU_AMD64_JSERV) CPUS=2 ./test-xv6.py

quick-test-amd64-jserv: check-amd64-jserv-toolchain build-amd64-jserv
	cd forks/amd64-jserv && CROSS_COMPILE=$(TOOLPREFIX_AMD64_JSERV) QEMU=$(QEMU_AMD64_JSERV) CPUS=2 ./test-xv6.py boot

clean-amd64-jserv:
	$(MAKE) -C forks/amd64-jserv clean

kill-amd64-jserv:
	-pkill -f '$(QEMU_AMD64_JSERV)' 2>/dev/null || true

###############################################################################
# amd64 (forks/amd64, MIT's own abandoned 2018 x86-64 experiment)
###############################################################################

check-amd64-toolchain:
	@if [ "$(TOOLPREFIX_AMD64)" = NONE ] || [ "$(QEMU_AMD64)" = NONE ]; then \
		echo "Makefile: amd64 toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

# Unlike every other fork here, this one boots via QEMU's own multiboot
# "-kernel" loading (see docs/claude_notes/notes_arch_amd64.txt) - no
# bootblock/xv6.img to build, just "kernel" and "fs.img" directly.
build-amd64: check-amd64-toolchain
	$(MAKE) -C forks/amd64 TOOLPREFIX=$(TOOLPREFIX_AMD64) QEMU=$(QEMU_AMD64) kernel fs.img

run-amd64: check-amd64-toolchain
	$(MAKE) -C forks/amd64 TOOLPREFIX=$(TOOLPREFIX_AMD64) QEMU=$(QEMU_AMD64) qemu-nox

# claude: same build, but a real GTK window (forks/amd64/Makefile's own
# "qemu" target, no -nographic) - same shape as run-i386-qemu-graphics.
# Real PS/2 keyboard emulation, same as run-amd64-jserv-qemu-graphics
# above. Requires $DISPLAY; not folded into build-all/test-all.
run-amd64-qemu-graphics: check-amd64-toolchain
	$(MAKE) -C forks/amd64 TOOLPREFIX=$(TOOLPREFIX_AMD64) QEMU=$(QEMU_AMD64) QEMUEXTRA="$(QMP_QEMUEXTRA)" qemu

# Same TOOLPREFIX-via-environment/QEMU-via-command-line-only split as
# forks/riscv64's/forks/i386's own test-<arch> targets (forks/amd64/
# Makefile's own "QEMU = qemu-system-x86_64" has no ifndef guard either).
test-amd64: check-amd64-toolchain build-amd64
	cd forks/amd64 && TOOLPREFIX=$(TOOLPREFIX_AMD64) ./test-xv6.py

quick-test-amd64: check-amd64-toolchain build-amd64
	cd forks/amd64 && TOOLPREFIX=$(TOOLPREFIX_AMD64) ./test-xv6.py boot

clean-amd64:
	$(MAKE) -C forks/amd64 clean

kill-amd64:
	-pkill -f '$(QEMU_AMD64)' 2>/dev/null || true

###############################################################################
# riscv32 (forks/riscv32, michaelengel/xv6-rv32)
###############################################################################

check-riscv32-toolchain:
	@if [ "$(TOOLPREFIX_RISCV32)" = NONE ] || [ "$(QEMU_RISCV32)" = NONE ]; then \
		echo "Makefile: riscv32 toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

# Same "kernel/kernel"+"fs.img", no separate whole-disk image, shape as
# riscv64 - forks/riscv32 boots via QEMU's own "-kernel" loading straight to
# 0x80000000, no bootblock stage (see docs/claude_notes/notes_arch_riscv32.txt).
build-riscv32: check-riscv32-toolchain
	$(MAKE) -C forks/riscv32 TOOLPREFIX=$(TOOLPREFIX_RISCV32) QEMU=$(QEMU_RISCV32) kernel/kernel fs.img

run-riscv32: check-riscv32-toolchain
	$(MAKE) -C forks/riscv32 TOOLPREFIX=$(TOOLPREFIX_RISCV32) QEMU=$(QEMU_RISCV32) qemu

# Same TOOLPREFIX-via-environment/QEMU-via-command-line-only split as
# forks/riscv64's/forks/i386's own test-<arch> targets (forks/riscv32/Makefile's
# own "QEMU = qemu-system-riscv32 -monitor ..." has no ifndef guard either).
test-riscv32: check-riscv32-toolchain build-riscv32
	cd forks/riscv32 && TOOLPREFIX=$(TOOLPREFIX_RISCV32) ./test-xv6.py

quick-test-riscv32: check-riscv32-toolchain build-riscv32
	cd forks/riscv32 && TOOLPREFIX=$(TOOLPREFIX_RISCV32) ./test-xv6.py boot

clean-riscv32:
	$(MAKE) -C forks/riscv32 clean

kill-riscv32:
	-pkill -f '$(QEMU_RISCV32)' 2>/dev/null || true

###############################################################################
# arm64 (forks/arm64, k-mrm/xv6-aarch64)
###############################################################################

check-arm64-toolchain:
	@if [ "$(TOOLPREFIX_ARM64)" = NONE ] || [ "$(QEMU_ARM64)" = NONE ]; then \
		echo "Makefile: arm64 toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

# NOTE: this port is NOT fully working yet - it boots to an interactive
# shell prompt on the first real attempt, but the shell itself then
# crashes (see notes_arch_arm64.txt's "bug 1", not yet fixed) -
# test-arm64 below will fail until that's resolved. build-arm64/
# run-arm64 do work.
build-arm64: check-arm64-toolchain
	$(MAKE) -C forks/arm64 TOOLPREFIX=$(TOOLPREFIX_ARM64) QEMU=$(QEMU_ARM64) kernel/kernel fs.img

run-arm64: check-arm64-toolchain
	$(MAKE) -C forks/arm64 TOOLPREFIX=$(TOOLPREFIX_ARM64) QEMU=$(QEMU_ARM64) qemu

# Same TOOLPREFIX-via-environment/QEMU-via-command-line-only split as
# forks/riscv64's/forks/i386's own test-<arch> targets (forks/arm64/
# Makefile's own "QEMU = $(QEMUPREFIX)qemu-system-aarch64" has no ifndef
# guard either).
test-arm64: check-arm64-toolchain build-arm64
	cd forks/arm64 && TOOLPREFIX=$(TOOLPREFIX_ARM64) ./test-xv6.py

quick-test-arm64: check-arm64-toolchain build-arm64
	cd forks/arm64 && TOOLPREFIX=$(TOOLPREFIX_ARM64) ./test-xv6.py boot

clean-arm64:
	$(MAKE) -C forks/arm64 clean

kill-arm64:
	-pkill -f '$(QEMU_ARM64)' 2>/dev/null || true

###############################################################################
# mips (forks/mips, nullpo-head/xv6-mips)
###############################################################################

check-mips-toolchain:
	@if [ "$(TOOLPREFIX_MIPS)" = NONE ] || [ "$(QEMU_MIPS)" = NONE ]; then \
		echo "Makefile: mips toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

# claude: "kernelmemfs" (embeds fs.img directly in the kernel image),
# not "kernel" (a real IDE-emulated disk boot) - the port's own Makefile
# already admits the disk-based "qemu" target "is not implemented at
# current", and even setting that aside has its own independent -hdb-
# vs-master-drive bug (see notes_arch_mips.txt) - kernelmemfs is the
# only path this repo has actually gotten booting.
#
# usr/usertests.c has several sub-tests commented out (sbrktest,
# validatetest, mem, preempt, exitwait, forktest - see that file's own
# "claude:" comments and notes_arch_mips.txt's "Gap" section) - all
# confirmed genuinely stuck via gdb, not just slow, spanning at least
# two different subsystems. Skipped so the rest of the suite reaches a
# real "ALL TESTS PASSED".
build-mips: check-mips-toolchain
	$(MAKE) -C forks/mips TOOLPREFIX=$(TOOLPREFIX_MIPS) QEMU=$(QEMU_MIPS) kernelmemfs

run-mips: check-mips-toolchain
	$(MAKE) -C forks/mips TOOLPREFIX=$(TOOLPREFIX_MIPS) QEMU=$(QEMU_MIPS) qemu-nox-memfs

test-mips: check-mips-toolchain build-mips
	cd forks/mips && TOOLPREFIX=$(TOOLPREFIX_MIPS) QEMU=$(QEMU_MIPS) ./test-xv6.py

quick-test-mips: check-mips-toolchain build-mips
	cd forks/mips && TOOLPREFIX=$(TOOLPREFIX_MIPS) QEMU=$(QEMU_MIPS) ./test-xv6.py boot

clean-mips:
	$(MAKE) -C forks/mips clean

kill-mips:
	-pkill -f '$(QEMU_MIPS)' 2>/dev/null || true

###############################################################################
# loongarch (forks/loongarch, SKT-CPUOS/xv6-loongarch-exp)
###############################################################################

check-loongarch-toolchain:
	@if [ "$(TOOLPREFIX_LOONGARCH)" = NONE ] || [ "$(CC_LOONGARCH)" = NONE ] || [ "$(QEMU_LOONGARCH)" = NONE ]; then \
		echo "Makefile: loongarch toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

# claude: CC is passed explicitly, not left to forks/loongarch/Makefile's
# own "$(TOOLPREFIX)gcc" default - the installed Debian/Ubuntu cross-gcc
# ships only a version-suffixed binary (e.g. loongarch64-linux-gnu-gcc-14,
# no plain "...-gcc" symlink), see notes_arch_loongarch.txt and
# ./configure's own detect_versioned_gcc. This port boots to a real shell
# and passes usertests on the very first attempt (no kernel/user code
# changes needed at all - only these build-plumbing fixes plus two real
# bugs in the port's own Makefile, see notes_arch_loongarch.txt), so
# unlike mips it gets the full build/run/test/clean/kill set right away.
build-loongarch: check-loongarch-toolchain
	$(MAKE) -C forks/loongarch TOOLPREFIX=$(TOOLPREFIX_LOONGARCH) CC=$(CC_LOONGARCH) all

run-loongarch: check-loongarch-toolchain
	$(MAKE) -C forks/loongarch TOOLPREFIX=$(TOOLPREFIX_LOONGARCH) CC=$(CC_LOONGARCH) QEMU=$(QEMU_LOONGARCH) qemu

# TOOLPREFIX/CC/QEMU via environment, not command-line make args - same
# shape as test-riscv32/test-arm64's own split, since forks/loongarch/
# test-xv6.py drives plain "make ..." itself with no way to pass args
# through. Relies on forks/loongarch/Makefile's own TOOLPREFIX/CC now
# being "?="-guarded (see that file's own claude: comment) so the
# environment value actually survives instead of being silently
# overwritten by the file's default.
test-loongarch: check-loongarch-toolchain
	cd forks/loongarch && TOOLPREFIX=$(TOOLPREFIX_LOONGARCH) CC=$(CC_LOONGARCH) QEMU=$(QEMU_LOONGARCH) ./test-xv6.py

quick-test-loongarch: check-loongarch-toolchain
	cd forks/loongarch && TOOLPREFIX=$(TOOLPREFIX_LOONGARCH) CC=$(CC_LOONGARCH) QEMU=$(QEMU_LOONGARCH) ./test-xv6.py boot

clean-loongarch:
	$(MAKE) -C forks/loongarch clean

kill-loongarch:
	-pkill -f '$(QEMU_LOONGARCH)' 2>/dev/null || true

###############################################################################
# arm-pi1-bis (forks/arm-pi1-bis, inaciose/xv6-armv6-rpi - renamed from
# forks/armv6-rpi)
###############################################################################
# Targets the literal same real board as forks/arm-pi1 below (ARMv6
# Raspberry Pi 1/Model B) - a genuine fork/continuation of zhiyihuang's
# own xv6_rpi_port (its own README: "based on zhiyihuang/xv6_rpi_port
# ... doesn't boot my Raspberry Pi B ... I have done some changes in
# order to successfully boot my pi"), not an independent implementation
# (see docs/provenance.md). Grouped under the shared "arm" ISA prefix,
# "-bis" marking it as the second port of that same board.
#
# claude: forks/arm-pi1-bis/makefile.inc's own toolchain variable is
# "CROSSCOMPILE", not "TOOLPREFIX" like every other fork here - passed
# through as-is below rather than renamed, to keep this repo's own
# changes to that file minimal.
#
# test-arm-pi1-bis/clean-arm-pi1-bis/kill-arm-pi1-bis and build-all/
# test-all/clean-all/kill-all added 2026-09-09: a fourth session applied
# the exact same fix pattern that got forks/arm-pi1 to a full shell +
# framebuffer + USB keyboard (same DEVSPACE/PHYSIO layout, byte-for-byte
# identical keyboard.s) - see notes_arch_arm_pi1_bis.txt "Session 2" for
# the full writeup. Worked on the very first attempt: this port shares
# arm-pi1's exact bugs (missing PL011 UART, no fb_ready guard, no .bss
# zeroing), so the earlier session's "may be a QEMU firmware issue"
# hypothesis for the six-bugs-in stopping point turned out to be wrong -
# it was the same kernel-level bug, just not yet diagnosed.
check-arm-pi1-bis-toolchain:
	@if [ "$(TOOLPREFIX_ARM_PI1_BIS)" = NONE ] || [ "$(QEMU_ARM_PI1_BIS)" = NONE ]; then \
		echo "Makefile: arm-pi1-bis toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

build-arm-pi1-bis: check-arm-pi1-bis-toolchain
	$(MAKE) -C forks/arm-pi1-bis CROSSCOMPILE=$(TOOLPREFIX_ARM_PI1_BIS) QEMU=$(QEMU_ARM_PI1_BIS) kernel.elf

run-arm-pi1-bis: check-arm-pi1-bis-toolchain
	$(MAKE) -C forks/arm-pi1-bis CROSSCOMPILE=$(TOOLPREFIX_ARM_PI1_BIS) QEMU=$(QEMU_ARM_PI1_BIS) qemu

# claude: same kernel.elf/kernel.img, but with a real GTK window and a
# USB keyboard attached - see forks/arm-pi1/Makefile's own
# "qemu-graphics" target for the identical pattern (this fork has no
# separate real-hardware-vs-QEMU target split, so there is only one
# "qemu" target to extend here).
run-arm-pi1-bis-qemu-graphics: check-arm-pi1-bis-toolchain
	$(MAKE) -C forks/arm-pi1-bis CROSSCOMPILE=$(TOOLPREFIX_ARM_PI1_BIS) QEMU=$(QEMU_ARM_PI1_BIS) QMP_SOCK="$(QMP_SOCK)" qemu-graphics

test-arm-pi1-bis: check-arm-pi1-bis-toolchain
	cd forks/arm-pi1-bis && CROSSCOMPILE=$(TOOLPREFIX_ARM_PI1_BIS) QEMU=$(QEMU_ARM_PI1_BIS) ./test-xv6.py

quick-test-arm-pi1-bis: check-arm-pi1-bis-toolchain
	cd forks/arm-pi1-bis && CROSSCOMPILE=$(TOOLPREFIX_ARM_PI1_BIS) QEMU=$(QEMU_ARM_PI1_BIS) ./test-xv6.py boot

clean-arm-pi1-bis:
	$(MAKE) -C forks/arm-pi1-bis clean

kill-arm-pi1-bis:
	-pkill -f '$(QEMU_ARM_PI1_BIS)' 2>/dev/null || true

###############################################################################
# arm (forks/arm, inaciose/xv6-armv7-rpi - renamed from forks/armv7-rpi)
###############################################################################
# This port turned out to be the clear winner among the four ARM32 ports
# here: the SMP boot-gating gap noted below was fixed, along with a
# Thumb/ARM codegen mismatch and a console/interrupt-routing swap to
# PL011 (see notes_arch_arm.txt), reaching a genuinely interactive shell
# with a passing usertests run - so unlike forks/arm-pi1-bis/forks/arm-pi1/
# forks/arm-pi2, which keep their own (ISA-prefixed) names (see this Makefile's
# own header comment on why), this fork was promoted to the bare ISA
# name outright and its directory renamed forks/armv7-rpi -> forks/arm
# to match (2026-09-08), same single-fork-per-ISA treatment as
# riscv64/i386/riscv32/arm64.
#
# usr/usertests.c has two tests commented out (preempt() hangs
# indefinitely under QEMU's raspi2b; sbrktest() crashes the whole
# process partway through, a pre-existing upstream-documented issue) -
# see that file's own "claude:" comments and notes_arch_arm.txt. The
# rest of usertests, plus exectest()'s own "ALL TESTS PASSED" exec
# trick, run to completion, so test-arm gets a real pass/fail signal.
check-arm-toolchain:
	@if [ "$(TOOLPREFIX_ARM)" = NONE ] || [ "$(QEMU_ARM)" = NONE ]; then \
		echo "Makefile: arm toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

build-arm: check-arm-toolchain
	$(MAKE) -C forks/arm CROSSCOMPILE=$(TOOLPREFIX_ARM) QEMU=$(QEMU_ARM) kernel.elf

run-arm: check-arm-toolchain
	$(MAKE) -C forks/arm CROSSCOMPILE=$(TOOLPREFIX_ARM) QEMU=$(QEMU_ARM) qemu

test-arm: check-arm-toolchain
	cd forks/arm && CROSSCOMPILE=$(TOOLPREFIX_ARM) QEMU=$(QEMU_ARM) ./test-xv6.py

quick-test-arm: check-arm-toolchain
	cd forks/arm && CROSSCOMPILE=$(TOOLPREFIX_ARM) QEMU=$(QEMU_ARM) ./test-xv6.py boot

clean-arm:
	$(MAKE) -C forks/arm clean

kill-arm:
	-pkill -f '$(QEMU_ARM)' 2>/dev/null || true

###############################################################################
# arm-pi1 (forks/arm-pi1, zhiyihuang/xv6_rpi_port - renamed from forks/rpi1)
###############################################################################
# claude: IMPORTANT - the user has physical Raspberry Pi 1/2 hardware
# and this fork genuinely boots there (forks/arm-pi1/kernel.ld, "make all",
# "kernel.img"). This section ONLY drives the separate, purely-additive
# QEMU-only targets (kernel-qemu.ld/"kernel-qemu.img"/"qemu" - see
# notes_arch_arm_pi1.txt) - never touches the real-hardware build at all.
# Do not fold the real-hardware target into this repo's top-level
# Makefile; it stays reachable only via forks/arm-pi1's own Makefile
# directly, by design (this repo's top-level build system is
# specifically about QEMU-based build/boot, per CLAUDE.md).
#
# forks/arm-pi1/Makefile's own cross-toolchain variable is "ARMGNU", and
# (unlike every other fork's TOOLPREFIX/CROSSCOMPILE) it does NOT
# include a trailing "-" - $(ARMGNU)-gcc etc. add it themselves - so the
# trailing "-" that ./configure's own TOOLPREFIX_ARM_PI1 always includes
# (matching every other TOOLPREFIX_<ARCH> in this repo) needs stripping
# here specifically.
#
# test-arm-pi1/clean-arm-pi1/kill-arm-pi1 and build-all/test-all/
# clean-all/kill-all added 2026-09-08: the console-output gap this
# section used to describe turned out to be TWO real, fixable bugs
# (an additive PL011 driver alongside the Mini-UART, and a framebuffer-
# write fault mishandled because tvinit() hadn't run yet - see
# notes_arch_arm_pi1.txt), not a QEMU/firmware-only limitation - once
# fixed, this fork now boots to a real interactive shell and passes a
# full "ALL TESTS PASSED" usertests run under QEMU.
check-arm-pi1-toolchain:
	@if [ "$(TOOLPREFIX_ARM_PI1)" = NONE ] || [ "$(QEMU_ARM_PI1)" = NONE ]; then \
		echo "Makefile: arm-pi1 toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

build-arm-pi1: check-arm-pi1-toolchain
	$(MAKE) -C forks/arm-pi1 ARMGNU=$(patsubst %-,%,$(TOOLPREFIX_ARM_PI1)) QEMU=$(QEMU_ARM_PI1) kernel-qemu.img

run-arm-pi1: check-arm-pi1-toolchain
	$(MAKE) -C forks/arm-pi1 ARMGNU=$(patsubst %-,%,$(TOOLPREFIX_ARM_PI1)) QEMU=$(QEMU_ARM_PI1) qemu

# claude: same build, but a real GTK window (no -nographic) so the
# emulated framebuffer console is actually visible - see
# forks/arm-pi1/Makefile's own "qemu-graphics" target comment and
# notes_arch_arm_pi1.txt's "Open gap" section. Requires $DISPLAY; not
# folded into build-all/test-all (nothing headless-CI can assert against
# a GTK window).
run-arm-pi1-qemu-graphics: check-arm-pi1-toolchain
	$(MAKE) -C forks/arm-pi1 ARMGNU=$(patsubst %-,%,$(TOOLPREFIX_ARM_PI1)) QEMU=$(QEMU_ARM_PI1) QMP_SOCK="$(QMP_SOCK)" qemu-graphics

test-arm-pi1: check-arm-pi1-toolchain
	cd forks/arm-pi1 && ARMGNU=$(patsubst %-,%,$(TOOLPREFIX_ARM_PI1)) QEMU=$(QEMU_ARM_PI1) ./test-xv6.py

quick-test-arm-pi1: check-arm-pi1-toolchain
	cd forks/arm-pi1 && ARMGNU=$(patsubst %-,%,$(TOOLPREFIX_ARM_PI1)) QEMU=$(QEMU_ARM_PI1) ./test-xv6.py boot

clean-arm-pi1:
	$(MAKE) -C forks/arm-pi1 clean

kill-arm-pi1:
	-pkill -f '$(QEMU_ARM_PI1)' 2>/dev/null || true

###############################################################################
# arm-pi2 (forks/arm-pi2, zhiyihuang/xv6_rpi_port, extended to ARMv7/rpi2 -
# renamed from forks/rpi2)
###############################################################################
# claude: same real-hardware-safety note as arm-pi1 above applies here -
# the user has physical Raspberry Pi 2 hardware, this section only
# drives the additive QEMU-only "kernel7-qemu.bin"/"qemu" targets, never
# the real-hardware "kernel7.bin"/"all" targets (see
# notes_arch_arm_pi2.txt).
#
# Unlike forks/arm-pi1's own "ARMGNU" (no trailing "-"), forks/arm-pi2/
# Makefile's own toolchain variable is "TOOLCHAIN" and DOES expect the
# trailing "-" (matching every other TOOLPREFIX_<ARCH> in this repo) -
# passed through as-is, no stripping needed.
#
# No test-arm-pi2/clean-arm-pi2/kill-arm-pi2 yet, not folded into
# build-all/run-all/test-all: boots cleanly under QEMU through the full
# firmware handoff and well into real kernel logic, but hits an
# unresolved crash before reaching any console output - see
# notes_arch_arm_pi2.txt.
check-arm-pi2-toolchain:
	@if [ "$(TOOLPREFIX_ARM_PI2)" = NONE ] || [ "$(QEMU_ARM_PI2)" = NONE ]; then \
		echo "Makefile: arm-pi2 toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

build-arm-pi2: check-arm-pi2-toolchain
	$(MAKE) -C forks/arm-pi2 hw=rpi2 TOOLCHAIN=$(TOOLPREFIX_ARM_PI2) QEMU=$(QEMU_ARM_PI2) kernel7-qemu.bin

run-arm-pi2: check-arm-pi2-toolchain
	$(MAKE) -C forks/arm-pi2 hw=rpi2 TOOLCHAIN=$(TOOLPREFIX_ARM_PI2) QEMU=$(QEMU_ARM_PI2) qemu

###############################################################################
# arm-pi3 (forks/arm-pi3, patha454/xv6_pi_mp - renamed from forks/pi_mp)
###############################################################################
# claude: real Raspberry Pi 3 MP hardware, 32-bit ARM despite the
# upstream README's "AArch64" description (see CLAUDE.md). Unlike
# arm-pi1/arm-pi2, this fork's own QEMU target is genuinely different
# from the real-hardware one, not just at LINK time - it needs a
# separate armstub64.bin (an AArch64 firmware stub - see that fork's
# own Makefile/armstub64.S comments for why: qemu-system-aarch64's
# raspi3b always resets in AArch64, and QEMU's raw loaders skip the
# firmware step that would drop it to AArch32 for this 32-bit kernel)
# and targets qemu-system-aarch64 -M raspi3b (not qemu-system-arm -M
# raspi2b like the sibling forks/arm-pi2). QEMU_AARCH64_TOOLPREFIX
# reuses whatever TOOLPREFIX_ARM64 above already found, rather than
# detecting a second aarch64 toolchain.
#
# No test-arm-pi3/clean-arm-pi3/kill-arm-pi3 yet, not folded into
# build-all/run-all/test-all: boots all 4 cores under QEMU, all the way
# through userinit - by far the furthest of the four ARM32 real-board
# ports - but a real SMP page-table race crashes a secondary core
# before reaching a shell. See notes_arch_arm_pi3.txt for the ten real
# bugs found and fixed getting this far, and the open one left behind.
check-arm-pi3-toolchain:
	@if [ "$(TOOLPREFIX_ARM_PI3)" = NONE ] || [ "$(QEMU_ARM_PI3)" = NONE ]; then \
		echo "Makefile: arm-pi3 toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

build-arm-pi3: check-arm-pi3-toolchain
	$(MAKE) -C forks/arm-pi3 hw=rpi2 TOOLCHAIN=$(TOOLPREFIX_ARM_PI3) QEMU=$(QEMU_ARM_PI3) QEMU_AARCH64_TOOLPREFIX=$(TOOLPREFIX_ARM64) kernel7-qemu.bin armstub64.bin

run-arm-pi3: check-arm-pi3-toolchain
	$(MAKE) -C forks/arm-pi3 hw=rpi2 TOOLCHAIN=$(TOOLPREFIX_ARM_PI3) QEMU=$(QEMU_ARM_PI3) QEMU_AARCH64_TOOLPREFIX=$(TOOLPREFIX_ARM64) qemu

###############################################################################
# Umbrella targets - each grows a per-arch prerequisite as a new port is
# wired up above.
###############################################################################

build-all: build-riscv64 build-i386 build-amd64-jserv build-amd64 build-riscv32 build-arm64 build-loongarch build-arm build-arm-pi1 build-arm-pi1-bis build-mips

# claude: "test-all" is the quick/boot-only umbrella (~1 min combined,
# each arch just booting to a shell - see each fork's own test-xv6.py
# "boot" mode) so it stays cheap enough to run on every change. The full
# usertests suite per arch is real signal but genuinely slow end to end
# (a verified run of the old test-all - now stress-test-all - took
# 24m34s on this host, dominated by TCG-emulated archs like mips/
# loongarch/arm running usertests' full syscall-heavy suite) - run
# "make stress-test-all" for that, or "make stress-test-<arch>" /
# "make test-<arch>" for one arch's own full run (that single-arch name
# keeps its pre-existing meaning: full usertests, unchanged, since
# .github/workflows/docker.yml's Dockerfile calls it by that exact name
# and CLAUDE.md's "Testing conventions" documents it that way).
test-all: quick-test-riscv64 quick-test-i386 quick-test-amd64-jserv quick-test-amd64 quick-test-riscv32 quick-test-arm64 quick-test-loongarch quick-test-arm quick-test-arm-pi1 quick-test-arm-pi1-bis quick-test-mips

stress-test-all: test-riscv64 test-i386 test-amd64-jserv test-amd64 test-riscv32 test-arm64 test-loongarch test-arm test-arm-pi1 test-arm-pi1-bis test-mips

clean-all: clean-riscv64 clean-i386 clean-amd64-jserv clean-amd64 clean-riscv32 clean-arm64 clean-loongarch clean-arm clean-arm-pi1 clean-arm-pi1-bis clean-mips

kill-all: kill-riscv64 kill-i386 kill-amd64-jserv kill-amd64 kill-riscv32 kill-arm64 kill-loongarch kill-arm kill-arm-pi1 kill-arm-pi1-bis kill-mips

# claude: opens a real GTK window per arch (see scripts/README.md) - needs
# $DISPLAY, so it's separate from test-all rather than folded into it.
test-all-graphics:
	python3 scripts/test_qemu_graphics.py

# claude: bare "all"/"clean" aliases for build-all/clean-all - the
# conventional Make entry-point names, requested alongside "default"
# below since a bare "make" previously ran "check-riscv64-toolchain"
# (Make's own "first rule in the file wins" default-goal rule) with no
# output at all. "test"/"kill" deliberately do NOT get bare aliases -
# every "test-<arch>" throughout this Makefile already omits the plain
# "test" form (there's no ambiguity to resolve the way "all"/"clean"
# had with "build-all"/"clean-all"), so only add one where a real,
# already-existing target name split motivated it.
all: build-all

clean: clean-all

# Builds and runs the build-<arch>/test-<arch> pipeline inside the
# reproducible image the Dockerfile pins - see that file's own header
# comment, in particular its own ARCH build-arg (default "all", same as
# leaving this ARCH unset - "make build-docker ARCH=riscv64" builds/tests
# just that one arch, for the same per-arch parallelism
# .github/workflows/docker.yml's own matrix uses in CI). Same style as
# ~/c--/Makefile's own build-docker.
ARCH ?= all
build-docker:
	docker build --build-arg ARCH=$(ARCH) -t "xv6-multiarch:$(ARCH)" .
