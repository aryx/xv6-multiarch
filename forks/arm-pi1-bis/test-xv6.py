#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled directly on
# forks/arm-pi1/test-xv6.py's own shape (same lineage, same console-input
# convention).
#
# claude: this fork's own console.c also only treats '\r' (0x0d) as
# Enter, not raw '\n' (0x0a) - see notes_arch_arm_pi1_bis.txt - so every
# command sent below ends in "\r".
#
# claude: this fork's own Makefile uses "CROSSCOMPILE" (not ARMGNU) as
# its toolchain variable name, and has no separate real-hardware-vs-QEMU
# target split the way forks/arm-pi1 does - "kernel.elf"/"qemu" are the
# only targets, already pointed at "-M raspi1ap"/kernel.img (see this
# fork's own kernel.ld comment on why - notes_arch_arm_pi1_bis.txt bug 5).
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
    f"CROSSCOMPILE={os.environ.get('CROSSCOMPILE', 'arm-linux-gnueabihf-')}",
    f"QEMU={os.environ.get('QEMU', 'qemu-system-arm')}",
]


def make_qemu(reset=True):
    return QEMU(
        reset_cmds=[
            ["make", *MAKEVARS, "clean"],
            ["make", *MAKEVARS, "kernel.elf"],
        ],
        qemu_argv=["make", *MAKEVARS, "qemu"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, usertests_cmd="usertests\r", timeout=TIMEOUT)
