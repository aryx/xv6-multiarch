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
            ["make", *MAKEVARS, "kernel7-qemu.bin"],
        ],
        qemu_argv=["make", *MAKEVARS, "qemu"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, usertests_cmd="usertests\r", timeout=TIMEOUT)
