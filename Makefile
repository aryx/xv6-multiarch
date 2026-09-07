# Top-level Makefile for xv6-multiarch.
#
# Each forks/<name>/ is a self-contained xv6 port with its own Makefile
# (see docs/PROVENANCE.md and the root README for what each one is). This
# file does not replace those - it drives them with the toolchain and QEMU
# binary ./configure detected for this host (Makefile.config, generated -
# do not edit by hand, re-run ./configure instead), and gives every port
# short "build-<arch>"/"run-<arch>" targets.
#
# Target names use the bare ISA (riscv64, i386, ...), matching ./configure's
# own TOOLPREFIX_<ARCH>/QEMU_<ARCH> naming - NOT the forks/<name> directory
# name, which instead names the upstream repo/port (forks/riscv is the
# riscv64 port; forks/x86 is the i386 one). See ./configure's own header
# comment for why the directories themselves aren't renamed to match.
#
# Currently wired for riscv64, i386, x86_64, amd64, riscv32, arm64, mips,
# and loongarch - see docs/claude_notes/build-and-test-plan.md, Phases 1-2,
# and Phase 4 (see docs/claude_notes/notes_arch_x86_64.txt,
# notes_arch_amd64.txt, notes_arch_riscv32.txt, notes_arch_arm64.txt -
# riscv32 and arm64 both boot to a shell but their own test-<arch> does
# not pass yet, see those two files' own open bugs; notes_arch_mips.txt -
# boots silently, no test-mips at all yet, and only build-mips/run-mips
# are wired up, not the full set; notes_arch_loongarch.txt - fully
# working, the only arch besides riscv64/i386/x86_64/amd64 whose
# test-<arch> actually passes).
#
# arm64, not aarch64: forks/aarch64 is still named for its upstream repo
# (k-mrm/xv6-aarch64), but the Makefile target/./configure variable name
# follows ~/c--'s and ~/goken's own CCARM64/RUN_ARM64/arch/arm64/
# convention for this ISA - same bare-ISA-name-not-directory-name
# reasoning as riscv64/i386/riscv32 above, just a different bare name
# than the directory happens to use.

-include Makefile.config

TOOLPREFIX_RISCV64 ?=
QEMU_RISCV64 ?= qemu-system-riscv64
TOOLPREFIX_I386 ?=
QEMU_I386 ?= qemu-system-i386
TOOLPREFIX_X86_64 ?=
QEMU_X86_64 ?= qemu-system-x86_64
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

.PHONY: build-riscv64 run-riscv64 test-riscv64 clean-riscv64 kill-riscv64 check-riscv64-toolchain \
        build-i386 run-i386 test-i386 clean-i386 kill-i386 check-i386-toolchain \
        build-x86_64 run-x86_64 test-x86_64 clean-x86_64 kill-x86_64 check-x86_64-toolchain \
        build-amd64 run-amd64 test-amd64 clean-amd64 kill-amd64 check-amd64-toolchain \
        build-riscv32 run-riscv32 test-riscv32 clean-riscv32 kill-riscv32 check-riscv32-toolchain \
        build-arm64 run-arm64 test-arm64 clean-arm64 kill-arm64 check-arm64-toolchain \
        build-mips run-mips check-mips-toolchain \
        build-loongarch run-loongarch test-loongarch clean-loongarch kill-loongarch check-loongarch-toolchain \
        build-all test-all clean-all kill-all build-docker

###############################################################################
# riscv64 (forks/riscv, RV64GC - MIT's current xv6-riscv)
###############################################################################

# NONE is ./configure's own sentinel for "looked, found nothing" (see its
# TOOLPREFIX_RISCV64=NONE/QEMU_RISCV64=NONE assignments) - caught here so a
# missing toolchain fails with a clear pointer to ./configure instead of
# forks/riscv/Makefile trying to run "NONEgcc" or a qemu binary that isn't
# on PATH.
check-riscv64-toolchain:
	@if [ "$(TOOLPREFIX_RISCV64)" = NONE ] || [ "$(QEMU_RISCV64)" = NONE ]; then \
		echo "Makefile: riscv64 toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

build-riscv64: check-riscv64-toolchain
	$(MAKE) -C forks/riscv TOOLPREFIX=$(TOOLPREFIX_RISCV64) QEMU=$(QEMU_RISCV64) kernel/kernel fs.img

run-riscv64: check-riscv64-toolchain
	$(MAKE) -C forks/riscv TOOLPREFIX=$(TOOLPREFIX_RISCV64) QEMU=$(QEMU_RISCV64) qemu

# forks/riscv/test-xv6.py drives plain "make qemu" itself (see that
# script's own QEMU class), so there's no command line to pass TOOLPREFIX/
# QEMU on - only the environment. TOOLPREFIX threads through correctly
# that way (its ifndef guard honors a value from any origin, environment
# included); QEMU does NOT (forks/riscv/Makefile's own "QEMU =
# qemu-system-riscv64" is an unconditional assignment with no ifndef
# guard, so a plain makefile assignment always wins over the environment)
# - harmless in practice since QEMU_RISCV64 is just the bare command name
# qemu-system-riscv64 once found on PATH, identical to that hardcoded
# default.
test-riscv64: check-riscv64-toolchain build-riscv64
	cd forks/riscv && TOOLPREFIX=$(TOOLPREFIX_RISCV64) ./test-xv6.py usertests

clean-riscv64:
	$(MAKE) -C forks/riscv clean

# "run-riscv64"/"test-riscv64" run QEMU attached to this shell (-nographic),
# but a stuck boot (or a Ctrl-C that missed) can leave qemu-system-riscv64
# running headless in the background - this kills it by matching the exact
# QEMU_RISCV64 command ./configure detected, not just the bare binary name,
# so it doesn't reach for some other arch's qemu-system-* by accident.
kill-riscv64:
	-pkill -f '$(QEMU_RISCV64)' 2>/dev/null || true

###############################################################################
# i386 (forks/x86, MIT xv6-public - the original teaching OS)
###############################################################################

check-i386-toolchain:
	@if [ "$(TOOLPREFIX_I386)" = NONE ] || [ "$(QEMU_I386)" = NONE ]; then \
		echo "Makefile: i386 toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

build-i386: check-i386-toolchain
	$(MAKE) -C forks/x86 TOOLPREFIX=$(TOOLPREFIX_I386) QEMU=$(QEMU_I386) kernel fs.img xv6.img

run-i386: check-i386-toolchain
	$(MAKE) -C forks/x86 TOOLPREFIX=$(TOOLPREFIX_I386) QEMU=$(QEMU_I386) qemu-nox

# forks/x86/test-xv6.py (this repo's own, not upstream - see its own header
# comment) drives plain "make qemu-nox" the same way forks/riscv's does -
# same TOOLPREFIX-via-environment/QEMU-via-PATH split applies (forks/x86/
# Makefile's own "QEMU = ..." auto-detect has no ifndef guard either).
test-i386: check-i386-toolchain build-i386
	cd forks/x86 && TOOLPREFIX=$(TOOLPREFIX_I386) ./test-xv6.py

clean-i386:
	$(MAKE) -C forks/x86 clean

kill-i386:
	-pkill -f '$(QEMU_I386)' 2>/dev/null || true

###############################################################################
# x86_64 (forks/x86_64, jserv/xv6-x86_64)
###############################################################################

check-x86_64-toolchain:
	@if [ "$(TOOLPREFIX_X86_64)" = NONE ] || [ "$(QEMU_X86_64)" = NONE ]; then \
		echo "Makefile: x86_64 toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

# forks/x86_64/Makefile has no TOOLPREFIX at all - it's CROSS_COMPILE
# there (see ./configure's own x86_64 section for why this repo still
# calls the detected value TOOLPREFIX_X86_64 for naming consistency with
# every other arch, just passed under the name this fork's Makefile
# actually expects).
#
# CPUS=2: forks/x86_64/Makefile's own "ifndef CPUS" default is this
# HOST's own core count (`grep -c ^processor /proc/cpuinfo`), unlike
# every other fork here (forks/riscv hardcodes 3, forks/x86 hardcodes 2)
# - harmless on a native x86_64 deployment, but on this 64-core dev
# machine it means qemu-nox boots 64 vCPUs. xv6 itself only ever starts
# min(that count, NCPU=8) of them, so the extra vCPUs do nothing useful,
# but QEMU still pays real per-vCPU TCG overhead for all 64 - confirmed
# this alone was enough to blow past test-xv6.py's 60s boot-detection
# window (a real full boot to a shell prompt, verified separately with
# -smp 2, took under 30s). Not a logic bug, just a bad default to
# inherit unmodified into an automated/CI context - pass a small,
# explicit CPUS here rather than edit that default in the fork itself.
build-x86_64: check-x86_64-toolchain
	$(MAKE) -C forks/x86_64 CROSS_COMPILE=$(TOOLPREFIX_X86_64) QEMU=$(QEMU_X86_64) out/kernel.elf fs.img xv6.img

run-x86_64: check-x86_64-toolchain
	$(MAKE) -C forks/x86_64 CROSS_COMPILE=$(TOOLPREFIX_X86_64) QEMU=$(QEMU_X86_64) CPUS=2 qemu-nox

# Unlike forks/riscv's/forks/x86's own QEMU vars, forks/x86_64/Makefile's
# is "QEMU ?= qemu-system-x86_64" (a conditional default) - so QEMU
# genuinely threads through the environment correctly here, not just
# CROSS_COMPILE. CPUS is also "ifndef"-guarded (like TOOLPREFIX
# elsewhere), so it too threads through the environment correctly into
# test-xv6.py's own "make qemu-nox" subprocess call.
test-x86_64: check-x86_64-toolchain build-x86_64
	cd forks/x86_64 && CROSS_COMPILE=$(TOOLPREFIX_X86_64) QEMU=$(QEMU_X86_64) CPUS=2 ./test-xv6.py

clean-x86_64:
	$(MAKE) -C forks/x86_64 clean

kill-x86_64:
	-pkill -f '$(QEMU_X86_64)' 2>/dev/null || true

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

# Same TOOLPREFIX-via-environment/QEMU-via-command-line-only split as
# forks/riscv's/forks/x86's own test-<arch> targets (forks/amd64/
# Makefile's own "QEMU = qemu-system-x86_64" has no ifndef guard either).
test-amd64: check-amd64-toolchain build-amd64
	cd forks/amd64 && TOOLPREFIX=$(TOOLPREFIX_AMD64) ./test-xv6.py

clean-amd64:
	$(MAKE) -C forks/amd64 clean

kill-amd64:
	-pkill -f '$(QEMU_AMD64)' 2>/dev/null || true

###############################################################################
# riscv32 (forks/rv32, michaelengel/xv6-rv32)
###############################################################################

check-riscv32-toolchain:
	@if [ "$(TOOLPREFIX_RISCV32)" = NONE ] || [ "$(QEMU_RISCV32)" = NONE ]; then \
		echo "Makefile: riscv32 toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

# Same "kernel/kernel"+"fs.img", no separate whole-disk image, shape as
# riscv64 - forks/rv32 boots via QEMU's own "-kernel" loading straight to
# 0x80000000, no bootblock stage (see docs/claude_notes/notes_arch_riscv32.txt).
#
# NOTE: this port is NOT fully working yet - it boots to an interactive
# shell prompt, but the shell itself then crashes (see
# notes_arch_riscv32.txt's "bug 4", not yet fixed) - test-riscv32 below will
# fail until that's resolved. build-riscv32/run-riscv32 do work.
build-riscv32: check-riscv32-toolchain
	$(MAKE) -C forks/rv32 TOOLPREFIX=$(TOOLPREFIX_RISCV32) QEMU=$(QEMU_RISCV32) kernel/kernel fs.img

run-riscv32: check-riscv32-toolchain
	$(MAKE) -C forks/rv32 TOOLPREFIX=$(TOOLPREFIX_RISCV32) QEMU=$(QEMU_RISCV32) qemu

# Same TOOLPREFIX-via-environment/QEMU-via-command-line-only split as
# forks/riscv's/forks/x86's own test-<arch> targets (forks/rv32/Makefile's
# own "QEMU = qemu-system-riscv32 -monitor ..." has no ifndef guard either).
test-riscv32: check-riscv32-toolchain build-riscv32
	cd forks/rv32 && TOOLPREFIX=$(TOOLPREFIX_RISCV32) ./test-xv6.py

clean-riscv32:
	$(MAKE) -C forks/rv32 clean

kill-riscv32:
	-pkill -f '$(QEMU_RISCV32)' 2>/dev/null || true

###############################################################################
# arm64 (forks/aarch64, k-mrm/xv6-aarch64)
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
	$(MAKE) -C forks/aarch64 TOOLPREFIX=$(TOOLPREFIX_ARM64) QEMU=$(QEMU_ARM64) kernel/kernel fs.img

run-arm64: check-arm64-toolchain
	$(MAKE) -C forks/aarch64 TOOLPREFIX=$(TOOLPREFIX_ARM64) QEMU=$(QEMU_ARM64) qemu

# Same TOOLPREFIX-via-environment/QEMU-via-command-line-only split as
# forks/riscv's/forks/x86's own test-<arch> targets (forks/aarch64/
# Makefile's own "QEMU = $(QEMUPREFIX)qemu-system-aarch64" has no ifndef
# guard either).
test-arm64: check-arm64-toolchain build-arm64
	cd forks/aarch64 && TOOLPREFIX=$(TOOLPREFIX_ARM64) ./test-xv6.py

clean-arm64:
	$(MAKE) -C forks/aarch64 clean

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
# No test-mips/clean-mips/kill-mips yet, and not folded into build-all/
# run-all: this port boots (confirmed via gdb + a QEMU trace) but
# produces no visible console output at all yet (notes_arch_mips.txt's
# own "bug 5", not yet root-caused) - there is currently no way to look
# at a "make run-mips" session and tell whether it's doing the right
# thing, so a scripted test-mips would just hang against a real
# silence, not a useful pass/fail signal.
build-mips: check-mips-toolchain
	$(MAKE) -C forks/mips TOOLPREFIX=$(TOOLPREFIX_MIPS) QEMU=$(QEMU_MIPS) kernelmemfs

run-mips: check-mips-toolchain
	$(MAKE) -C forks/mips TOOLPREFIX=$(TOOLPREFIX_MIPS) QEMU=$(QEMU_MIPS) qemu-nox-memfs

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

clean-loongarch:
	$(MAKE) -C forks/loongarch clean

kill-loongarch:
	-pkill -f '$(QEMU_LOONGARCH)' 2>/dev/null || true

###############################################################################
# Umbrella targets - each grows a per-arch prerequisite as a new port is
# wired up above.
###############################################################################

build-all: build-riscv64 build-i386 build-x86_64 build-amd64 build-riscv32 build-arm64 build-loongarch

test-all: test-riscv64 test-i386 test-x86_64 test-amd64 test-riscv32 test-arm64 test-loongarch

clean-all: clean-riscv64 clean-i386 clean-x86_64 clean-amd64 clean-riscv32 clean-arm64 clean-loongarch

kill-all: kill-riscv64 kill-i386 kill-x86_64 kill-amd64 kill-riscv32 kill-arm64 kill-loongarch

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
