#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled directly on
# forks/arm-pi1-bis/test-xv6.py's own shape (same lineage/console-input
# convention - this fork's own console.c also only treats '\r' (0x0d) as
# Enter, not raw '\n' (0x0a) - see notes_arch_arm_pi2.txt - so every
# command sent below ends in "\r", not "\n").
#
# claude: this fork's own Makefile takes "hw"/"TOOLCHAIN"/"QEMU" as
# command-line make variable overrides (not just inherited environment -
# "hw" in particular only has a "?=" default of "rpi2", not read from
# any environment variable), matching the top-level Makefile's own
# build-arm-pi2/run-arm-pi2 targets - passed the same way here.
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
    "hw=rpi2",
    f"TOOLCHAIN={os.environ.get('TOOLCHAIN', 'arm-linux-gnueabihf-')}",
    f"QEMU={os.environ.get('QEMU', 'qemu-system-arm')}",
]


def make_qemu(reset=True):
    return QEMU(
        reset_cmds=[
            ["make", *MAKEVARS, "clean"],
            ["make", *MAKEVARS, "kernel7-qemu.bin"],
        ],
        qemu_argv=["make", *MAKEVARS, "qemu"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, usertests_cmd="usertests\r", timeout=TIMEOUT)
