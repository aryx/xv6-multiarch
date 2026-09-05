#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled on
# forks/x86/test-xv6.py's own shape (itself modeled on forks/riscv/
# test-xv6.py's QEMU class): this fork has no crash/log/orphan-recovery
# test programs either (see this directory's own Makefile UPROGS list -
# no grind/sync/logstress/forphan/dorphan), so a usertests run is the
# whole test.
#
# Success detection reuses the same "ALL TESTS PASSED" string forks/x86's
# own usertests.c produces - via exec("echo", echoargv) as usertests.c's
# very last action, not a direct printf (see forks/x86/test-xv6.py's own
# header comment for the mechanism).
import fcntl
import os
import re
import subprocess
import sys
import time

# Same full (non "-q") budget as forks/riscv's and forks/x86's own
# test-xv6.py - see forks/x86/test-xv6.py's own comment for why an
# earlier, shorter timeout wasn't safe to assume.
TIMEOUT = 600


class QEMU:
    def __init__(self, reset=True):
        if reset:
            subprocess.run(["make", "out/kernel.elf"], check=True)
            subprocess.run("rm -f fs.img xv6.img", shell=True, check=True)
            subprocess.run(["make", "fs.img", "xv6.img"], check=True)
        # qemu-nox, not qemu: the latter also opens a graphical window
        # (QEMUOPTS has no -nographic of its own, only "-serial mon:stdio"
        # alongside the default display) which fails/hangs on a headless
        # host - qemu-nox's "-nographic" is the one this script can drive
        # over a pipe, same shape as forks/x86's own qemu-nox.
        self.proc = subprocess.Popen(
            ["make", "qemu-nox"],
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
