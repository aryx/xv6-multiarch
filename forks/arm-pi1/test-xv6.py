#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled directly on
# forks/riscv32/test-xv6.py's own QEMU class (see that file's own header
# comment for the general shape/reasoning this one reuses) and
# forks/arm/test-xv6.py's own pared-down shape (no crash/log/orphan
# tiers - this fork has no such test programs either).
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
import fcntl
import os
import re
import subprocess
import sys
import time

TIMEOUT = 600

MAKEVARS = [
    f"ARMGNU={os.environ.get('ARMGNU', 'arm-linux-gnueabihf')}",
    f"QEMU={os.environ.get('QEMU', 'qemu-system-arm')}",
]


class QEMU:
    def __init__(self, reset=True):
        if reset:
            subprocess.run(["make", *MAKEVARS, "clean"], check=True)
            subprocess.run(["make", *MAKEVARS, "kernel-qemu.img"], check=True)
        self.proc = subprocess.Popen(
            ["make", *MAKEVARS, "qemu"],
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
        q.cmd("usertests\r")
        if not q.wait_for(r"ALL TESTS PASSED", timeout=TIMEOUT):
            print(f"ERROR: usertests did not report ALL TESTS PASSED within {TIMEOUT}s")
            sys.exit(1)
        print("usertests: ALL TESTS PASSED")
    finally:
        q.kill()


if __name__ == "__main__":
    test_usertests()
