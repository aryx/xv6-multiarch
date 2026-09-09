#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled on
# forks/arm-pi2/test-xv6.py's own shape (this fork has no crash/log/
# orphan-recovery test programs either - see this directory's own
# Makefile UPROGS list - so a usertests run is the whole test).
#
# Success detection uses the "ALL TESTS PASSED" string this fork's own
# user/usertests.c prints directly (a plain printf as its last action,
# not the exec("echo", ...) trick forks/i386/forks/amd64 need - see
# CLAUDE.md's own "Testing conventions" on why that differs per port).
# Commands are sent with "\n": this fork's console.c normalises '\r' to
# '\n' on input (kernel/console.c's consoleintr), so unlike arm-pi1/
# arm-pi1-bis/arm-pi2 either ending works here.
#
# claude: this fork's own Makefile takes TOOLPREFIX/QEMU/QEMUMACHINE/
# QEMUMEM as make variable overrides, matching the top-level Makefile's
# own build-arm64-pi4/run-arm64-pi4 targets - passed the same way here.
# QEMUMACHINE/QEMUMEM in particular are NOT optional: the Makefile's own
# defaults are "raspi4b1g"/"1G", the board name in k-mrm's patched
# qemu fork this port was developed against, which mainline qemu does
# not have. Mainline calls its Pi 4 board "raspi4b" and models the
# 2GB board revision (0xb03115, hw/arm/raspi4b.c), and a raspi machine
# rejects a -m that doesn't match its revision, so both must be passed.
#
# NOTE: mainline qemu only grew "-M raspi4b" in 9.1, so unlike every
# other fork here this script needs a qemu newer than most distros
# package (this host's own /usr/bin/qemu-system-aarch64 is 8.2.2 and
# cannot run it at all). That's why the top-level Makefile deliberately
# leaves arm64-pi4 out of the test-all/stress-test-all umbrellas and out
# of the Dockerfile/CI matrix - see notes_arch_arm64_pi4.txt.
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

MAKEVARS = [
    f"TOOLPREFIX={os.environ.get('TOOLPREFIX', 'aarch64-linux-gnu-')}",
    f"QEMU={os.environ.get('QEMU', 'qemu-system-aarch64')}",
    f"QEMUMACHINE={os.environ.get('QEMUMACHINE', 'raspi4b')}",
    f"QEMUMEM={os.environ.get('QEMUMEM', '2G')}",
]


def make_qemu(reset=True):
    return QEMU(
        reset_cmds=[
            ["make", *MAKEVARS, "clean"],
            ["make", *MAKEVARS, "kernel/kernel"],
        ],
        qemu_argv=["make", *MAKEVARS, "qemu"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, timeout=TIMEOUT)
