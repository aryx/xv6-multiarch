K=kernel
U=user
# claude: Phase 0 (docs/claude_notes/plan_factorization.md) - test programs,
# the user library and the host-built tools each get their own directory.
T=tests
L=ulib
O=tools

OBJS = \
  $K/entry.o \
  $K/main.o \
  $K/uart.o \
  $K/printf.o \
  $K/proc.o \
  $K/spinlock.o \
  $K/string.o \
  $K/swtch.o \
  $K/console.o \
  $K/sleeplock.o \
  $K/file.o \
  $K/kalloc.o\
  $K/vm.o\
  $K/trap.o\
  $K/kernelvec.o\
  $K/tlbrefill.o\
  $K/merror.o\
  $K/apic.o\
  $K/extioi.o\
  $K/ramdisk.o\
  $K/bio.o\
  $K/log.o\
  $K/fs.o\
  $K/pipe.o\
  $K/exec.o\
  $K/syscall.o\
  $K/sysproc.o\
  $K/sysfile.o\
  $K/uservec.o

UNAME_M=$(shell uname -m)
ifeq ($(findstring loongarch64,$(UNAME_M)),loongarch64)
TOOLPREFIX ?=
else
# claude: was a plain "=" here, which (unlike every other fork's own
# TOOLPREFIX auto-detect - see e.g. forks/riscv64/Makefile's own "ifndef
# TOOLPREFIX" block) silently overrides a TOOLPREFIX already set in the
# environment or passed on the command line before this line runs -
# "?=" makes this fork's own default just that, a default, consistent
# with every other fork and with how the top-level Makefile/test-xv6.py
# override it (see notes_arch_loongarch.txt).
TOOLPREFIX ?= loongarch64-unknown-linux-gnu-
endif

# claude: this toolchain's gcc is version-suffixed only (e.g.
# loongarch64-linux-gnu-gcc-14, no plain "...-gcc" - see
# notes_arch_loongarch.txt), so "$(TOOLPREFIX)gcc" is wrong on this
# specific host/toolchain combination; "?=" (not "=") lets an external
# CC=... override (./configure's CC_LOONGARCH, threaded through by the
# top-level Makefile and this fork's own test-xv6.py) supply the right
# binary name without editing this file per-host.
CC ?= $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

ASFLAGS = -march=loongarch64 -mabi=lp64s
CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb -I../../include -I../../include/kernel -I../../include/user
CFLAGS += -MD -MP
CFLAGS += -march=loongarch64 -mabi=lp64s
CFLAGS += -ffreestanding -fno-common -nostdlib
CFLAGS += -I. -fno-stack-protector
CFLAGS += -fno-pie -no-pie
LDFLAGS = -z max-page-size=4096

$K/kernel: $(OBJS) $K/kernel.ld $U/initcode
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS)
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

$U/initcode: $U/initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -Ikernel -c $U/initcode.S -o $U/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $U/initcode.out $U/initcode.o
	# claude: initcode.S's "la" pseudo-ops make the LoongArch assembler
	# emit a GOT-relocation relic (.rela.got, ALLOC'd at a huge synthetic
	# address around 0x1200000b0 - readelf -l shows it as its own PT_LOAD
	# segment, unrelated to .text's real 0x0 load address) even though
	# this is a fully static, no-dynamic-linker blob: the linker already
	# resolves the two real symbol references (init/argv) directly into
	# .text, and the kernel boots/passes usertests either way - .rela.got
	# is dead weight. "objcopy -O binary" zero-fills the entire gap
	# between the lowest and highest surviving section, so without "-R
	# .rela.got" this silently produces a multi-GB sparse file instead of
	# the few-hundred-byte blob this actually is - same class of bug as
	# forks/mips's .reginfo/.MIPS.abiflags one, see notes_arch_mips.txt
	# bug 4 and notes_arch_loongarch.txt.
	$(OBJCOPY) -S -R .rela.got -O binary $U/initcode.out $U/initcode
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

SH_FLAGS = -O -fno-omit-frame-pointer -ggdb -MD -MP -march=loongarch64 -mabi=lp64s -ffreestanding -fno-common -nostdlib -I. -I../../include -I../../include/user -fno-stack-protector -fno-pie -no-pie -c -o

# claude: sh.c is shared from the repo-root shells/ now. This fork compiles it
# with its own SH_FLAGS rather than CFLAGS, so it needs its own rule updating -
# the generic $U/%.o: ../../shells/%.c rule above never gets a look in.
$U/_sh: ../../shells/sh.c $(ULIB)
	$(CC) $(SH_FLAGS) $U/sh.o ../../shells/sh.c
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_sh $U/sh.o $(ULIB)
	$(OBJDUMP) -S $U/_sh > $U/sh.asm

# claude: "@mkdir -p $O" - tools/ has no tracked file of its own any more
# (mkfs.c moved to the shared ../../tools/mkfs.c), so a clean checkout (CI,
# docker build) never creates it; only a stale local build tree happens to
# have it already. Same bug class forks/i386's own user/%.o mkdir-p fix
# describes - a real CI regression this exact change caused.
$O/mkfs: ../../tools/mkfs.c $K/fs.h $K/param.h
	@mkdir -p $O
	gcc -Werror -Wall -I. -o $O/mkfs ../../tools/mkfs.c -I../../include -I../../include/user

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
#	$T/_grind\
	$U/_wc\
	$T/_zombie\

fs.img: $O/mkfs README $(UPROGS)
	$O/mkfs fs.img README $(UPROGS)
	xxd -i fs.img > kernel/ramdisk.h

-include kernel/*.d user/*.d tests/*.d ulib/*.d

all: fs.img $K/kernel

# claude: upstream never checked in a working run target - the only trace
# of one is qemu-loongarch-runenv/ (an empty directory) and test.sh (which
# cd's into "qemu-loongarch64-runenv", a typo for the actual directory
# name, so it was never run successfully as shipped either). "-M virt" is
# QEMU's Loongson-3A5000/LS7A1000 board - the entry.S/memlayout.h comments
# describe a BIOS that loads the kernel raw at physical 0x00200000 and
# jumps to it, and kernel.ld links at 0x9000000000200000 (that physical
# address's DMW0 direct-mapped-window alias) - this works as-is with
# QEMU's ELF loader with no code changes; see notes_arch_loongarch.txt.
# claude: QEMU's loongarch64 "virt" machine refuses less than 1G of RAM
# ("ram_size must be greater than 1G") even though this kernel's own
# memlayout.h only actually uses 128MB of it (RAMBASE..RAMSTOP) - the
# 1G floor is a board-model constraint, not something this kernel needs.
QEMU = qemu-system-loongarch64
QEMUOPTS = -M virt -kernel $K/kernel -smp 1 -m 1G -nographic

qemu: all
	$(QEMU) $(QEMUOPTS)

clean:
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*/*.o */*.d */*.asm */*.sym \
	$U/initcode $U/initcode.out $K/kernel fs.img \
	$O/mkfs .gdbinit \
        $L/usys.S \
	$(UPROGS)




