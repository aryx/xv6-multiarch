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
QEMU = qemu-system-x86_64

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

XFLAGS = -m64 -mcmodel=large -ggdb
# CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -ggdb -Werror -fno-omit-frame-pointer
# claude: -I. is the FORK ROOT, so the shared programs in the repo-root
# utilities/ can spell their includes "kernel/types.h" / "user/user.h" the
# way mit-pdos/xv6-riscv does. It must go on this unconditional line: an
# earlier attempt appended it to the last CFLAGS line, which in several
# forks sits inside an ifeq branch that is not taken.
# claude: -I../../kernel - kernel/spinlock.h is shared there (kernel-only,
# so top-level kernel/ rather than include/kernel/).
CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -Wall -MD -ggdb -Werror -fno-omit-frame-pointer -I. -I../../include -I../../include/kernel -I../../include/user -I../../kernel
CFLAGS += -ffreestanding -fno-common -nostdlib $(XFLAGS) -Ikernel -Iuser
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
# claude: GCC 12+'s -Winfinite-recursion (part of -Wall) misfires on
# sh.c's runcmd() - same recursive-descent shell interpreter every
# other xv6-public-descended fork here shares, see forks/i386/Makefile's
# own identical fix and comment for the full explanation. Confirmed via
# x86_64-linux-gnu-gcc 13.3.0.
CFLAGS += -Wno-infinite-recursion
ASFLAGS = -gdwarf-2 -Wa,-divide $(XFLAGS) -Ikernel -I../../include/kernel
# FreeBSD ld wants ``elf_i386_fbsd''
LDFLAGS += -m $(shell $(LD) -V | grep elf_x86_64 2>/dev/null | head -n 1)
LDFLAGS += -z max-page-size=4096

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

# claude: target renamed "kernel" -> "kernel/kernel" - a make target cannot
# share a name with an existing directory: kernel/ exists, so make would
# treat the target as already built and never relink. See gotcha 2 in
# plan_factorization.md.
kernel/kernel: $(OBJS) kernel/entry.o entryother initcode kernel/kernel.ld
	$(LD) $(LDFLAGS) -T kernel/kernel.ld -o kernel/kernel kernel/entry.o $(OBJS) -b binary initcode entryother
	$(OBJDUMP) -S kernel/kernel > kernel/kernel.asm
	$(OBJDUMP) -t kernel/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel/kernel.sym

# claude: the generated "entryother" and "initcode" blobs stay at the fork
# root even though their .S sources moved into kernel/ - "ld -b binary"
# derives its synthetic symbols from the file path, so kernel/entryother
# would yield _binary_kernel_entryother_start while main.c reads
# _binary_entryother_start. Gotcha 3.
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

tags: $(OBJS) kernel/entryother.S user/_init
	etags kernel/*.S kernel/*.c user/*.c tests/*.c ulib/*.c ulib/*.S

kernel/vectors.S: tools/vectors.pl
	./tools/vectors.pl > kernel/vectors.S

# claude: the portable user programs live at the repo-root utilities/ and are
# shared by every fork. An explicit pattern rule, not
# vpath: vpath matches the WHOLE prerequisite path, so "vpath %.c ../../utilities"
# would make make look for utilities/user/echo.c. The object still lands in this
# fork, so concurrent per-fork builds never race on a shared output file.
# claude: "@mkdir -p $(dir $@)" - user/ has no tracked file of its own any
# more (every source that used to live there moved to a shared dir), so a
# clean checkout (CI, docker build) never creates it; only a stale local
# build tree happens to have it already. Caught by CI failing on i386,
# amd64 and mips with "fatal error: opening dependency file user/cat.d: No
# such file or directory" while the same build was green on a machine that
# had already built here before - see arm-pi1's own $(BUILD)csud/%.o rule
# for the same pattern.
user/%.o: ../../utilities/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

user/%.o: ../../shells/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

user/%.o: ../../kernel/init/user/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# claude: the portable test programs, shared at the repo root. usertests.c
# is only partly among them - most forks' own copy is genuinely per-port
# (each with its own skipped sub-tests) and stays local, but this fork's
# copy was close enough to forks/i386's to share - see the explicit rule
# below and tests/usertests-x86.c's own header comment.
tests/%.o: ../../tests/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# claude: explicit rule, not covered by the generic pattern above - the
# shared file's name doesn't match usertests.o's expected usertests.c.
tests/usertests.o: ../../tests/usertests-x86.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# claude: the shared user library, under lib_core/ per ~/principia's
# convention. Separate rule because these objects land in ulib/, not
# alongside the programs.
ulib/%.o: ../../lib_core/libc/ulib/%.c
	@mkdir -p $(dir $@)
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

# claude: -idirafter, NOT -I. mkfs is a HOST program and includes the
# system <fcntl.h> as well as xv6's own "fs.h" - -Ikernel is searched
# before the system directories, so <fcntl.h> would resolve to
# kernel/fcntl.h, which has no O_TRUNC. See i386's identical fix.
# -idirafter ../../include/kernel too, same reasoning now that fcntl.h
# and stat.h moved there (include/kernel/fcntl.h itself defines O_CREATE,
# not POSIX's O_CREAT that this file's own <fcntl.h> use needs - a plain
# -I here would reintroduce the exact shadowing this rule exists to avoid).
tools/mkfs: ../../tools/mkfs-nomagic.c kernel/fs.h
	gcc -Werror -Wall -idirafter kernel -idirafter ../../include/kernel -o tools/mkfs ../../tools/mkfs-nomagic.c -I../../include -I../../include/user

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
	initcode initcode.out fs.img \
	tools/mkfs .gdbinit \
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

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 2
endif
QEMUOPTS = -kernel kernel/kernel -drive file=fs.img,index=1,media=disk,format=raw -smp $(CPUS) -m 512 $(QEMUEXTRA)
# claude: was "qemu: fs.img" - missing the "kernel/kernel" prerequisite
# its three siblings below (qemu-nox, qemu-gdb, qemu-nox-gdb) all have,
# so "make qemu" (and the top-level Makefile's run-amd64-qemu-graphics,
# which calls it) could launch QEMU with a stale or entirely missing
# kernel/kernel and no rebuild.
qemu: fs.img kernel/kernel
	$(QEMU) -serial mon:stdio $(QEMUOPTS)

qemu-nox: fs.img kernel/kernel
	$(QEMU) -nographic $(QEMUOPTS)

.gdbinit: tools/.gdbinit.tmpl-x64
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

qemu-gdb: fs.img kernel/kernel .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

qemu-nox-gdb: fs.img kernel/kernel .gdbinit
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
