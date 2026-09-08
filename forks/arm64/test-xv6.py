#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled on
# forks/riscv32/test-xv6.py's own shape (same "make qemu" boot mechanism,
# same kernel/user-split lineage). This fork's own UPROGS list adds
# `grind` (a stress test) beyond what rv32 has, but usertests.c's own
# suite is still the whole test here - grind is not separately driven.
#
# claude: as of docs/claude_notes/notes_arch_arm64.txt's "bug 1", this
# currently does NOT pass - the freshly-exec'd sh crashes shortly after
# printing its prompt, so the "usertests\n" command below is never
# actually processed. Written and wired up anyway, per this repo's own
# "Adding a new arch" recipe, so it starts passing automatically once
# bug 1 is fixed.
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
