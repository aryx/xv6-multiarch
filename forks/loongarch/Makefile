K=kernel
U=user

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
CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb
CFLAGS += -MD
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

ULIB = $U/ulib.o $U/usys.o $U/printf.o $U/umalloc.o

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

$U/usys.S : $U/usys.pl
	perl $U/usys.pl > $U/usys.S

$U/usys.o : $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S

$U/_forktest: $U/forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_forktest $U/forktest.o $U/ulib.o $U/usys.o
	$(OBJDUMP) -S $U/_forktest > $U/forktest.asm

SH_FLAGS = -O -fno-omit-frame-pointer -ggdb -MD -march=loongarch64 -mabi=lp64s -ffreestanding -fno-common -nostdlib -I. -fno-stack-protector -fno-pie -no-pie -c -o

$U/_sh: $U/sh.c $(ULIB)
	$(CC) $(SH_FLAGS) $U/sh.o $U/sh.c
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_sh $U/sh.o $(ULIB)
	$(OBJDUMP) -S $U/_sh > $U/sh.asm

mkfs/mkfs: mkfs/mkfs.c $K/fs.h $K/param.h
	gcc -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

UPROGS=\
	$U/_cat\
	$U/_echo\
	$U/_forktest\
	$U/_grep\
	$U/_init\
	$U/_kill\
	$U/_ln\
	$U/_ls\
	$U/_mkdir\
	$U/_rm\
	$U/_sh\
	$U/_stressfs\
	$U/_usertests\
#	$U/_grind\
	$U/_wc\
	$U/_zombie\

fs.img: mkfs/mkfs README $(UPROGS)
	mkfs/mkfs fs.img README $(UPROGS)
	xxd -i fs.img > kernel/ramdisk.h

-include kernel/*.d user/*.d

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
	mkfs/mkfs .gdbinit \
        $U/usys.S \
	$(UPROGS)




