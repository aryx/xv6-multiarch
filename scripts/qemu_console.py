#!/usr/bin/env python3
# Shared serial-console QEMU test harness for xv6-multiarch's own
# forks/<name>/test-xv6.py scripts - drives a headless `make <target>`
# boot over a pipe (QEMU's "-nographic"/"qemu-nox" stdio console), sends
# shell commands, and waits for a regex to appear in the output. See
# scripts/qemu_graphics.py for this repo's other QEMU library - that one
# drives a real GTK window over the QMP protocol (screendumps, injected
# keystrokes) for the five ports with a graphical framebuffer console;
# this one drives the plain serial console every wired-up port has, and
# is what "make test-<arch>"/"make quick-test-<arch>" actually run.
#
# claude: this is this repo's own tooling, not anything inherited from
# an upstream xv6 port (see CLAUDE.md's own "Testing conventions"), so
# factoring it out doesn't touch the git-blame-preservation rule
# docs/claude_notes/factorization-plan.md's later kernel-tree merge has
# to respect - that plan stays blocked on every port independently
# building and booting; this is ordinary Python-module deduplication of
# this repo's own test scripts, done because ten of the eleven
# forks/<name>/test-xv6.py files had already converged to near-byte-
# identical (the "boot" CLI mode was originally patched into all ten by
# one small script, precisely because of that). forks/riscv64/
# test-xv6.py is NOT built on this - it has real extra features (crash/
# log/orphan-recovery tests, "-q" usertests) this shared shape doesn't
# need to support, so unifying it in would force shape onto something
# genuinely different rather than removing duplication.
#
# Usage from a fork's own test-xv6.py (see forks/amd64/test-xv6.py for a
# full worked example):
#
#   import os, sys
#   sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "scripts"))
#   from qemu_console import QEMU, main
#
#   def make_qemu(reset=True):
#       return QEMU(
#           reset_cmds=[["make", "kernel"], ["rm", "-f", "fs.img"], ["make", "fs.img"]],
#           qemu_argv=["make", "qemu-nox"],
#           reset=reset,
#       )
#
#   if __name__ == "__main__":
#       main(make_qemu)
import fcntl
import os
import re
import subprocess
import sys
import time


class QEMU:
    """Boots one xv6 kernel headless via `make <qemu_argv>`, over a pipe.

    reset_cmds: list of argv lists run (subprocess.run(..., check=True))
        in order before launching - typically a clean/rebuild sequence.
        Skipped entirely when reset=False (each argv already carries
        whatever env-derived toolchain/QEMU overrides a fork needs - see
        forks/arm/test-xv6.py's own MAKEVARS for why some forks must
        pass these on the command line rather than relying on inherited
        environment variables).
    qemu_argv: argv list that launches QEMU headless over this pipe
        (e.g. ["make", "qemu-nox"] or ["make", *MAKEVARS, "qemu"]) -
        must not open a graphical window (see scripts/qemu_graphics.py
        for the ports where that's the point instead).
    """

    def __init__(self, reset_cmds, qemu_argv, reset=True):
        if reset:
            for argv in reset_cmds:
                subprocess.run(argv, check=True)
        self.proc = subprocess.Popen(
            qemu_argv,
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


def test_boot(make_qemu):
    """Fast smoke check: boot to a shell prompt, no usertests.

    make_qemu: zero-arg callable returning a QEMU(reset=True) instance -
    see each fork's own test-xv6.py for its reset_cmds/qemu_argv.
    """
    q = make_qemu()
    try:
        if not q.wait_for(r"init: starting sh", r"^\$", timeout=60):
            print("ERROR: xv6 did not boot to a shell within 60s")
            sys.exit(1)
        print("boot: reached shell prompt")
    finally:
        q.kill()


def test_usertests(make_qemu, usertests_cmd="usertests\n", timeout=300):
    """Full check: boot, run usertests, assert ALL TESTS PASSED.

    usertests_cmd: the exact command line sent to the shell, line ending
    included - most ports want "usertests\\n", but a few (arm-pi1,
    arm-pi1-bis) only treat '\\r' as Enter - see those forks' own
    test-xv6.py header comments.

    claude: timeout default lowered from 600 to 300 (2026-09-10) - every
    wired-up arch's own full usertests run finishes in well under that
    when it is actually going to pass (i386/amd64 ~60-80s locally under
    Docker; even the slowest ones that legitimately complete stay under
    it). A run still going at 300s in CI turned out to be a real
    kernel-side bug making an ARM32 Pi port's forked children crash
    (trap 4) one after another indefinitely, not a slow-but-fine test -
    see forks/arm-pi2's and forks/arm-pi3's own notes_arch_*.txt. Cutting
    the ceiling in half only makes a genuinely broken run fail faster in
    CI; it does not trade away coverage of anything that was passing.
    """
    q = make_qemu()
    try:
        if not q.wait_for(r"init: starting sh", r"^\$", timeout=60):
            print("ERROR: xv6 did not boot to a shell within 60s")
            sys.exit(1)
        q.cmd(usertests_cmd)
        if not q.wait_for(r"ALL TESTS PASSED", timeout=timeout):
            print(f"ERROR: usertests did not report ALL TESTS PASSED within {timeout}s")
            sys.exit(1)
        print("usertests: ALL TESTS PASSED")
    finally:
        q.kill()


def main(make_qemu, usertests_cmd="usertests\n", timeout=300):
    """Call from a fork's own `if __name__ == "__main__":` block.

    "./test-xv6.py boot" runs the fast smoke check; anything else
    (including no argument at all) runs the full usertests check - same
    dispatch every fork's own test-xv6.py used before this factoring.
    """
    if len(sys.argv) > 1 and sys.argv[1] == "boot":
        test_boot(make_qemu)
    else:
        test_usertests(make_qemu, usertests_cmd=usertests_cmd, timeout=timeout)
