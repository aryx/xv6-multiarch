#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled on
# forks/x86/test-xv6.py's own shape (itself modeled on forks/riscv/
# test-xv6.py's QEMU class): this fork has no crash/log/orphan-recovery
# test programs either (see this directory's own Makefile UPROGS list -
# no grind/sync/logstress/forphan/dorphan), so a usertests run is the
# whole test.
#
# Success detection reuses the same "ALL TESTS PASSED" string forks/x86's
# own usertests.c produces - via exec("echo", echoargv) as usertests.c's
# very last action, not a direct printf (see forks/x86/test-xv6.py's own
# header comment for the mechanism).
#
# The actual QEMU-driving/regex-matching machinery lives in
# scripts/qemu_console.py, shared with every other pared-down fork here -
# see that file's own header comment for why.
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "scripts"))
from qemu_console import QEMU, main

# Same full (non "-q") budget as forks/riscv's and forks/x86's own
# test-xv6.py - see forks/x86/test-xv6.py's own comment for why an
# earlier, shorter timeout wasn't safe to assume.
TIMEOUT = 600


def make_qemu(reset=True):
    return QEMU(
        reset_cmds=[
            ["make", "out/kernel.elf"],
            ["rm", "-f", "fs.img", "xv6.img"],
            ["make", "fs.img", "xv6.img"],
        ],
        # qemu-nox, not qemu: the latter also opens a graphical window
        # (QEMUOPTS has no -nographic of its own, only "-serial mon:stdio"
        # alongside the default display) which fails/hangs on a headless
        # host - qemu-nox's "-nographic" is the one this script can drive
        # over a pipe, same shape as forks/x86's own qemu-nox.
        qemu_argv=["make", "qemu-nox"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, timeout=TIMEOUT)
