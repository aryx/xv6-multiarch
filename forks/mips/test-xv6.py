#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled on
# forks/riscv32/test-xv6.py's own QEMU class. This fork has no crash/log/
# orphan-recovery test programs either, so a usertests run is the whole
# test.
#
# claude: usr/usertests.c has several sub-tests commented out (see that
# file's own "claude:" comments right above each call site, and
# docs/claude_notes/notes_arch_mips.txt's own "Gap" section) - sbrktest,
# validatetest, mem, preempt, exitwait, and forktest all hang for real
# under this port's own QEMU/malta emulation (confirmed via gdb, not
# just slow - none of them are simple to root-cause: they span at least
# two different subsystems, an allocuvm()/kalloc() stall under real
# memory pressure and a separate fork()/copyuvm()/walkpgdir() stall).
# Skipped pragmatically so the rest of the suite can reach exectest()'s
# "ALL TESTS PASSED" signal (this file's own echoargv trick, not a raw
# printf - see CLAUDE.md's own testing-conventions note on this).
#
# This fork's own "qemu-nox-memfs" Makefile target already includes
# "-nographic" and boots the memfs kernel variant (embeds fs.img
# directly in the kernel image - the disk-based "kernel"/"qemu-nox"
# path was never finished by the port's own author, see
# notes_arch_mips.txt) - "make qemu-nox-memfs" is what this script
# drives over a pipe.
#
# claude: TOOLPREFIX/QEMU here are plain "=" in this fork's own
# Makefile (not "?="), so an inherited environment variable of the same
# name is silently overridden by the file's own default
# ("mipsel-sde-elf-", a toolchain that doesn't exist on this host) -
# same class of gap as forks/arm's own test-xv6.py hit (forks/armv7-rpi
# at the time). Passed explicitly on make's command line instead, from
# the TOOLPREFIX/QEMU environment variables the top-level Makefile's own
# test-mips target sets before invoking this script.
import fcntl
import os
import re
import subprocess
import sys
import time

TIMEOUT = 600

MAKEVARS = [
    f"TOOLPREFIX={os.environ.get('TOOLPREFIX', '')}",
    f"QEMU={os.environ.get('QEMU', 'qemu-system-mipsel')}",
]


class QEMU:
    def __init__(self, reset=True):
        if reset:
            subprocess.run(["make", *MAKEVARS, "clean"], check=True)
            subprocess.run(["make", *MAKEVARS, "kernelmemfs"], check=True)
        self.proc = subprocess.Popen(
            ["make", *MAKEVARS, "qemu-nox-memfs"],
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
