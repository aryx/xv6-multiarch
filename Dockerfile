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
# for the general reasoning this one reuses. Simpler here: only thirteen
# arches are wired up so far (riscv64, i386, amd64-jserv, amd64, riscv32, arm,
# arm-pi1, arm-pi1-bis, arm-pi2, arm-pi3, arm64, mips, loongarch - build-and-test-plan.md's
# Phases 1, 2, and Phase 4), not every forks/ directory - extend the ARCH
# case below (both the apt-get and the build/test one) as more arches get
# their own ./configure detection, matching build.md's own Phase 4 order.
# arm64-pi4 is the one wired-up arch with no case of its own below, and
# it is NOT because the port is unfinished - it boots to a shell and
# passes its own usertests. It is the emulator: qemu only grew a
# Raspberry Pi 4 board ("-M raspi4b") in 9.1, ubuntu:24.04 packages
# 8.2.2, and no apt line fixes that. The qemu it needs is a local source
# build, which is exactly the kind of thing a reproducible image should
# not be doing, so arm64-pi4's BOOT is run directly on a dev machine
# instead - same reason it has no .github/workflows/docker.yml matrix
# entry and is not in the top-level Makefile's test-all/stress-test-all.
# Its BUILD is a different matter and is covered here: it needs only
# gcc-aarch64-linux-gnu, which the ARCH=all case below already installs
# for arm64/arm-pi3, and build-arm64-pi4 is part of build-all - so the
# ARCH=all path does compile it. See notes_arch_arm64_pi4.txt.
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
      amd64-jserv) apt-get install -y --no-install-recommends \
                 gcc-x86-64-linux-gnu qemu-system-x86 ;; \
      amd64)   apt-get install -y --no-install-recommends \
                 gcc-x86-64-linux-gnu qemu-system-x86 ;; \
      riscv32) apt-get install -y --no-install-recommends \
                 gcc-riscv64-unknown-elf qemu-system-misc ;; \
      arm)     apt-get install -y --no-install-recommends \
                 gcc-arm-linux-gnueabihf qemu-system-arm ;; \
      arm-pi1) apt-get install -y --no-install-recommends \
                 gcc-arm-linux-gnueabihf qemu-system-arm ;; \
      arm-pi1-bis) apt-get install -y --no-install-recommends \
                 gcc-arm-linux-gnueabihf qemu-system-arm ;; \
      arm-pi2) apt-get install -y --no-install-recommends \
                 gcc-arm-linux-gnueabihf qemu-system-arm ;; \
      # claude: the only fork here needing TWO toolchains: the kernel is
      # 32-bit ARM (gcc-arm-linux-gnueabihf), but armstub64.S - the
      # AArch64 firmware stub QEMU's raspi3b needs before it can enter a
      # 32-bit kernel at all - is built with gcc-aarch64-linux-gnu (see
      # forks/arm-pi3/armstub64.S's own header comment). qemu-system-arm
      # is still the right package: it ships qemu-system-aarch64 too.
      arm-pi3) apt-get install -y --no-install-recommends \
                 gcc-arm-linux-gnueabihf gcc-aarch64-linux-gnu \
                 qemu-system-arm ;; \
      # claude: ipxe-qemu (provides efi-virtio.rom) is only a Recommends
      # of qemu-system-arm, stripped by --no-install-recommends above -
      # forks/arm64's own Makefile passes "-device virtio-blk-device"
      # for its disk, and qemu-system-aarch64 needs that ROM to boot a
      # virtio device at all ("failed to find romfile efi-virtio.rom"),
      # even though it's never actually executed (this kernel never runs
      # any firmware/EFI code) - same class of gap as the python3/
      # qemu-system-gui one documented in notes_debugging_techniques.txt
      # item 6, caught the same way (a local
      # "docker build --build-arg ARCH=arm64" before touching CI).
      arm64)   apt-get install -y --no-install-recommends \
                 gcc-aarch64-linux-gnu qemu-system-arm ipxe-qemu ;; \
      # claude: qemu-system-mips (NOT qemu-system-misc, which covers
      # riscv/loongarch - see notes_arch_mips.txt's own "Toolchain"
      # section) is what actually provides qemu-system-mipsel.
      # libc6-dev-mipsel-cross (provides stdc-predef.h and friends) is
      # only a Recommends of gcc-mipsel-linux-gnu, stripped by
      # --no-install-recommends - same shape as i386's own
      # libc6-dev-i386-cross below, just easy to miss since this port's
      # own bring-up never needed a fresh apt install of the toolchain.
      # ipxe-qemu (same package as arm64's own efi-virtio.rom need
      # above) also ships efi-pcnet.rom, needed for the malta board's
      # emulated PCNET NIC even though this kernel never touches the
      # network - same "romfile" class of gap either way. seabios (also
      # only a Recommends) ships vgabios-cirrus.bin, needed for malta's
      # emulated Cirrus VGA card, same reason - this board model wires
      # up a whole legacy PC-compatible peripheral set (see
      # notes_arch_mips.txt's own "What this is" section) regardless of
      # whether this kernel's own drivers ever touch most of it.
      mips)    apt-get install -y --no-install-recommends \
                 gcc-mipsel-linux-gnu libc6-dev-mipsel-cross \
                 qemu-system-mips ipxe-qemu seabios ;; \
      # claude: gcc-14-loongarch64-linux-gnu, NOT a plain
      # "gcc-loongarch64-linux-gnu" - Ubuntu ships no unversioned
      # metapackage for this target, and the package it does ship
      # installs only "loongarch64-linux-gnu-gcc-14" with no unversioned
      # gcc symlink either (see notes_arch_loongarch.txt bug 1, and
      # ./configure's own detect_versioned_gcc, which is what makes the
      # build work anyway). qemu-system-loongarch64 comes from
      # qemu-system-misc, same package as riscv64/riscv32's.
      # xxd (its own package since noble, split out of vim-common; not
      # in the base image and NOT pulled in by build-essential) is a
      # hard build dependency unique to this port: forks/loongarch/
      # Makefile runs "xxd -i fs.img > kernel/ramdisk.h" to embed the
      # whole filesystem image into the kernel binary as a C array (see
      # notes_arch_loongarch.txt's own "structural oddity" note) -
      # without it the build fails at that rule. ipxe-qemu for
      # efi-virtio.rom, exactly the same gap as arm64's above but for a
      # subtler reason: forks/loongarch's own qemu invocation passes NO
      # virtio device at all (it has no disk - the filesystem is
      # compiled into the kernel), yet "-M virt" wires up a virtio-net
      # device unconditionally as part of the board model, and QEMU
      # refuses to start without that ROM even though this kernel never
      # touches the network. Caught only by running the real
      # "docker build --build-arg ARCH=loongarch" locally - the host
      # this port was brought up on had ipxe-qemu already installed, so
      # test-loongarch had never once needed to name this dependency.
      loongarch) apt-get install -y --no-install-recommends \
                 gcc-14-loongarch64-linux-gnu qemu-system-misc xxd \
                 ipxe-qemu ;; \
      all)     apt-get install -y --no-install-recommends \
                 gcc-riscv64-unknown-elf qemu-system-misc bc \
                 gcc-i686-linux-gnu libc6-dev-i386-cross \
                 gcc-x86-64-linux-gnu qemu-system-x86 \
                 gcc-arm-linux-gnueabihf gcc-aarch64-linux-gnu \
                 qemu-system-arm ipxe-qemu \
                 gcc-mipsel-linux-gnu libc6-dev-mipsel-cross qemu-system-mips \
                 gcc-14-loongarch64-linux-gnu xxd ;; \
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
#
# claude: ARCH=all just calls the top-level Makefile's own build-all/
# test-all umbrella targets now - they cover the exact same thirteen arches
# as this Dockerfile's own ARCH=all apt-get case above (only the order
# differs), so spelling the list out a second time here was pure
# duplication (this used to be a real, documented divergence back when
# build-all didn't yet cover arm64/loongarch - not anymore). The apt-get
# "all" case above remains the actual source of truth for which arches
# get a toolchain/qemu installed; ./configure and build-all/test-all
# themselves cover every arch wired into the top-level Makefile
# regardless of ARCH.
#
# "test-all" means the fast, boot-only "quick-test-<arch>" check across
# every arch (~1 min total - see CLAUDE.md's "Testing conventions"), not
# full usertests - full coverage for ARCH=all here would cost the ~25 min
# a real "stress-test-all" run takes, on every build of this one-image-
# does-everything convenience path. Single-ARCH builds are unaffected:
# the "else" branch below still calls "test-$ARCH" directly, which keeps
# its original full-usertests meaning - this is the exact path
# .github/workflows/docker.yml's own matrix uses (one real arch per job,
# never ARCH=all), so CI's actual coverage is untouched by this change.
RUN if [ "$ARCH" = all ]; then \
      make build-all; \
    else \
      make "build-$ARCH"; \
    fi
RUN if [ "$ARCH" = all ]; then \
      make test-all; \
    else \
      make "test-$ARCH"; \
    fi
