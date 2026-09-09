#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled on
# forks/arm-pi2/test-xv6.py's own shape (same zhiyihuang/xv6_rpi_port
# lineage) with two deliberate differences:
#
#  - Enter is sent as "\r", the same as the sibling arm-pi1/arm-pi1-bis/
#    arm-pi2 forks, and deliberately so: '\r' (0x0d) is what a real
#    terminal's Return key puts on the serial line, so this is the path
#    an interactive "make run-arm-pi3" user actually exercises. This
#    fork's console.c originally honoured ONLY '\n' and discarded '\r'
#    outright, which is why the Return key did nothing interactively
#    while a scripted "usertests\n" passed - see notes_arch_arm_pi3.txt's
#    own Bug 16. consoleintr() now accepts CR, LF and CRLF alike, so
#    either ending works here; "\r" is chosen to keep the human-facing
#    path under test.
#  - the build needs QEMU_AARCH64_TOOLPREFIX as well, for armstub64.bin
#    (an AArch64 stub this fork alone needs - see its own Makefile and
#    armstub64.S comments), and the QEMU target is qemu-system-aarch64
#    -M raspi3b rather than qemu-system-arm -M raspi2b.
#
# claude: this fork's own Makefile takes "hw"/"TOOLCHAIN"/"QEMU"/
# "QEMU_AARCH64_TOOLPREFIX" as command-line make variable overrides (not
# inherited environment - "hw" in particular only has a "?=" default),
# matching the top-level Makefile's own build-arm-pi3/run-arm-pi3
# targets - passed the same way here.
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
    f"QEMU={os.environ.get('QEMU', 'qemu-system-aarch64')}",
    "QEMU_AARCH64_TOOLPREFIX="
    + os.environ.get("QEMU_AARCH64_TOOLPREFIX", "aarch64-linux-gnu-"),
]


def make_qemu(reset=True):
    return QEMU(
        reset_cmds=[
            ["make", *MAKEVARS, "clean"],
            ["make", *MAKEVARS, "kernel7-qemu.bin", "armstub64.bin"],
        ],
        qemu_argv=["make", *MAKEVARS, "qemu"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, usertests_cmd="usertests\r", timeout=TIMEOUT)
