#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled on
# forks/riscv/test-xv6.py's own QEMU class but pared down: this older,
# pre-2019 xv6-public fork has no crash/log/orphan-recovery test programs
# (see this directory's own Makefile UPROGS list) and no unified test
# harness of its own, so a usertests run is the whole test.
#
# Success detection reuses the exact same "ALL TESTS PASSED" string
# forks/riscv's own usertests.c prints directly - here it's produced the
# OLDER way: this usertests.c's exectest(), its very last action, does
# exec("echo", echoargv) where echoargv is {"echo","ALL","TESTS","PASSED",0}
# (see the top of usertests.c) - a working exec() replaces the usertests
# process image with echo, which then prints that line and exits normally,
# handing control back to the shell.
import fcntl
import os
import re
import subprocess
import sys
import time

# claude: matches forks/riscv/test-xv6.py's own full (non "-q") usertests
# budget, not an arbitrary shorter one - an earlier 300s here was cutting
# it close: a real Docker/CI run timed out mid-suite at exactly 300s with
# no other load on the host, so TCG-emulated i386 usertests genuinely
# needs the same headroom riscv64's does, not a fraction of it, even
# though this fork's own suite has fewer tests than riscv's (no
# grind/sync/logstress/forphan/dorphan - see this file's own header
# comment).
TIMEOUT = 600


class QEMU:
    def __init__(self, reset=True):
        if reset:
            subprocess.run(["make", "kernel"], check=True)
            subprocess.run("rm -f fs.img xv6.img", shell=True, check=True)
            subprocess.run(["make", "fs.img", "xv6.img"], check=True)
        # qemu-nox, not qemu: the latter also opens a graphical window
        # (QEMUOPTS has no -nographic of its own, only "-serial mon:stdio"
        # alongside the default display) which fails/hangs on a headless
        # host - qemu-nox's "-nographic" is the one this script can drive
        # over a pipe, same shape as forks/riscv's own "-nographic".
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
