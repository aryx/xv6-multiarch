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
import fcntl
import os
import re
import subprocess
import sys
import time

# Same full (non "-q") budget as every other fork's own test-xv6.py.
TIMEOUT = 600


class QEMU:
    def __init__(self, reset=True):
        if reset:
            subprocess.run(["make", "clean"], check=True)
            subprocess.run(["make", "all"], check=True)
        self.proc = subprocess.Popen(
            ["make", "qemu"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        fl = fcntl.fcntl(self.proc.stdout, fcntl.F_GETFL)
        fcntl.fcntl(self.proc.stdout, fcntl.F_SETFL, fl | os.O_NONBLOCK)
        self.buf = ""
        time.sleep(1)

    def cmd(self, s):
        self.proc.stdin.write(s.encode())
        self.proc.stdin.flush()

    def read(self):
        try:
            data = self.proc.stdout.read()
        except (BlockingIOError, TypeError):
            data = None
        if data:
            self.buf += data.decode("utf-8", "replace")

    def lines(self):
        parts = self.buf.split("\n")
        self.buf = parts[-1]
        return parts[:-1]

    def wait_for(self, *regexps, timeout):
        deadline = time.time() + timeout
        while time.time() < deadline:
            self.read()
            for line in self.lines():
                print(line)
                if any(re.search(r, line) for r in regexps):
                    return True
            time.sleep(0.2)
        return False

    def kill(self):
        self.proc.terminate()
        try:
            self.proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.proc.kill()


def test_usertests():
    q = QEMU()
    try:
        if not q.wait_for(r"init: starting sh", r"^\$", timeout=60):
            print("ERROR: xv6 did not boot to a shell within 60s")
            sys.exit(1)
        q.cmd("usertests\n")
        if not q.wait_for(r"ALL TESTS PASSED", timeout=TIMEOUT):
            print(f"ERROR: usertests did not report ALL TESTS PASSED within {TIMEOUT}s")
            sys.exit(1)
        print("usertests: ALL TESTS PASSED")
    finally:
        q.kill()


# claude: fast smoke check for "make quick-test-<arch>"/"make test-all" -
# just asserts the kernel boots to an interactive shell, skipping the
# (much slower, often TCG-emulation-bound) usertests run below. See
# "make stress-test-<arch>"/"make stress-test-all" for the full check.
def test_boot():
    q = QEMU()
    try:
        if not q.wait_for(r"init: starting sh", r"^\$", timeout=60):
            print("ERROR: xv6 did not boot to a shell within 60s")
            sys.exit(1)
        print("boot: reached shell prompt")
    finally:
        q.kill()


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "boot":
        test_boot()
    else:
        test_usertests()
