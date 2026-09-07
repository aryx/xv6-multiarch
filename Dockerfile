# Build and boot xv6-multiarch's ports on Ubuntu Linux, pinning the
# toolchain/QEMU this repo's own bring-up was verified against (see
# docs/claude_notes/notes_arch_riscv64.txt and notes_arch_i386.txt) so a
# second machine or CI reproduces them exactly, instead of "whatever apt
# happens to resolve today". This is Phase 3 of
# docs/claude_notes/build-and-test-plan.md.
#
# claude: modeled on ~/c--/Dockerfile and ~/goken/Dockerfile's own shape
# (apt-get update, install the cross toolchains, COPY the source, run
# ./configure, build, then test) - see those files' own header comments
# for the general reasoning this one reuses. Simpler here: only five
# arches are wired up so far (riscv64, i386, x86_64, amd64, riscv32 -
# build-and-test-plan.md's Phases 1, 2, and Phase 4), not all thirteen
# forks/ - extend the ARCH case below (both the apt-get and the
# build/test one) as more arches get their own ./configure detection,
# matching build.md's own Phase 4 order. loongarch/mips/arm64/the ARM
# board ports are already wired into ./configure/the top-level Makefile
# but not added here yet: mips/arm64 don't have a passing test-<arch>
# to run in CI yet (see their own notes_arch_*.txt's open bugs), and
# loongarch/the ARM board ports simply haven't had this step done yet -
# nothing blocking it technically (loongarch's own toolchain turned out
# to be a native arm64 apt package, no binfmt_misc emulation needed, see
# notes_arch_loongarch.txt).
#
# ubuntu:24.04 to match the dev machine the notes_arch_*.txt files record
# toolchain/qemu versions against - not pinned for any of c--'s own
# reasons (no prebuilt checked-in cross-arch objects here to match), just
# consistency with what was actually verified.
FROM ubuntu:24.04

# ARCH selects which arch's toolchain/qemu to install and which
# build-<arch>/test-<arch> Makefile target to run - "all" (the default,
# what plain "docker build ." / "make build-docker" still do) builds and
# tests everything, matching this Dockerfile's original one-image-does-
# everything shape. A CI matrix instead passes one real arch per job
# (--build-arg ARCH=riscv64), so each job only installs and waits on its
# own arch - see .github/workflows/docker.yml's own "strategy: matrix"
# and this file's own final RUN below for how ARCH is consumed.
ARG ARCH=all

RUN apt-get update # needed otherwise can't find any package

# make (this Dockerfile's own build-<arch>/test-<arch> below) and a host
# gcc - both forks/riscv/Makefile's and forks/x86/Makefile's own "mkfs"
# rules compile that HOST tool with plain "gcc", not the cross TOOLPREFIX
# one, since it runs here rather than under QEMU. python3: forks/riscv/
# test-xv6.py and forks/x86/test-xv6.py both need it. None of these three
# are in the base image, and all are needed regardless of ARCH.
#
# claude: python3 wasn't listed here in an earlier version of this
# Dockerfile and the build still passed - purely by accident, because
# qemu-system-gui's own Recommends chain (pulled in before
# --no-install-recommends was added below) happened to drag in
# python3-gi/python3-dbus as transitive dependencies of the GTK/desktop
# stack. Adding --no-install-recommends removed that accidental source
# and immediately surfaced "/usr/bin/env: 'python3': No such file or
# directory" from test-riscv64 - a real, previously-hidden dependency
# this repo's own scripts have, now declared explicitly instead of
# relying on an unrelated package's Recommends to keep providing it.
RUN apt-get install -y --no-install-recommends build-essential python3

# Per-arch toolchain + qemu-system-<arch>, matching what ./configure's own
# detect_toolprefix/detect_qemu_system calls look for - see that script's
# own header comment for why i686-linux-gnu- specifically (Ubuntu ships no
# i386-jos-elf- package) and notes_arch_riscv64.txt's "what was missing"
# section for why qemu-system-riscv64 needs the qemu-system-misc package
# specifically (a separate package from the more common qemu-system-x86/
# -arm). bc: forks/riscv/Makefile's own check-qemu-version target needs it
# directly (not ./configure's own version check, which deliberately
# avoids bc - see notes_arch_riscv64.txt's "PATH hazard" section for why).
#
# claude: --no-install-recommends matters a lot here specifically - both
# qemu-system packages only RECOMMEND (not Depend on) qemu-system-gui and
# its whole GTK/SDL/VTE/icon-theme chain (confirmed via "apt-cache depends
# qemu-system-misc/qemu-system-x86"), which is most of the install time
# and image size this Dockerfile would otherwise pay for a graphical
# frontend NOTHING here ever uses - every QEMU invocation in this repo's
# Makefiles passes -nographic. Same reasoning as ~/goken/Dockerfile's own
# "--no-install-recommends gcc libc6-dev".
RUN case "$ARCH" in \
      riscv64) apt-get install -y --no-install-recommends \
                 gcc-riscv64-unknown-elf qemu-system-misc bc ;; \
      i386)    apt-get install -y --no-install-recommends \
                 gcc-i686-linux-gnu libc6-dev-i386-cross qemu-system-x86 ;; \
      x86_64)  apt-get install -y --no-install-recommends \
                 gcc-x86-64-linux-gnu qemu-system-x86 ;; \
      amd64)   apt-get install -y --no-install-recommends \
                 gcc-x86-64-linux-gnu qemu-system-x86 ;; \
      riscv32) apt-get install -y --no-install-recommends \
                 gcc-riscv64-unknown-elf qemu-system-misc ;; \
      all)     apt-get install -y --no-install-recommends \
                 gcc-riscv64-unknown-elf qemu-system-misc bc \
                 gcc-i686-linux-gnu libc6-dev-i386-cross \
                 gcc-x86-64-linux-gnu qemu-system-x86 ;; \
      *) echo "Dockerfile: unknown ARCH=$ARCH" >&2; exit 1 ;; \
    esac

WORKDIR /src
COPY . .

# Detect the toolchain(s)/qemu installed above and write Makefile.config -
# see ./configure's own header comment. .dockerignore drops the working
# tree's own Makefile.config (if any) so this always regenerates fresh
# inside the image rather than reusing a host-detected one. Harmless that
# ./configure always probes for both arches regardless of ARCH - the
# packages for whichever one wasn't installed above just come back NONE,
# and only the build-<arch>/test-<arch> pair actually invoked below ever
# looks at its own arch's result.
RUN ./configure

# Build, then boot each selected arch under QEMU (software emulation via
# TCG - no /dev/kvm needed or used, same as this repo's own bring-up on an
# aarch64 host emulating both riscv64 and i386) and run its own usertests
# suite end to end - see forks/riscv/test-xv6.py and forks/x86/test-xv6.py.
RUN if [ "$ARCH" = all ]; then make build-all; else make "build-$ARCH"; fi
RUN if [ "$ARCH" = all ]; then make test-all; else make "test-$ARCH"; fi
