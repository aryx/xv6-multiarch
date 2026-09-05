# Build and boot xv6-multiarch's riscv64 and i386 ports on Ubuntu Linux,
# pinning the toolchain/QEMU this repo's own bring-up was verified against
# (see docs/claude_notes/notes_arch_riscv64.txt and notes_arch_i386.txt) so
# a second machine or CI reproduces them exactly, instead of "whatever apt
# happens to resolve today". This is Phase 3 of
# docs/claude_notes/build-and-test-plan.md.
#
# claude: modeled on ~/c--/Dockerfile and ~/goken/Dockerfile's own shape
# (apt-get update, install the cross toolchains, COPY the source, run
# ./configure, build, then test) - see those files' own header comments
# for the general reasoning this one reuses. Simpler here: only two arches
# are wired up so far (riscv64, i386 - build-and-test-plan.md's Phases 1
# and 2), not all thirteen forks/ - extend the two RUN apt-get install
# lines below and the build/test lines at the bottom as more arches get
# their own ./configure detection, matching build.md's own Phase 4 order.
#
# ubuntu:24.04 to match the dev machine the notes_arch_*.txt files record
# toolchain/qemu versions against - not pinned for any of c--'s own
# reasons (no prebuilt checked-in cross-arch objects here to match), just
# consistency with what was actually verified.
FROM ubuntu:24.04

RUN apt-get update # needed otherwise can't find any package

# make (this Dockerfile's own build-all/test-all below) and a host gcc -
# both forks/riscv/Makefile's and forks/x86/Makefile's own "mkfs" rules
# compile that HOST tool with plain "gcc", not the cross TOOLPREFIX one,
# since it runs here rather than under QEMU. Neither is in the base image.
RUN apt-get install -y build-essential

# The riscv64 toolchain (forks/riscv, RV64GC) and the i386 toolchain
# (forks/x86) ./configure detects - see its own header comment for why
# i686-linux-gnu- specifically (Ubuntu ships no i386-jos-elf- package,
# the name forks/x86/Makefile's own auto-detect expects, and
# forks/x86/Makefile's own probe doesn't know about i686-linux-gnu-
# either - notes_arch_i386.txt's own "toolchain" section has the detail).
RUN apt-get install -y \
      gcc-riscv64-unknown-elf \
      gcc-i686-linux-gnu libc6-dev-i386-cross

# qemu-system-misc: the FULL-SYSTEM emulator for riscv64
# (qemu-system-riscv64) - a separate package from the more common
# qemu-system-x86/-arm, see notes_arch_riscv64.txt's own "what was
# missing" section for why this isn't just "qemu" or "qemu-user".
# qemu-system-x86: qemu-system-i386, for forks/x86.
# bc: forks/riscv/Makefile's own check-qemu-version target needs it
# directly (not ./configure's own version check, which deliberately
# avoids bc - see notes_arch_riscv64.txt's "PATH hazard" section for why).
RUN apt-get install -y qemu-system-misc qemu-system-x86 bc

WORKDIR /src
COPY . .

# Detect the toolchains/qemu installed above and write Makefile.config -
# see ./configure's own header comment. .dockerignore drops the working
# tree's own Makefile.config (if any) so this always regenerates fresh
# inside the image rather than reusing a host-detected one.
RUN ./configure

# Build both arches.
RUN make build-all

# Boot each under QEMU (software emulation via TCG - no /dev/kvm needed
# or used, same as this repo's own bring-up on an aarch64 host emulating
# both riscv64 and i386) and run its own usertests suite end to end - see
# forks/riscv/test-xv6.py and forks/x86/test-xv6.py.
RUN make test-all
