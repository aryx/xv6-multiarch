#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled on
# forks/riscv/test-xv6.py's own QEMU class but pared down: this older,
# pre-2019 xv6-public fork has no crash/log/orphan-recovery test programs
# (see this directory's own Makefile UPROGS list) and no unified test
# harness of its own, so a usertests run is the whole test.
#
# Success detection reuses the exact same "ALL TESTS PASSED" string
# forks/riscv's own usertests.c prints directly - here it's produced the
# OLDER way: this usertests.c's exectest(), its very last action, does
# exec("echo", echoargv) where echoargv is {"echo","ALL","TESTS","PASSED",0}
# (see the top of usertests.c) - a working exec() replaces the usertests
# process image with echo, which then prints that line and exits normally,
# handing control back to the shell.
#
# The actual QEMU-driving/regex-matching machinery lives in
# scripts/qemu_console.py, shared with every other pared-down fork here -
# see that file's own header comment for why.
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "scripts"))
from qemu_console import QEMU, main

# claude: matches forks/riscv/test-xv6.py's own full (non "-q") usertests
# budget, not an arbitrary shorter one - an earlier 300s here was cutting
# it close: a real Docker/CI run timed out mid-suite at exactly 300s with
# no other load on the host, so TCG-emulated i386 usertests genuinely
# needs the same headroom riscv64's does, not a fraction of it, even
# though this fork's own suite has fewer tests than riscv's (no
# grind/sync/logstress/forphan/dorphan - see this file's own header
# comment).
TIMEOUT = 300


def make_qemu(reset=True):
    return QEMU(
        reset_cmds=[
            ["make", "kernel"],
            ["rm", "-f", "fs.img", "xv6.img"],
            ["make", "fs.img", "xv6.img"],
        ],
        # qemu-nox, not qemu: the latter also opens a graphical window
        # (QEMUOPTS has no -nographic of its own, only "-serial mon:stdio"
        # alongside the default display) which fails/hangs on a headless
        # host - qemu-nox's "-nographic" is the one this script can drive
        # over a pipe, same shape as forks/riscv's own "-nographic".
        qemu_argv=["make", "qemu-nox"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, timeout=TIMEOUT)
