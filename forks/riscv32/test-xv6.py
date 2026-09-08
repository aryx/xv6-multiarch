#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled on
# forks/amd64/test-xv6.py's own shape: this fork has no crash/log/orphan-
# recovery test programs either (see this directory's own Makefile
# UPROGS list - no grind/sync/logstress/forphan/dorphan), so a usertests
# run is the whole test.
#
# claude: as of docs/claude_notes/notes_arch_riscv32.txt's "bug 4", this
# currently does NOT pass - the freshly-exec'd sh crashes on its own
# first getcmd() call before ever printing a real prompt response, so
# the "usertests\n" command below is never actually processed. Written
# and wired up anyway, per this repo's own "Adding a new arch" recipe,
# so it starts passing automatically once bug 4 is fixed - no need to
# remember to come back and add it later.
#
# Unlike forks/x86/forks/x86_64 (whole-disk xv6.img) or forks/amd64
# (multiboot -kernel with no disk image at all), this fork's own "qemu"
# target already boots via "-kernel kernel/kernel" plus a separate
# virtio-mmio fs.img drive (see kernel/kernel.ld's own comment: entry.S
# is loaded straight at 0x80000000, where qemu's -kernel jumps for
# riscv - no bootblock stage at all) - QEMUOPTS already includes
# "-nographic" itself, so "make qemu" (not "qemu-nox" - this fork has no
# such target) is what this script drives over a pipe.
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
            subprocess.run(["make", "kernel/kernel"], check=True)
            subprocess.run("rm -f fs.img", shell=True, check=True)
            subprocess.run(["make", "fs.img"], check=True)
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


if __name__ == "__main__":
    test_usertests()
