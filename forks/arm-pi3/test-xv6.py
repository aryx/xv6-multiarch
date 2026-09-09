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
            ["make", *MAKEVARS, "kernel7-qemu.bin", "armstub64.bin"],
        ],
        qemu_argv=["make", *MAKEVARS, "qemu"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, usertests_cmd="usertests\r", timeout=TIMEOUT)
