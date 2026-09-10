#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled on
# forks/riscv32/test-xv6.py's own shape (same "make qemu" boot mechanism,
# same kernel/user-split lineage). This fork's own UPROGS list adds
# `grind` (a stress test) beyond what rv32 has, but usertests.c's own
# suite is still the whole test here - grind is not separately driven.
#
# claude: as of docs/claude_notes/notes_arch_arm64.txt's "bug 1", this
# currently does NOT pass - the freshly-exec'd sh crashes shortly after
# printing its prompt, so the "usertests\n" command below is never
# actually processed. Written and wired up anyway, per this repo's own
# "Adding a new arch" recipe, so it starts passing automatically once
# bug 1 is fixed.
#
# The actual QEMU-driving/regex-matching machinery lives in
# scripts/qemu_console.py, shared with every other pared-down fork here -
# see that file's own header comment for why.
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "scripts"))
from qemu_console import QEMU, main

# Same full (non "-q") budget as every other fork's own test-xv6.py.
TIMEOUT = 300


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
