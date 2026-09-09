#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled on
# forks/x86/test-xv6.py's own pared-down shape: this fork has no
# crash/log/orphan-recovery test programs either (see this directory's
# own Makefile UPROGS list - no grind/sync/logstress/forphan/dorphan), so
# a usertests run is the whole test.
#
# Unlike every other fork's own "qemu"/"qemu-nox" target, this one embeds
# the whole filesystem image directly into the kernel binary (mkfs writes
# fs.img, then "xxd -i fs.img > kernel/ramdisk.h", which kernel/ramdisk.c
# includes and the kernel serves as an in-memory ramdisk - see
# notes_arch_loongarch.txt) - there is no separate disk/virtio image to
# reset, so the reset step here is just "make all" (which itself already
# rebuilds fs.img before the kernel, in that order, so the freshly
# embedded ramdisk always matches the just-built UPROGS).
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


def make_qemu(reset=True):
    return QEMU(
        reset_cmds=[
            # claude: NO "make clean" here - it used to be the first reset
            # command, and made "make test-all" recompile this port from
            # scratch even straight after a full "make build-all". Unlike
            # the Pi ports it was not even compensating for anything:
            # this Makefile already both generated (-MD) and read
            # (-include) its header dependencies, so an incremental build
            # was already trustworthy and the clean was pure waste.
            # Nothing else needs resetting either - this port boots with
            # "-kernel" and its filesystem is embedded in the kernel
            # image, so guest writes land in RAM and die with QEMU.
            # See docs/claude_notes/notes_build_system.txt.
            ["make", "all"],
        ],
        qemu_argv=["make", "qemu"],
        reset=reset,
    )


if __name__ == "__main__":
    main(make_qemu, timeout=TIMEOUT)
