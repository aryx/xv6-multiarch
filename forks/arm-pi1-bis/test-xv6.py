#!/usr/bin/env python3
# Minimal usertests-only smoke test for this port, modeled directly on
# forks/arm-pi1/test-xv6.py's own QEMU class (see that file's own header
# comment for the general shape/reasoning this one reuses - same lineage,
# same console-input convention).
#
# claude: this fork's own console.c also only treats '\r' (0x0d) as
# Enter, not raw '\n' (0x0a) - see notes_arch_arm_pi1_bis.txt - so every
# command sent below ends in "\r".
#
# claude: this fork's own Makefile uses "CROSSCOMPILE" (not ARMGNU) as
# its toolchain variable name, and has no separate real-hardware-vs-QEMU
# target split the way forks/arm-pi1 does - "kernel.elf"/"qemu" are the
# only targets, already pointed at "-M raspi1ap"/kernel.img (see this
# fork's own kernel.ld comment on why - notes_arch_arm_pi1_bis.txt bug 5).
import fcntl
import os
import re
import subprocess
import sys
import time

TIMEOUT = 600

MAKEVARS = [
    f"CROSSCOMPILE={os.environ.get('CROSSCOMPILE', 'arm-linux-gnueabihf-')}",
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
        q.cmd("usertests\r")
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
