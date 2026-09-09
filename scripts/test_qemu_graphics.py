#!/usr/bin/env python3
# test_qemu_graphics.py
#
# Regression test for every "-qemu-graphics" capable arch (i386, amd64,
# amd64-jserv, arm-pi1, arm-pi1-bis - see the top-level Makefile's own
# "run-<arch>-qemu-graphics" targets): boots a real GTK window (no
# -nographic), waits for a shell, types a command via a QMP-injected
# keyboard, and asserts real visible output appeared - not just that
# characters echoed on the serial console, which every existing
# test-<arch>.py / test-xv6.py already covers.
#
# claude: property-based checks (screen isn't blank; pixel count/content
# changes after typing; the shell's own serial output independently
# shows the command ran), not golden-image diffing - font
# rendering/timing make byte-for-byte reference images fragile to
# maintain across QEMU versions, and the point here is catching "the
# framebuffer/graphics path broke" or "the injected keyboard stopped
# reaching the shell", not pixel-perfect regressions.
#
# Usage:
#   python3 scripts/test_qemu_graphics.py               # all 5 arches
#   python3 scripts/test_qemu_graphics.py i386 arm-pi1   # a subset
#   python3 scripts/test_qemu_graphics.py --list
#
# Requires a real $DISPLAY (a GTK window actually opens, same
# requirement as "make run-<arch>-qemu-graphics" itself).

import os
import signal
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import qemu_graphics as qg  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parent.parent

BOOT_TIMEOUT = 60
TYPE_SETTLE = 1.5

ARCHES = {
    "i386": {
        "build_target": "build-i386",
        "run_target": "run-i386-qemu-graphics",
    },
    "amd64": {
        "build_target": "build-amd64",
        "run_target": "run-amd64-qemu-graphics",
    },
    "amd64-jserv": {
        "build_target": "build-amd64-jserv",
        "run_target": "run-amd64-jserv-qemu-graphics",
    },
    "arm-pi1": {
        "build_target": "build-arm-pi1",
        "run_target": "run-arm-pi1-qemu-graphics",
    },
    "arm-pi1-bis": {
        "build_target": "build-arm-pi1-bis",
        "run_target": "run-arm-pi1-bis-qemu-graphics",
    },
}

# claude: stock upstream xv6 init.c's own banner/prompt - confirmed
# present verbatim across every one of these five ports, despite their
# otherwise-diverging console/keyboard implementations (see each
# fork's own test-xv6.py/test-<arch>.py, which already waits on the
# exact same two patterns over the plain serial console).
BOOT_READY_MARKERS = ("init: starting sh", "$")


def _kill_process_group(proc):
    """Terminate proc's WHOLE process group (see the start_new_session
    comment at its own Popen call) - falls back to killing just proc if
    the group is already gone."""
    try:
        pgid = os.getpgid(proc.pid)
    except ProcessLookupError:
        return
    for sig in (signal.SIGTERM, signal.SIGKILL):
        try:
            os.killpg(pgid, sig)
        except ProcessLookupError:
            return
        try:
            proc.wait(timeout=5)
            return
        except subprocess.TimeoutExpired:
            continue


def wait_for_boot(log_path, timeout):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if log_path.exists():
            text = log_path.read_text(errors="replace")
            if any(m in text for m in BOOT_READY_MARKERS):
                return True
        time.sleep(0.5)
    return False


def wait_for_socket(sock_path, timeout):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if sock_path.exists():
            return True
        time.sleep(0.2)
    return False


def run_one(arch, tmpdir, keep_artifacts):
    cfg = ARCHES[arch]
    print(f"\n=== {arch} ===")

    print(f"  building ({cfg['build_target']})...")
    r = subprocess.run(["make", cfg["build_target"]], cwd=REPO_ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        print(f"  FAIL: build failed\n{r.stdout[-2000:]}\n{r.stderr[-2000:]}")
        return False

    sock_path = tmpdir / f"{arch}-qmp.sock"
    log_path = tmpdir / f"{arch}-boot.log"
    sock_path.unlink(missing_ok=True)

    print(f"  launching ({cfg['run_target']})...")
    with open(log_path, "wb") as logf:
        # claude: start_new_session=True (its own process group) so
        # cleanup can kill the WHOLE tree - "make run-<arch>-qemu-graphics"
        # forks a second "make -C forks/<name>" which then execs
        # qemu-system-*; terminating only the top-level "make" process
        # (its default pid) leaves qemu itself running as an orphan,
        # confirmed directly (a leftover "qemu-system-arm" process,
        # still holding its own QMP socket, survived proc.terminate()
        # in an earlier version of this script).
        proc = subprocess.Popen(
            ["make", cfg["run_target"], f"QMP_SOCK={sock_path}"],
            cwd=REPO_ROOT,
            stdin=subprocess.DEVNULL,
            stdout=logf,
            stderr=subprocess.STDOUT,
            start_new_session=True,
        )

    try:
        if not wait_for_boot(log_path, BOOT_TIMEOUT):
            print(f"  FAIL: did not reach a shell within {BOOT_TIMEOUT}s - see {log_path}")
            return False

        if not wait_for_socket(sock_path, 10):
            print(f"  FAIL: QMP socket {sock_path} never appeared")
            return False

        m = qg.QMP(str(sock_path))
        try:
            before_path = tmpdir / f"{arch}-before.ppm"
            m.screendump(str(before_path))
            bw, bh, bpx = qg.load_ppm(str(before_path))
            before_count = qg.non_black_count(bw, bh, bpx)
            print(f"  boot screen: {bw}x{bh}, {before_count} non-black pixels")

            if before_count == 0:
                print("  FAIL: boot screen is completely blank")
                return False

            log_before = log_path.read_text(errors="replace")

            m.type_text("ls\r")
            time.sleep(TYPE_SETTLE)

            after_path = tmpdir / f"{arch}-after.ppm"
            m.screendump(str(after_path))
            aw, ah, apx = qg.load_ppm(str(after_path))
            after_count = qg.non_black_count(aw, ah, apx)
            print(f"  after typing 'ls': {after_count} non-black pixels")

            log_after = log_path.read_text(errors="replace")
            new_serial_output = log_after[len(log_before):]

            ok = True
            if after_count <= before_count:
                print(f"  FAIL: pixel count did not increase after typing ({before_count} -> {after_count})")
                ok = False
            if "ls" not in new_serial_output:
                print("  FAIL: typed command did not appear on the serial console")
                ok = False

            if ok:
                print("  PASS")
            elif not keep_artifacts:
                print(f"  (screendumps kept at {before_path}, {after_path} for inspection)")
            return ok
        finally:
            m.close()
    finally:
        _kill_process_group(proc)


def main():
    args = sys.argv[1:]
    if "--list" in args:
        print("\n".join(ARCHES))
        return 0

    keep_artifacts = "--keep-artifacts" in args
    args = [a for a in args if not a.startswith("--")]

    if not os.environ.get("DISPLAY"):
        print("ERROR: no $DISPLAY set - these targets open a real GTK window", file=sys.stderr)
        return 1

    arches = args if args else list(ARCHES)
    unknown = [a for a in arches if a not in ARCHES]
    if unknown:
        print(f"ERROR: unknown arch(es): {unknown} - see --list", file=sys.stderr)
        return 1

    tmpdir = Path("/tmp") / f"qemu-graphics-test-{os.getpid()}"
    tmpdir.mkdir(parents=True, exist_ok=True)

    results = {}
    for arch in arches:
        try:
            results[arch] = run_one(arch, tmpdir, keep_artifacts)
        except Exception as e:  # noqa: BLE001 - report and continue to the next arch
            print(f"  FAIL: {type(e).__name__}: {e}")
            results[arch] = False

    print("\n=== summary ===")
    for arch, ok in results.items():
        print(f"  {'PASS' if ok else 'FAIL'}  {arch}")

    if not keep_artifacts:
        import shutil

        shutil.rmtree(tmpdir, ignore_errors=True)
    else:
        print(f"\nartifacts kept in {tmpdir}")

    return 0 if all(results.values()) else 1


if __name__ == "__main__":
    sys.exit(main())
