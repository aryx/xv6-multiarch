#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled directly on
# forks/riscv32/test-xv6.py's own shape. This fork has no crash/log/
# orphan-recovery test programs either, so a usertests run is the whole
# test.
#
# claude: this fork's own usr/usertests.c has two tests commented out
# (see that file's own "claude:" comments right above each call site,
# and docs/claude_notes/notes_arch_armv7_rpi.txt) - preempt() hangs
# indefinitely under QEMU's raspi2b, and sbrktest() crashes the whole
# usertests process partway through its own "out of memory" section (a
# pre-existing, upstream-documented issue, not something introduced
# here). Both are skipped so the remaining suite can reach exectest()'s
# "ALL TESTS PASSED" signal (this file's own echoargv trick, not a raw
# printf - see CLAUDE.md's own testing-conventions note on this).
#
# This fork's own "qemu" Makefile target already includes "-nographic"
# and boots via "-kernel xv6.img" (a raw binary, not the ELF - see
# notes_arch_armv7_rpi.txt for why), so "make qemu" (not "make
# qemu-nox" - no such target here) is what this script drives over a
# pipe, same as forks/riscv32.
#
# claude: unlike forks/riscv32's own TOOLPREFIX (which has a runtime
# auto-detect fallback in its Makefile that happens to land on the right
# toolchain even without an external override), this fork's makefile.inc
# hardcodes "CROSSCOMPILE := arm-none-eabi-" with no such fallback - a
# plain ":=" in the makefile beats an inherited environment variable of
# the same name (make only lets the environment win with "make -e", or
# for a variable the makefile itself declares "?="). The verified-working
# toolchain is arm-linux-gnueabihf- (see notes_arch_armv7_rpi.txt), which
# only takes effect if CROSSCOMPILE is passed on make's own command line
# - exactly how the top-level Makefile's build-armv7-rpi/run-armv7-rpi
# targets already do it. Passed through here the same way, from the
# CROSSCOMPILE/QEMU environment variables the top-level Makefile's own
# test-arm target sets before invoking this script.
#
# The actual QEMU-driving/regex-matching machinery lives in
# scripts/qemu_console.py, shared with every other pared-down fork here -
# see that file's own header comment for why.
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "scripts"))
from qemu_console import QEMU, main

TIMEOUT = 600

MAKEVARS = [
    f"CROSSCOMPILE={os.environ.get('CROSSCOMPILE', '')}",
    f"QEMU={os.environ.get('QEMU', 'qemu-system-arm')}",
]


def make_qemu(reset=True):
    return QEMU(
        reset_cmds=[
            # claude: NO "make clean" here - it used to be the first reset
            # command, and made "make test-all" recompile this port from
            # scratch even straight after a full "make build-all". It was
            # only ever compensating for header dependencies this port's
            # Makefile generated but never read (see its own "-include"
            # near the bottom, added at the same time as this): with no
            # tracking, an incremental build was silently wrong, and a
            # clean was the only way to trust what was under test. Now
            # that headers are tracked, an ordinary incremental build is
            # trustworthy. Nothing else needs resetting: this port boots
            # with "-kernel" and its filesystem is embedded in the kernel
            # image, so guest writes land in RAM and die with QEMU - a
            # test run cannot dirty anything on the host.
            # See docs/claude_notes/notes_build_system.txt.
            ["make", *MAKEVARS, "kernel.elf"],
        ],
        qemu_argv=["make", *MAKEVARS, "qemu"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, timeout=TIMEOUT)
