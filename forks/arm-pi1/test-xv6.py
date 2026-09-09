#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled directly on
# forks/riscv32/test-xv6.py's own shape and forks/arm/test-xv6.py's own
# pared-down shape (no crash/log/orphan tiers - this fork has no such
# test programs either).
#
# claude: this fork's own console.c drops a raw '\n' (0x0a) input byte
# outright ("if(c == 0xa) break;") and only treats '\r' (0x0d) as
# Enter - a real terminal-input convention, not a bug (see
# notes_arch_arm_pi1.txt) - so every command sent below ends in "\r",
# not "\n".
#
# claude: this script drives the fork's own additive, QEMU-only "qemu"
# Makefile target (kernel-qemu.img, never the real-hardware
# kernel.img/"all" ones - see this fork's own Makefile/kernel-qemu.ld
# comments) - passed ARMGNU (no trailing "-", this fork's own toolchain
# variable name) and QEMU from the environment, the same way the
# top-level Makefile's own build-arm-pi1/run-arm-pi1 targets already do
# it (ARMGNU=$(patsubst %-,%,$(TOOLPREFIX_ARM_PI1))).
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
    f"ARMGNU={os.environ.get('ARMGNU', 'arm-linux-gnueabihf')}",
    f"QEMU={os.environ.get('QEMU', 'qemu-system-arm')}",
]


def make_qemu(reset=True):
    return QEMU(
        reset_cmds=[
            ["make", *MAKEVARS, "clean"],
            ["make", *MAKEVARS, "kernel-qemu.img"],
        ],
        qemu_argv=["make", *MAKEVARS, "qemu"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, usertests_cmd="usertests\r", timeout=TIMEOUT)
