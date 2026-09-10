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
  $K/trampoline.o \
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
  $K/kernelvec.o \
  $K/plic.o \
  $K/virtio_disk.o

# riscv32-unknown-elf- or riscv32-linux-gnu-
# perhaps in /opt/riscv/bin
TOOLPREFIX = riscv32-unknown-elf-

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if riscv32-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv32-unknown-elf-'; \
	elif riscv32-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv32-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an riscv32 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

QEMU = qemu-system-riscv32 -monitor telnet:127.0.0.1:55555,server,nowait 

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb
CFLAGS += -MD
# claude: this repo has no dedicated riscv32-unknown-elf- package (Ubuntu's
# gcc-riscv64-unknown-elf is multilib and defaults to rv64/lp64d) - without
# an explicit -march/-mabi here the TOOLPREFIX detection above falls through
# to riscv64-unknown-elf- (see xv6-multiarch's own ./configure) and silently
# builds 64-bit code, which this kernel's 32-bit pointer/register assumptions
# (see kernel.ld's own comments, virtio_disk.c) do not tolerate. README.md
# documents this port as "rv32ia" - the spinlock.c amoswap.w atomic requires
# the 'a' extension; "m"/"c" added to match ~/c--/configure's own already-
# verified "riscv64-unknown-elf-gcc -march=rv32imac -mabi=ilp32" bare-metal
# combo for this same multilib toolchain (its -riscv32 backend), rather than
# risk GCC emitting soft-multiply calls (__mulsi3 etc.) with nothing to
# link them against under -nostdlib. _zicsr_zifencei: newer binutils (2.38+,
# per the RISC-V ISA manual's own 20191213 spec split) no longer folds the
# CSR/fence.i instructions this kernel's trap/mtvec/satp setup uses into the
# base "I" extension - they're their own named extensions now, so they must
# be listed explicitly or the assembler rejects "csrr"/"csrw"/"fence.i" as
# unrecognized (c--'s own -riscv32 backend never touches CSRs, so it never
# needed this).
CFLAGS += -march=rv32ima_zicsr_zifencei -mabi=ilp32
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

# claude: entry.S/swtch.S/trampoline.S/kernelvec.S build via make's builtin
# implicit .S.o rule ($(CC) $(ASFLAGS) ... -c), which this Makefile never
# overrode - so with ASFLAGS empty they were assembled at the toolchain's
# 64-bit default (elf64-littleriscv) even though every .c file above got
# -march=rv32imac...-mabi=ilp32 via CFLAGS, causing a 32-vs-64-bit ELF class
# mismatch at final link ("file class ELFCLASS64 incompatible with
# ELFCLASS32"). Reusing CFLAGS here keeps the two in sync.
ASFLAGS = $(CFLAGS)

# claude: same GCC 12+ -Winfinite-recursion false positive on user/sh.c's
# runcmd() as every other MIT-lineage xv6 fork here (see
# forks/i386/Makefile's own fix and notes_arch_i386.txt's first writeup) -
# it's a mutually-recursive tree walk over "struct cmd", not real infinite
# recursion.
CFLAGS += -Wno-infinite-recursion

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

# claude: riscv64-unknown-elf-ld defaults its emulation to 64-bit
# (elf64lriscv) regardless of the actual object files' ELF class - a
# dedicated riscv32-unknown-elf- toolchain would default to elf32lriscv
# instead, but this repo only has the riscv64 multilib package (see the
# CFLAGS -march/-mabi comment above), so the 32-bit emulation must be
# selected explicitly or every link fails with "ABI is incompatible with
# that of the selected emulation".
LDFLAGS = -m elf32lriscv -z max-page-size=4096

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

# claude: the shared user library, under lib_core/ per ~/principia's
# convention. Separate rule because these objects land in $L/, not
# alongside the programs.
$L/%.o: ../../lib_core/libc/ulib/%.c
	$(CC) $(CFLAGS) -c -o $@ $<


ULIB = $L/arith.o $L/ulib.o $L/usys.o $L/printf.o $L/umalloc.o

# claude: -T $L/user.ld (matching forks/riscv64's own rule, ported here as
# user/user.ld) instead of the original "-N -e main -Ttext 0" - the latter
# packs .text/.data with no page-alignment between them, which
# kernel/exec.c's own ph.vaddr%PGSIZE==0 check rejects at runtime for any
# program whose .text doesn't happen to end exactly on a page boundary
# (every real UPROG here). forktest below deliberately keeps the old style.
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

$O/mkfs: $O/mkfs.c $K/fs.h
	gcc -Werror -Wall -I. -o $O/mkfs $O/mkfs.c

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
CPUS := 3
endif

QEMUEXTRA = -drive file=fs1.img,if=none,format=raw,id=x1 -device virtio-blk-device,drive=x1,bus=virtio-mmio-bus.1
QEMUOPTS = -machine virt -bios none -kernel $K/kernel -m 1024M -smp $(CPUS) -nographic
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

qemu: $K/kernel fs.img
	$(QEMU) $(QEMUOPTS)

.gdbinit: .gdbinit.tmpl-riscv
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

qemu-gdb: $K/kernel .gdbinit fs.img
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

# CUT HERE
# prepare dist for students
# after running make dist, probably want to
# rename it to rev0 or rev1 or so on and then
# check in that version.

EXTRA=\
	mkfs.c ulib.c user.h cat.c echo.c forktest.c grep.c kill.c\
	ln.c ls.c mkdir.c rm.c stressfs.c usertests.c wc.c zombie.c\
	printf.c umalloc.c\
	README dot-bochsrc *.pl \
	.gdbinit.tmpl gdbutil\

dist:
	rm -rf dist
	mkdir dist
	for i in $(FILES); \
	do \
		grep -v PAGEBREAK $$i >dist/$$i; \
	done
	sed '/CUT HERE/,$$d' Makefile >dist/Makefile
	echo >dist/runoff.spec
	cp $(EXTRA) dist

dist-test:
	rm -rf dist
	make dist
	rm -rf dist-test
	mkdir dist-test
	cp dist/* dist-test
	cd dist-test; $(MAKE) print
	cd dist-test; $(MAKE) bochs || true
	cd dist-test; $(MAKE) qemu

# update this rule (change rev#) when it is time to
# make a new revision.
tar:
	rm -rf /tmp/xv6
	mkdir -p /tmp/xv6
	cp dist/* dist/.gdbinit.tmpl /tmp/xv6
	(cd /tmp; tar cf - xv6) | gzip >xv6-rev10.tar.gz  # the next one will be 10 (9/17)

.PHONY: dist-test dist
