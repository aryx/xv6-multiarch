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
# Currently wired for riscv64, i386, x86_64, and amd64 - see
# docs/claude_notes/build-and-test-plan.md, Phases 1-2, and Phase 4
# (see docs/claude_notes/notes_arch_x86_64.txt, notes_arch_amd64.txt).

-include Makefile.config

TOOLPREFIX_RISCV64 ?=
QEMU_RISCV64 ?= qemu-system-riscv64
TOOLPREFIX_I386 ?=
QEMU_I386 ?= qemu-system-i386
TOOLPREFIX_X86_64 ?=
QEMU_X86_64 ?= qemu-system-x86_64
TOOLPREFIX_AMD64 ?=
QEMU_AMD64 ?= qemu-system-x86_64

.PHONY: build-riscv64 run-riscv64 test-riscv64 clean-riscv64 kill-riscv64 check-riscv64-toolchain \
        build-i386 run-i386 test-i386 clean-i386 kill-i386 check-i386-toolchain \
        build-x86_64 run-x86_64 test-x86_64 clean-x86_64 kill-x86_64 check-x86_64-toolchain \
        build-amd64 run-amd64 test-amd64 clean-amd64 kill-amd64 check-amd64-toolchain \
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
# Umbrella targets - each grows a per-arch prerequisite as a new port is
# wired up above.
###############################################################################

build-all: build-riscv64 build-i386 build-x86_64 build-amd64

test-all: test-riscv64 test-i386 test-x86_64 test-amd64

clean-all: clean-riscv64 clean-i386 clean-x86_64 clean-amd64

kill-all: kill-riscv64 kill-i386 kill-x86_64 kill-amd64

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
