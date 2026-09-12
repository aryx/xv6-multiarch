K=kernel
U=user
# claude: Phase 0 (docs/claude_notes/plan_factorization.md) - test programs,
# the user library and the host-built tools each get their own directory.
T=tests
L=ulib
O=tools

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
  $K/virtio_disk.o \
  $K/gicv3.o \

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

#QEMUPREFIX = ~/qemu/build/
QEMU = $(QEMUPREFIX)qemu-system-aarch64

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

# claude: -I../../kernel/nopcs - kernel/spinlock.h is shared there
# (kernel-only, so top-level kernel/ rather than include/kernel/).
CFLAGS = -Wall -Werror -Os -g -fno-omit-frame-pointer -mcpu=cortex-a72+nofp -I../../include -I../../include/kernel -I../../include/user -I../../kernel/nopcs -I../../kernel/arch/arm64 -I../../include/arch/arm64
CFLAGS += -MD
CFLAGS += -ffreestanding -fno-common -nostdlib
CFLAGS += -I. -Ikernel
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

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
ASFLAGS = -Og -ggdb -mcpu=cortex-a72 -MD -I. -I../../include/kernel

$K/kernel: $(OBJS) $K/kernel.ld $U/initcode
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS) 
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

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

# claude: -T $L/user.ld (matching forks/riscv64's own rule, ported here as
# user/user.ld) instead of the original "-N -e main -Ttext 0" - the latter
# packs .text/.data with no page-alignment between them, which
# kernel/exec.c's own ph.vaddr%PGSIZE==0 check rejects at runtime (see
# notes_arch_riscv32.txt's "bug 2", the same issue in the rv32 sibling fork).
# forktest below deliberately keeps the old style.
_%: %.o $(ULIB) $L/user.ld
	$(LD) $(LDFLAGS) -T $L/user.ld -o $@ $< $(ULIB)
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
	*/*.o */*.d */*.asm */*.sym \
	$U/initcode $U/initcode.out $K/kernel fs.img \
	$O/mkfs .gdbinit \
        $L/usys.S \
	$(UPROGS)

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 4
endif

QEMUOPTS = -cpu cortex-a72 -machine virt,gic-version=3 -kernel $K/kernel -m 128M -smp $(CPUS) -nographic
# claude: cache=unsafe - fs.img is regenerated by mkfs on every build and
# thrown away afterward, so there's no durability requirement to protect
# here (unlike a real flashed board). See
# docs/claude_notes/plan_test_speed.md.
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0,cache=unsafe
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

qemu: $K/kernel fs.img
	$(QEMU) $(QEMUOPTS)

.gdbinit: .gdbinit.tmpl-aarch64
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

qemu-gdb: $K/kernel .gdbinit fs.img
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

