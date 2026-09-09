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
            ["make", *MAKEVARS, "kernel-qemu.img"],
        ],
        qemu_argv=["make", *MAKEVARS, "qemu"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, usertests_cmd="usertests\r", timeout=TIMEOUT)
