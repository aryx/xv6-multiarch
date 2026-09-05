# Top-level Makefile for xv6-multiarch.
#
# Each forks/<name>/ is a self-contained xv6 port with its own Makefile
# (see docs/PROVENANCE.md and the root README for what each one is). This
# file does not replace those - it drives them with the toolchain and QEMU
# binary ./configure detected for this host (Makefile.config, generated -
# do not edit by hand, re-run ./configure instead), and gives every port
# short "build-<arch>"/"run-<arch>" targets.
#
# Currently wired for riscv only (forks/riscv) - see
# docs/claude_notes/build-and-test-plan.md, Phase 1: get one arch building
# and booting before replicating this shape to the other twelve ports.

-include Makefile.config

TOOLPREFIX_RISCV ?=
QEMU_RISCV ?= qemu-system-riscv64

.PHONY: build-riscv run-riscv test-riscv clean-riscv check-riscv-toolchain clean kill-riscv kill

# NONE is ./configure's own sentinel for "looked, found nothing" (see its
# TOOLPREFIX_RISCV=NONE/QEMU_RISCV=NONE assignments) - caught here so a
# missing toolchain fails with a clear pointer to ./configure instead of
# forks/riscv/Makefile trying to run "NONEgcc" or a qemu binary that isn't
# on PATH.
check-riscv-toolchain:
	@if [ "$(TOOLPREFIX_RISCV)" = NONE ] || [ "$(QEMU_RISCV)" = NONE ]; then \
		echo "Makefile: riscv toolchain/qemu not found - run ./configure to see what's missing" >&2; \
		exit 1; \
	fi

build-riscv: check-riscv-toolchain
	$(MAKE) -C forks/riscv TOOLPREFIX=$(TOOLPREFIX_RISCV) QEMU=$(QEMU_RISCV) kernel/kernel fs.img

run-riscv: check-riscv-toolchain
	$(MAKE) -C forks/riscv TOOLPREFIX=$(TOOLPREFIX_RISCV) QEMU=$(QEMU_RISCV) qemu

# forks/riscv/test-xv6.py drives plain "make qemu" itself (see that
# script's own QEMU class), so there's no command line to pass TOOLPREFIX/
# QEMU on - only the environment. TOOLPREFIX threads through correctly
# that way (its ifndef guard honors a value from any origin, environment
# included); QEMU does NOT (forks/riscv/Makefile's own "QEMU =
# qemu-system-riscv64" is an unconditional assignment with no ifndef
# guard, so a plain makefile assignment always wins over the environment)
# - harmless in practice since QEMU_RISCV is just the bare command name
# qemu-system-riscv64 once found on PATH, identical to that hardcoded
# default.
test-riscv: check-riscv-toolchain build-riscv
	cd forks/riscv && TOOLPREFIX=$(TOOLPREFIX_RISCV) ./test-xv6.py usertests

clean-riscv:
	$(MAKE) -C forks/riscv clean

# Umbrella target - just clean-riscv for now, grows a clean-<arch>
# prerequisite per port as each one gets wired up above.
clean: clean-riscv

# "run-riscv"/"test-riscv" run QEMU attached to this shell (-nographic),
# but a stuck boot (or a Ctrl-C that missed) can leave qemu-system-riscv64
# running headless in the background - this kills it by matching the exact
# QEMU_RISCV command ./configure detected, not just the bare binary name,
# so it doesn't reach for some other arch's qemu-system-* by accident.
kill-riscv:
	-pkill -f '$(QEMU_RISCV)' 2>/dev/null || true

# Umbrella target - just kill-riscv for now, same growth path as "clean".
kill: kill-riscv
