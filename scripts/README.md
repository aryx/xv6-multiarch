# scripts/

## qemu_graphics.py / test_qemu_graphics.py

Maintained, meant to be run today. A regression test for every port
with a "run-`<arch>`-qemu-graphics" Makefile target (i386, amd64,
amd64-jserv, arm-pi1, arm-pi1-bis, arm-pi3) - boots each one with a real GTK
window (no `-nographic`), types a shell command through a QMP-injected
keyboard, and asserts real visible output appeared on the emulated
screen. This is a different signal from what each port's own
`test-<arch>.py`/`test-xv6.py` already checks (that usertests passes
over the plain serial console) - it exercises the actual graphical
framebuffer/VGA console and keyboard input path instead, which nothing
else in this repo tests automatically. See
`docs/claude_notes/notes_tutorial_qemu.txt` for the underlying QMP/
screendump techniques this borrows from.

```sh
make test-all-graphics                               # all 6 ports, via the top-level Makefile
python3 scripts/test_qemu_graphics.py               # same thing, direct
python3 scripts/test_qemu_graphics.py i386 arm-pi1   # a subset
python3 scripts/test_qemu_graphics.py --list
python3 scripts/test_qemu_graphics.py --keep-artifacts   # keep screendumps in /tmp for inspection
```

Requires a real `$DISPLAY` (a GTK window actually opens - same
requirement as `make run-<arch>-qemu-graphics` itself). Deliberately
separate from `make test-all`/CI - nothing headless can assert against a
real window; run `make test-all-graphics` by hand after touching a
graphics/console/keyboard code path in one of these six ports.

`qemu_graphics.py` is a library, not a script - a QMP client
(`send_key`/`type_text`/`screendump`) and P6 PPM helpers
(`load_ppm`/`non_black_count`/`distinct_colors`/`ascii_art`), reusable
for other one-off QEMU-graphics scripting beyond this test runner.

Every `run-<arch>-qemu-graphics` target (this repo's top-level
Makefile, and forks/arm-pi1's/forks/arm-pi1-bis's/forks/arm-pi3's own
Makefiles)
accepts an optional `QMP_SOCK=<path>` to add a QMP socket to the QEMU
command line - unset by default, so plain interactive use
(`make run-i386-qemu-graphics`, no extra variables) is unaffected.

## qemu_console.py

The plain-serial-console counterpart to `qemu_graphics.py` above - a
shared library, not a script. Ten of the eleven `forks/<name>/
test-xv6.py` files (every wired-up port except `riscv64`, which has its
own richer crash/log/orphan-recovery harness) import `QEMU`/`main` from
here instead of each carrying its own copy of the same
Popen/wait_for/kill machinery. Each fork's own `test-xv6.py` stays the
place that encodes what's actually specific to that port - its own
`reset_cmds`/`qemu_argv` (which `make` targets to run, whether it needs
explicit `MAKEVARS` on the command line, `\n` vs `\r` as Enter) - see
any of them (`forks/amd64/test-xv6.py` is a short one) for the pattern.
Factored out 2026-09-09 after the "boot" CLI mode (see `make
quick-test-<arch>` in the top-level Makefile) had to be patched into all
ten files at once, by hand - not something this repo's own
plan_factorization.md is blocked on: these are this repo's own test
scripts, not upstream kernel/user code, so there's no git-blame history
to preserve across the merge.

## repo-history/

Archived record of how this repository itself was assembled - not
related to the qemu-graphics tooling above. See
`repo-history/README.md`.
