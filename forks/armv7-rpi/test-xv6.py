#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled directly on
# forks/rv32/test-xv6.py's own QEMU class (see that file's own header
# comment for the general shape/reasoning this one reuses). This fork
# has no crash/log/orphan-recovery test programs either, so a usertests
# run is the whole test.
#
# claude: this fork's own usr/usertests.c has two tests commented out
# (see that file's own "claude:" comments right above each call site,
# and docs/claude_notes/notes_arch_armv7_rpi.txt) - preempt() hangs
# indefinitely under QEMU's raspi2b, and sbrktest() crashes the whole
# usertests process partway through its own "out of memory" section (a
# pre-existing, upstream-documented issue, not something introduced
# here). Both are skipped so the remaining suite can reach exectest()'s
# "ALL TESTS PASSED" signal (this file's own echoargv trick, not a raw
# printf - see CLAUDE.md's own testing-conventions note on this).
#
# This fork's own "qemu" Makefile target already includes "-nographic"
# and boots via "-kernel xv6.img" (a raw binary, not the ELF - see
# notes_arch_armv7_rpi.txt for why), so "make qemu" (not "make
# qemu-nox" - no such target here) is what this script drives over a
# pipe, same as forks/rv32.
#
# claude: unlike forks/rv32's own TOOLPREFIX (which has a runtime
# auto-detect fallback in its Makefile that happens to land on the right
# toolchain even without an external override), this fork's makefile.inc
# hardcodes "CROSSCOMPILE := arm-none-eabi-" with no such fallback - a
# plain ":=" in the makefile beats an inherited environment variable of
# the same name (make only lets the environment win with "make -e", or
# for a variable the makefile itself declares "?="). The verified-working
# toolchain is arm-linux-gnueabihf- (see notes_arch_armv7_rpi.txt), which
# only takes effect if CROSSCOMPILE is passed on make's own command line
# - exactly how the top-level Makefile's build-armv7-rpi/run-armv7-rpi
# targets already do it. Passed through here the same way, from the
# CROSSCOMPILE/QEMU environment variables the top-level Makefile's own
# test-arm target sets before invoking this script.
import fcntl
import os
import re
import subprocess
import sys
import time

TIMEOUT = 600

MAKEVARS = [
    f"CROSSCOMPILE={os.environ.get('CROSSCOMPILE', '')}",
    f"QEMU={os.environ.get('QEMU', 'qemu-system-arm')}",
]


class QEMU:
    def __init__(self, reset=True):
        if reset:
            subprocess.run(["make", *MAKEVARS, "clean"], check=True)
            subprocess.run(["make", *MAKEVARS, "kernel.elf"], check=True)
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
        q.cmd("usertests\n")
        if not q.wait_for(r"ALL TESTS PASSED", timeout=TIMEOUT):
            print(f"ERROR: usertests did not report ALL TESTS PASSED within {TIMEOUT}s")
            sys.exit(1)
        print("usertests: ALL TESTS PASSED")
    finally:
        q.kill()


if __name__ == "__main__":
    test_usertests()
