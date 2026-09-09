#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled on
# forks/amd64/test-xv6.py's own shape: this fork has no crash/log/orphan-
# recovery test programs either (see this directory's own Makefile
# UPROGS list - no grind/sync/logstress/forphan/dorphan), so a usertests
# run is the whole test.
#
# claude: as of docs/claude_notes/notes_arch_riscv32.txt's "bug 4", this
# currently does NOT pass - the freshly-exec'd sh crashes on its own
# first getcmd() call before ever printing a real prompt response, so
# the "usertests\n" command below is never actually processed. Written
# and wired up anyway, per this repo's own "Adding a new arch" recipe,
# so it starts passing automatically once bug 4 is fixed - no need to
# remember to come back and add it later.
#
# Unlike forks/x86/forks/amd64-jserv (whole-disk xv6.img) or forks/amd64
# (multiboot -kernel with no disk image at all), this fork's own "qemu"
# target already boots via "-kernel kernel/kernel" plus a separate
# virtio-mmio fs.img drive (see kernel/kernel.ld's own comment: entry.S
# is loaded straight at 0x80000000, where qemu's -kernel jumps for
# riscv - no bootblock stage at all) - QEMUOPTS already includes
# "-nographic" itself, so "make qemu" (not "qemu-nox" - this fork has no
# such target) is what this script drives over a pipe.
#
# The actual QEMU-driving/regex-matching machinery lives in
# scripts/qemu_console.py, shared with every other pared-down fork here -
# see that file's own header comment for why.
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "scripts"))
from qemu_console import QEMU, main

# Same full (non "-q") budget as every other fork's own test-xv6.py.
TIMEOUT = 600


def make_qemu(reset=True):
    return QEMU(
        reset_cmds=[
            ["make", "kernel/kernel"],
            ["rm", "-f", "fs.img"],
            ["make", "fs.img"],
        ],
        qemu_argv=["make", "qemu"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, timeout=TIMEOUT)
