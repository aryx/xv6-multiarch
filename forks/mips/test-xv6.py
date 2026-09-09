#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled on
# forks/riscv32/test-xv6.py's own shape. This fork has no crash/log/
# orphan-recovery test programs either, so a usertests run is the whole
# test.
#
# claude: usr/usertests.c has several sub-tests commented out (see that
# file's own "claude:" comments right above each call site, and
# docs/claude_notes/notes_arch_mips.txt's own "Gap" section) - sbrktest,
# validatetest, mem, preempt, exitwait, and forktest all hang for real
# under this port's own QEMU/malta emulation (confirmed via gdb, not
# just slow - none of them are simple to root-cause: they span at least
# two different subsystems, an allocuvm()/kalloc() stall under real
# memory pressure and a separate fork()/copyuvm()/walkpgdir() stall).
# Skipped pragmatically so the rest of the suite can reach exectest()'s
# "ALL TESTS PASSED" signal (this file's own echoargv trick, not a raw
# printf - see CLAUDE.md's own testing-conventions note on this).
#
# This fork's own "qemu-nox-memfs" Makefile target already includes
# "-nographic" and boots the memfs kernel variant (embeds fs.img
# directly in the kernel image - the disk-based "kernel"/"qemu-nox"
# path was never finished by the port's own author, see
# notes_arch_mips.txt) - "make qemu-nox-memfs" is what this script
# drives over a pipe.
#
# claude: TOOLPREFIX/QEMU here are plain "=" in this fork's own
# Makefile (not "?="), so an inherited environment variable of the same
# name is silently overridden by the file's own default
# ("mipsel-sde-elf-", a toolchain that doesn't exist on this host) -
# same class of gap as forks/arm's own test-xv6.py hit (forks/armv7-rpi
# at the time). Passed explicitly on make's command line instead, from
# the TOOLPREFIX/QEMU environment variables the top-level Makefile's own
# test-mips target sets before invoking this script.
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
    f"TOOLPREFIX={os.environ.get('TOOLPREFIX', '')}",
    f"QEMU={os.environ.get('QEMU', 'qemu-system-mipsel')}",
]


def make_qemu(reset=True):
    return QEMU(
        reset_cmds=[
            # claude: NO "make clean" here - it used to be the first reset
            # command, and made "make test-all" recompile this port from
            # scratch even straight after a full "make build-all". Unlike
            # the Pi ports it was not even compensating for anything:
            # this Makefile already both generated (-MD) and read
            # (-include) its header dependencies, so an incremental build
            # was already trustworthy and the clean was pure waste.
            # Nothing else needs resetting either - this port boots with
            # "-kernel" and its filesystem is embedded in the kernel
            # image, so guest writes land in RAM and die with QEMU.
            # See docs/claude_notes/notes_build_system.txt.
            ["make", *MAKEVARS, "kernelmemfs"],
        ],
        qemu_argv=["make", *MAKEVARS, "qemu-nox-memfs"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, timeout=TIMEOUT)
