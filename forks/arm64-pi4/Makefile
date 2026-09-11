K=kernel
U=user
# claude: Phase 0 (docs/claude_notes/plan_factorization.md) - test programs,
# the user library and the host-built tools each get their own directory.
T=tests
L=ulib
O=tools
# claude: renamed T= -> H= to free T= for tests/ (Phase 0), consistent
# with the other four MIT-2019-shaped forks. This one is the bare-metal
# hardware-probe kernel in test/, unrelated to the userland test programs.
H=test

TOBJS = $H/entry.o $H/gpio.o $H/uart.o $H/start.o $H/main.o

OBJS = \
  $K/entry.o \
  $K/start.o \
  $K/console.o \
  $K/printf.o \
  $K/uart.o \
  $K/kalloc.o \
  $K/spinlock.o \
  $K/string.o \
  $K/main.o \
  $K/vm.o \
  $K/proc.o \
  $K/swtch.o \
  $K/trap.o \
  $K/syscall.o \
  $K/sysproc.o \
  $K/bio.o \
  $K/fs.o \
  $K/log.o \
  $K/sleeplock.o \
  $K/file.o \
  $K/pipe.o \
  $K/exec.o \
  $K/sysfile.o \
  $K/trapasm.o \
  $K/timer.o \
  $K/gicv2.o \
	$K/gpio.o \
	$K/ramdisk.o

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if aarch64-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'aarch64-unknown-elf-'; \
	elif aarch64-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'aarch64-linux-gnu-'; \
	elif aarch64-unknown-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'aarch64-unknown-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find a aarch64 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

# claude: "?=", not "=", on all four, and retargeted at MAINLINE qemu.
#
# Upstream hardcoded QEMUPREFIX to a qemu-patch-raspberry4 build
# (https://github.com/k-mrm/qemu-patch-raspberry4) in the author's own
# ~/project, and its board name "raspi4b1g" with "-m 1G". Mainline qemu
# has had a Raspberry Pi 4 board since 9.1 under the name "raspi4b", and
# models the 2GB board revision (0xb03115, hw/arm/raspi4b.c) - a raspi
# machine rejects a -m that does not match its revision, so the two go
# together. Verified booting to a shell and passing usertests on mainline
# 11.1.50; see docs/claude_notes/notes_arch_arm64_pi4.txt.
#
# So the defaults now describe the configuration that actually works with
# a stock qemu, and the patched fork is the thing you opt into:
#   make qemu QEMUPREFIX=<path>/qemu-patch-raspberry4/build/ \
#             QEMUMACHINE=raspi4b1g QEMUMEM=1G
# This repo's top-level "make run-arm64-pi4"/"make test-arm64-pi4" pass
# QEMU explicitly, from whatever ./configure detected.
QEMUPREFIX ?=
QEMU ?= $(QEMUPREFIX)qemu-system-aarch64
QEMUMACHINE ?= raspi4b
QEMUMEM ?= 2G

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

# claude: -I../../kernel/nopcs - kernel/spinlock.h is shared there
# (kernel-only, so top-level kernel/ rather than include/kernel/).
CFLAGS = -Wall -Werror -Os -fno-omit-frame-pointer -mcpu=cortex-a72+nofp -I../../include -I../../include/kernel -I../../include/user -I../../kernel/nopcs -I../../include/arch/arm64
CFLAGS += -MD -MP
CFLAGS += -ffreestanding -fno-common -nostdlib
CFLAGS += -I. -Ikernel
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
# claude: GCC 12+'s -Warray-bounds (part of -Wall) misfires on start.c's
# cpuN_wakeup(), which writes the secondary cores' entry address into the
# ARM spin-table mailboxes at the fixed low physical addresses 0xe0/0xe8/
# 0xf0 - exactly where the Pi 4 firmware parks cores 1-3 waiting for a
# release address. GCC only sees a store through a small integer constant
# cast to a pointer and reports "source object is likely at address zero";
# the addresses are deliberate and correct. Same false-positive class, and
# the same -Werror-makes-it-fatal situation, as forks/i386/Makefile's own
# -Wno-array-bounds. Confirmed via aarch64-linux-gnu-gcc 13.3.0.
CFLAGS += -Wno-array-bounds
# claude: same GCC 12+ -Winfinite-recursion false positive on user/sh.c's
# runcmd() as every other MIT-lineage xv6 fork here (see
# forks/i386/Makefile's own fix) - it's a mutually-recursive tree walk
# over "struct cmd", not real infinite recursion.
CFLAGS += -Wno-infinite-recursion

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

LDFLAGS = -z max-page-size=4096
ASFLAGS = -Og -ggdb -mcpu=cortex-a72 -MD -MP -I. -I../../include/kernel
# claude: upstream had "ASFLAGS += -DRPI4_QEMU" here, and a README step
# telling you to delete this line by hand before building for a real Pi 4.
# kernel/entry.S now reads CurrentEL at runtime instead, so one kernel8.img
# is correct on both - see that file's own comment on _entry.

$H/hwtest: $(TOBJS) $H/test.ld
	$(LD) $(LDFLAGS) -T $H/test.ld -o $H/hwtest $(TOBJS)

hwtest.img: $H/hwtest
	$(OBJCOPY) -O binary $^ $@

$K/kernel: $(OBJS) $K/kernel.ld $U/initcode fs.img
	$(LD) -r -b binary fs.img -o fs.img.o
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS) fs.img.o
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

kernel8.img: $K/kernel
	$(OBJCOPY) -O binary $^ $@

$U/initcode: $U/initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -Ikernel -c $U/initcode.S -o $U/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $U/initcode.out $U/initcode.o
	$(OBJCOPY) -S -O binary $U/initcode.out $U/initcode
	$(OBJDUMP) -S $U/initcode.o > $U/initcode.asm

tags: $(OBJS) _init
	etags *.S *.c

# claude: the portable user programs live at the repo-root utilities/ and are
# shared by every fork. An explicit pattern rule, not
# vpath: vpath matches the WHOLE prerequisite path, so "vpath %.c ../../utilities"
# would make make look for utilities/user/echo.c. The object still lands in this
# fork, so concurrent per-fork builds never race on a shared output file.
$U/%.o: ../../utilities/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$U/%.o: ../../shells/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$U/%.o: ../../kernel/init/user/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# claude: the portable test programs, shared at the repo root. usertests.c
# is only partly among them - most forks' own copy is genuinely per-port
# (each with its own skipped sub-tests) and stays local, but this fork's
# copy was close enough to some others to share - see the explicit rule
# below and tests/usertests-arm64.c's own header comment for which ones.
tests/%.o: ../../tests/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# claude: explicit rule, not covered by the generic pattern above - the
# shared file's name doesn't match usertests.o's expected usertests.c.
tests/usertests.o: ../../tests/usertests-arm64.c
	$(CC) $(CFLAGS) -c -o $@ $<

# claude: the shared user library, under lib_core/ per ~/principia's
# convention. Separate rule because these objects land in $L/, not
# alongside the programs.
$L/%.o: ../../lib_core/libc/ulib/%.c
	$(CC) $(CFLAGS) -c -o $@ $<


ULIB = $L/arith.o $L/ulib.o $L/usys.o $L/printf.o $L/umalloc.o

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

$L/usys.S : $L/usys.pl
	perl $L/usys.pl > $L/usys.S

$L/usys.o : $L/usys.S
	$(CC) $(CFLAGS) -c -o $L/usys.o $L/usys.S

$T/_forktest: $T/forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $T/_forktest $T/forktest.o $L/ulib.o $L/usys.o
	$(OBJDUMP) -S $T/_forktest > $T/forktest.asm

# claude: "@mkdir -p $O" - tools/ has no tracked file of its own any more
# (mkfs.c moved to the shared ../../tools/mkfs.c), so a clean checkout (CI,
# docker build) never creates it; only a stale local build tree happens to
# have it already. Same bug class forks/i386's own user/%.o mkdir-p fix
# describes - a real CI regression this exact change caused.
$O/mkfs: ../../tools/mkfs.c ../../include/kernel/fs.h $K/conf.h $K/param.h
	@mkdir -p $O
	gcc -Werror -Wall -I. -idirafter kernel -idirafter ../../include/kernel -o $O/mkfs ../../tools/mkfs.c -I../../include -I../../include/user -I../../include/arch/arm64

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

UPROGS=\
	$U/_cat\
	$U/_echo\
	$T/_forktest\
	$U/_grep\
	$U/_init\
	$U/_kill\
	$U/_ln\
	$U/_ls\
	$U/_mkdir\
	$U/_rm\
	$U/_sh\
	$T/_stressfs\
	$T/_usertests\
	$T/_grind\
	$U/_wc\
	$T/_zombie\

fs.img: $O/mkfs README $(UPROGS)
	$O/mkfs fs.img README $(UPROGS)

-include kernel/*.d user/*.d tests/*.d ulib/*.d

clean: 
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*/*.o */*.d */*.asm */*.sym *.img */*.img *.o \
	$U/initcode $U/initcode.out $K/kernel fs.img $H/hwtest \
	$O/mkfs .gdbinit \
        $L/usys.S \
	$(UPROGS)

SDPATH = /media/k-mrm/BOOT/

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 4
endif

QEMUOPTS = -cpu cortex-a72 -machine $(QEMUMACHINE) -kernel $K/kernel -m $(QEMUMEM) -smp $(CPUS) -nographic
QEMUTOPTS = -cpu cortex-a72 -machine $(QEMUMACHINE) -kernel $H/hwtest -m $(QEMUMEM) -smp $(CPUS) -nographic

qemu: $K/kernel
	$(QEMU) $(QEMUOPTS)

qemu-test: hwtest.img
	$(QEMU) $(QEMUTOPTS)

rpi4: kernel8.img
	cp kernel8.img $(SDPATH)

dts: kernel8.img
	$(QEMU) -cpu cortex-a72 -machine $(QEMUMACHINE),dumpdtb=a.dtb -kernel $K/kernel -m $(QEMUMEM) -smp $(CPUS) -nographic
	dtc -I dtb -O dts -o a.dts a.dtb
	$(RM) a.dtb

test: hwtest.img
	cp hwtest.img $(SDPATH)kernel8.img	

.gdbinit: .gdbinit.tmpl-aarch64
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

.gdbinit-test: .gdbinit.tmpl-aarch64-test
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

qemu-gdb: $K/kernel .gdbinit fs.img
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

test-gdb: hwtest.img .gdbinit-test
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUTOPTS) -S $(QEMUGDB)

