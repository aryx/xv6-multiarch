OBJS = \
	kernel/bio.o\
	kernel/console.o\
	kernel/exec.o\
	kernel/file.o\
	kernel/fs.o\
	kernel/ide.o\
	kernel/ioapic.o\
	kernel/kalloc.o\
	kernel/kbd.o\
	kernel/lapic.o\
	kernel/log.o\
	kernel/main.o\
	kernel/mp.o\
	kernel/picirq.o\
	kernel/pipe.o\
	kernel/proc.o\
	kernel/sleeplock.o\
	kernel/spinlock.o\
	kernel/string.o\
	kernel/swtch.o\
	kernel/syscall.o\
	kernel/sysfile.o\
	kernel/sysproc.o\
	kernel/trapasm.o\
	kernel/trap.o\
	kernel/uart.o\
	kernel/vectors.o\
	kernel/vm.o\

# Cross-compiling (e.g., on Mac OS X)
# TOOLPREFIX = i386-jos-elf

# Using native tools (e.g., on X86 Linux)
#TOOLPREFIX = 

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if i386-jos-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
	then echo 'i386-jos-elf-'; \
	elif objdump -i 2>&1 | grep 'elf32-i386' >/dev/null 2>&1; \
	then echo ''; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an i386-*-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with i386-jos-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your i386-*-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'i386-jos-elf-', set your TOOLPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

# If the makefile can't find QEMU, specify its path here
# QEMU = qemu-system-i386

# Try to infer the correct QEMU
ifndef QEMU
QEMU = $(shell if which qemu > /dev/null; \
	then echo qemu; exit; \
	elif which qemu-system-i386 > /dev/null; \
	then echo qemu-system-i386; exit; \
	elif which qemu-system-x86_64 > /dev/null; \
	then echo qemu-system-x86_64; exit; \
	else \
	qemu=/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu; \
	if test -x $$qemu; then echo $$qemu; exit; fi; fi; \
	echo "***" 1>&2; \
	echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
	echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
	echo "*** or have you tried setting the QEMU variable in Makefile?" 1>&2; \
	echo "***" 1>&2; exit 1)
endif

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
# claude: -I. is the FORK ROOT, so the shared programs in the repo-root
# utilities/ can spell their includes "kernel/types.h" / "user/user.h" the
# way mit-pdos/xv6-riscv does. It must go on this unconditional line: an
# earlier attempt appended it to the last CFLAGS line, which in several
# forks sits inside an ifeq branch that is not taken.
CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -ggdb -m32 -Werror -fno-omit-frame-pointer -Ikernel -Iuser -I. -I../../include -I../../include/user
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
# claude: GCC 10+'s -Warray-bounds (part of -Wall) misfires on mp.c's BIOS-
# memory pointer casts (mpconfig()/mpinit() treat a small physical address
# as if it indexed a huge on-stack array) - a well-known false-positive
# class for freestanding kernel code on modern GCC, fatal here only
# because of -Werror above. Confirmed via i686-linux-gnu-gcc 13.3.0.
CFLAGS += -Wno-array-bounds
# claude: GCC 12+'s -Winfinite-recursion (also part of -Wall) misfires on
# sh.c's runcmd(), a genuinely-terminating recursive-descent interpreter
# whose base cases (exec()/exit() on the leaf command types) the
# heuristic can't see through - confirmed it actually runs fine under
# qemu. Same -Werror-makes-it-fatal situation as -Warray-bounds above.
CFLAGS += -Wno-infinite-recursion
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide -Ikernel
# FreeBSD ld wants ``elf_i386_fbsd''
LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

xv6.img: bootblock kernel/kernel
	dd if=/dev/zero of=xv6.img count=10000
	dd if=bootblock of=xv6.img conv=notrunc
	dd if=kernel/kernel of=xv6.img seek=1 conv=notrunc

xv6memfs.img: bootblock kernel/kernelmemfs
	dd if=/dev/zero of=xv6memfs.img count=10000
	dd if=bootblock of=xv6memfs.img conv=notrunc
	dd if=kernel/kernelmemfs of=xv6memfs.img seek=1 conv=notrunc

bootblock: kernel/bootasm.S kernel/bootmain.c
	$(CC) $(CFLAGS) -fno-pic -O -nostdinc -Ikernel -c kernel/bootmain.c
	$(CC) $(CFLAGS) -fno-pic -nostdinc -Ikernel -c kernel/bootasm.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o bootblock.o bootasm.o bootmain.o
	$(OBJDUMP) -S bootblock.o > bootblock.asm
	$(OBJCOPY) -S -O binary -j .text bootblock.o bootblock
	./tools/sign.pl bootblock

# claude: the generated "entryother" and "initcode" blobs stay at the fork
# root even though their .S sources moved into kernel/. "ld -b binary"
# derives its synthetic symbols from the FILE PATH it is given, so
# kernel/entryother would yield _binary_kernel_entryother_start while
# main.c reads _binary_entryother_start. Only the sources move; the output
# paths are load-bearing. See plan_factorization.md's gotcha 3.
entryother: kernel/entryother.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -Ikernel -c kernel/entryother.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o bootblockother.o entryother.o
	$(OBJCOPY) -S -O binary -j .text bootblockother.o entryother
	$(OBJDUMP) -S bootblockother.o > entryother.asm

initcode: kernel/initcode.S
	$(CC) $(CFLAGS) -nostdinc -Ikernel -c kernel/initcode.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
	$(OBJCOPY) -S -O binary initcode.out initcode
	$(OBJDUMP) -S initcode.o > initcode.asm

# claude: target renamed "kernel" -> "kernel/kernel" (kernelmemfs likewise
# below) - a make target cannot share a name with an existing directory:
# kernel/ exists, so make would treat the target as already built and never
# relink. Where forks/riscv64 already puts it.
kernel/kernel: $(OBJS) kernel/entry.o entryother initcode kernel/kernel.ld
	$(LD) $(LDFLAGS) -T kernel/kernel.ld -o kernel/kernel kernel/entry.o $(OBJS) -b binary initcode entryother
	$(OBJDUMP) -S kernel/kernel > kernel/kernel.asm
	$(OBJDUMP) -t kernel/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel/kernel.sym

# kernelmemfs is a copy of kernel that maintains the
# disk image in memory instead of writing to a disk.
# This is not so useful for testing persistent storage or
# exploring disk buffering implementations, but it is
# great for testing the kernel on real hardware without
# needing a scratch disk.
MEMFSOBJS = $(filter-out kernel/ide.o,$(OBJS)) kernel/memide.o
kernel/kernelmemfs: $(MEMFSOBJS) kernel/entry.o entryother initcode kernel/kernel.ld fs.img
	$(LD) $(LDFLAGS) -T kernel/kernel.ld -o kernel/kernelmemfs kernel/entry.o  $(MEMFSOBJS) -b binary initcode entryother fs.img
	$(OBJDUMP) -S kernel/kernelmemfs > kernel/kernelmemfs.asm
	$(OBJDUMP) -t kernel/kernelmemfs | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel/kernelmemfs.sym

tags: $(OBJS) kernel/entryother.S user/_init
	etags kernel/*.S kernel/*.c user/*.c tests/*.c ulib/*.c ulib/*.S

kernel/vectors.S: tools/vectors.pl
	./tools/vectors.pl > kernel/vectors.S

# claude: the portable user programs live at the repo-root utilities/ and are
# shared by every fork. An explicit pattern rule, not
# vpath: vpath matches the WHOLE prerequisite path, so "vpath %.c ../../utilities"
# would make make look for utilities/user/echo.c. The object still lands in this
# fork, so concurrent per-fork builds never race on a shared output file.
user/%.o: ../../utilities/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

user/%.o: ../../shells/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# claude: the shared user library, under lib_core/ per ~/principia's
# convention. Separate rule because these objects land in ulib/, not
# alongside the programs.
ulib/%.o: ../../lib_core/libc/ulib/%.c
	$(CC) $(CFLAGS) -c -o $@ $<


ULIB = ulib/arith.o ulib/ulib.o ulib/usys.o ulib/printf.o ulib/umalloc.o

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

tests/_forktest: tests/forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o tests/_forktest tests/forktest.o ulib/ulib.o ulib/usys.o
	$(OBJDUMP) -S tests/_forktest > tests/forktest.asm

# claude: -idirafter, NOT -I. mkfs is a HOST program: it includes the
# system <fcntl.h> as well as xv6's own "fs.h"/"types.h". -Ikernel is
# searched before the system directories, so <fcntl.h> would resolve to
# xv6's kernel/fcntl.h - which defines O_RDONLY/O_WRONLY/O_RDWR/O_CREATE
# and no O_TRUNC - and the build fails with "'O_TRUNC' undeclared".
# -idirafter puts kernel/ after the system directories instead, so each
# header comes from where it should. Same idiom forks/mips already used.
tools/mkfs: tools/mkfs.c kernel/fs.h
	gcc -Werror -Wall -idirafter kernel -o tools/mkfs tools/mkfs.c -I../../include -I../../include/user

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

UPROGS=\
	user/_cat\
	user/_echo\
	tests/_forktest\
	user/_grep\
	user/_init\
	user/_kill\
	user/_ln\
	user/_ls\
	user/_mkdir\
	user/_rm\
	user/_sh\
	tests/_stressfs\
	tests/_usertests\
	user/_wc\
	tests/_zombie\

fs.img: tools/mkfs README $(UPROGS)
	./tools/mkfs fs.img README $(UPROGS)

-include kernel/*.d user/*.d tests/*.d ulib/*.d

clean: 
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*.o *.d *.asm *.sym bootblock entryother \
	initcode initcode.out xv6.img fs.img \
	xv6memfs.img tools/mkfs .gdbinit \
	kernel/vectors.S kernel/kernel kernel/kernelmemfs \
	kernel/*.o kernel/*.d kernel/*.asm kernel/*.sym \
	ulib/*.o ulib/*.d \
	user/*.o user/*.d user/*.asm user/*.sym \
	tests/*.o tests/*.d tests/*.asm tests/*.sym \
	$(UPROGS)

# make a printout
FILES = $(shell grep -v '^\#' runoff.list)
PRINT = runoff.list runoff.spec README toc.hdr toc.ftr $(FILES)

xv6.pdf: $(PRINT)
	./runoff
	ls -l xv6.pdf

print: xv6.pdf

# run in emulators

bochs : fs.img xv6.img
	if [ ! -e .bochsrc ]; then ln -s dot-bochsrc .bochsrc; fi
	bochs -q

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 2
endif
QEMUOPTS = -drive file=fs.img,index=1,media=disk,format=raw -drive file=xv6.img,index=0,media=disk,format=raw -smp $(CPUS) -m 512 $(QEMUEXTRA)

qemu: fs.img xv6.img
	$(QEMU) -serial mon:stdio $(QEMUOPTS)

qemu-memfs: xv6memfs.img
	$(QEMU) -drive file=xv6memfs.img,index=0,media=disk,format=raw -smp $(CPUS) -m 256

qemu-nox: fs.img xv6.img
	$(QEMU) -nographic $(QEMUOPTS)

.gdbinit: tools/.gdbinit.tmpl
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

qemu-gdb: fs.img xv6.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB)

qemu-nox-gdb: fs.img xv6.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -nographic $(QEMUOPTS) -S $(QEMUGDB)

# CUT HERE
# prepare dist for students
# after running make dist, probably want to
# rename it to rev0 or rev1 or so on and then
# check in that version.

EXTRA=\
	mkfs.c ulib.c user.h cat.c echo.c forktest.c grep.c kill.c\
	ln.c ls.c mkdir.c rm.c stressfs.c usertests.c wc.c zombie.c\
	printf.c umalloc.c\
	README dot-bochsrc *.pl toc.* runoff runoff1 runoff.list\
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
