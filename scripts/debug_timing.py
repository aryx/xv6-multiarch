#!/usr/bin/env python3
# claude: boots a fork's own test-xv6.py QEMU setup and reports WALL-CLOCK
# TIMING for each of a list of regex checkpoints, instead of the single
# pass/fail scripts/qemu_console.py's own test_usertests() gives.
#
# Built for the arm-pi3 mem()/preempt() investigation (2026-09-11, see
# docs/claude_notes/notes_arch_arm_pi3.txt) - test_usertests() alone can only
# tell you PASS or "didn't finish within N seconds", not where the time
# actually went. This answers "is stage X fast, slow, or the one that never
# finishes" by timestamping each checkpoint as it appears, and pairs with
# the gdb-sampling technique in notes_debugging_techniques.txt (attach
# repeatedly, check whether $pc is actually moving) for the complementary
# "is this a real hang or just slow" question - gdbstub reliability varies
# by board (it was not usable for arm-pi3's own raspi3b/AArch64-EL-stub
# setup, which is what motivated timing checkpoints as the fallback).
#
# Reuses the target fork's own forks/<name>/test-xv6.py make_qemu() (so it
# inherits that fork's exact MAKEVARS/reset_cmds/qemu_argv, no per-fork
# special-casing here) and scripts/qemu_console.py's own QEMU class (so the
# actual boot/pipe/read machinery isn't duplicated).
#
# Usage (from the repo root):
#   scripts/debug_timing.py arm-pi3 --cmd "usertests p m" \
#       --checkpoint "mem test" --checkpoint "mem ok" \
#       --checkpoint preempt --checkpoint "ALL TESTS PASSED"
#
#   scripts/debug_timing.py arm-pi3 --no-reset   # skip the rebuild step,
#                                                 # re-run against an
#                                                 # already-built kernel
import argparse
import importlib.util
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from qemu_console import QEMU  # noqa: E402


def load_make_qemu(fork_name):
    path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "forks", fork_name, "test-xv6.py"
    )
    spec = importlib.util.spec_from_file_location(f"{fork_name}_test_xv6", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod.make_qemu


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("fork", help="fork name under forks/, e.g. arm-pi3")
    ap.add_argument(
        "--cmd",
        default="usertests\n",
        help='shell command line to send after boot, line ending included - most '
        "ports want \\n, a few (arm-pi1, arm-pi1-bis, arm-pi2, arm-pi3) only treat "
        "\\r as Enter, see that fork's own test-xv6.py header comment",
    )
    ap.add_argument(
        "--checkpoint",
        action="append",
        default=[],
        metavar="REGEX",
        help="regex to timestamp when first seen, checked in the order given "
        "(repeatable). If none given, just waits for 'ALL TESTS PASSED'.",
    )
    ap.add_argument(
        "--stage-timeout",
        type=float,
        default=300,
        help="max seconds to wait for EACH checkpoint (default 300 - this repo's "
        "own CI/test_usertests budget, see qemu_console.py's own comment on why "
        "300 and not more: a real run finishing at all takes well under it, so "
        "a stage still running past 300s is itself evidence of a real bug, not "
        "just something to wait out longer)",
    )
    ap.add_argument("--boot-timeout", type=float, default=60)
    ap.add_argument(
        "--no-reset",
        action="store_true",
        help="skip the fork's own reset_cmds (rebuild) - use for a fast re-run "
        "against a kernel you already built",
    )
    args = ap.parse_args()

    checkpoints = args.checkpoint or [r"ALL TESTS PASSED"]

    make_qemu = load_make_qemu(args.fork)
    q = make_qemu(reset=not args.no_reset)
    t0 = time.time()
    ok = True
    try:
        if not q.wait_for(r"init: starting sh", timeout=args.boot_timeout):
            print(f"[{time.time()-t0:6.1f}s] TIMEOUT waiting for boot")
            sys.exit(1)
        print(f"[{time.time()-t0:6.1f}s] boot")
        q.cmd(args.cmd)
        for cp in checkpoints:
            if q.wait_for(cp, timeout=args.stage_timeout):
                print(f"[{time.time()-t0:6.1f}s] {cp}")
            else:
                print(f"[{time.time()-t0:6.1f}s] TIMEOUT waiting for {cp!r}")
                ok = False
                break
    finally:
        q.kill()
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
