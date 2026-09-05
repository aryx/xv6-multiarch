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
# Currently wired for riscv64 and i386 - see
# docs/claude_notes/build-and-test-plan.md, Phases 1-2: one arch from each
# of the two layout families before replicating this shape to the other
# eleven ports.

-include Makefile.config

TOOLPREFIX_RISCV64 ?=
QEMU_RISCV64 ?= qemu-system-riscv64
TOOLPREFIX_I386 ?=
QEMU_I386 ?= qemu-system-i386

.PHONY: build-riscv64 run-riscv64 test-riscv64 clean-riscv64 kill-riscv64 check-riscv64-toolchain \
        build-i386 run-i386 test-i386 clean-i386 kill-i386 check-i386-toolchain \
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
# Umbrella targets - each grows a per-arch prerequisite as a new port is
# wired up above.
###############################################################################

build-all: build-riscv64 build-i386

test-all: test-riscv64 test-i386

clean-all: clean-riscv64 clean-i386

kill-all: kill-riscv64 kill-i386

# Builds and runs the full build-all/test-all pipeline inside the
# reproducible image the Dockerfile pins - see that file's own header
# comment. Same style as ~/c--/Makefile's own build-docker.
build-docker:
	docker build -t "xv6-multiarch" .
