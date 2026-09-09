OBJS = \
	bio.o\
	console.o\
	exec.o\
	file.o\
	fs.o\
	ide.o\
	kalloc.o\
	kbd.o\
	log.o\
	main.o\
	mp.o\
	picirq.o\
	pipe.o\
	proc.o\
	procasm.o\
	spinlock.o\
	string.o\
	swtch.o\
	syscall.o\
	sysfile.o\
	sysproc.o\
	timer.o\
	trapasm.o\
	trap.o\
	uart.o\
	vectors.o\
	vm.o\

# Cross-compiling (e.g., on Mac OS X)
TOOLPREFIX = mipsel-sde-elf-

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
QEMU = qemu-system-mipsel

# Try to infer the correct QEMU
ifndef QEMU
QEMU = $(shell if which qemu > /dev/null; \
	then echo qemu; exit; \
	elif which qemu-system-i386 > /dev/null; \
	then echo qemu-system-i386; exit; \
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
CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -fvar-tracking -fvar-tracking-assignments -O0 -g -std=gnu99 -Wall -Wunused-function -MD -MP -gdwarf-2 -march=mips4 -G0 -fno-omit-frame-pointer -I.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
# claude: mipsel-linux-gnu-gcc defaults -mabicalls on (confirmed via
# "gcc -Q --help=target"), i.e. GOT-relative R_MIPS_GOT16/CALL16 calls -
# a PIC-adjacent ABI convention independent from -fno-pic (which only
# controls data addressing, not the calling convention). This kernel is
# a single statically-linked freestanding image with no runtime linker
# to populate a GOT for, and its many-KB size overflows GOT16/CALL16's
# 16-bit relative range, failing the link with "relocation truncated to
# fit". -mno-abicalls switches to direct absolute jumps/calls instead -
# the toolchain this Makefile's own comments target (a 2014-era
# mipsel-sde-elf-, matching the port's original author's setup) likely
# defaulted -mabicalls off for a bare-metal target; this modern Ubuntu
# Linux-userspace-oriented cross toolchain does not.
CFLAGS += -mno-abicalls
ASFLAGS = -gdwarf-2 -I. -G0 -march=mips4 -mno-abicalls
# FreeBSD ld wants ``elf_i386_fbsd''
LDFLAGS += -m elf32ltsmip -L. -G0

xv6memfs.img: kernelmemfs
	dd if=/dev/zero of=xv6memfs.img count=10000
	dd if=kernelmemfs of=xv6memfs.img seek=1 conv=notrunc

xv6.img: kernel fs.img
	dd if=/dev/zero of=xv6.img count=10000
	dd if=kernel of=xv6.img seek=1 conv=notrunc

bootblock: bootasm.S bootmain.c
	$(CC) $(CFLAGS) -fno-pic -O -nostdinc -I. -c bootmain.c
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c bootasm.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o bootblock.o bootasm.o bootmain.o
	$(OBJDUMP) -S bootblock.o > bootblock.asm
	$(OBJCOPY) -S -O binary -j .text bootblock.o bootblock
	./sign.pl bootblock

# claude: also strip .reginfo, not just .MIPS.abiflags - both get placed
# by the default MIPS linker script at a virtual address ~4MB away from
# .text (this Makefile's own "-N -Ttext 0" has no script telling the
# linker to co-locate them), and "objcopy -O binary" fills the entire
# gap between the lowest and highest surviving section with zero bytes -
# so leaving .reginfo in produced a ~4.2MB "initcode" file (should be a
# few dozen bytes), embedded whole into the kernel image via the
# "kernel: ... -b binary initcode" link step below, which in turn pushed
# the kernel's own `end` symbol past main.c's hardcoded 4MB kinit1()
# phase-1 boundary and made every very first kfree() call fail its own
# "v < end" sanity check (see docs/claude_notes/notes_arch_mips.txt).
initcode: initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -c initcode.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
	$(OBJCOPY) -R .MIPS.abiflags -R .reginfo -S -O binary initcode.out initcode
	$(OBJDUMP) -S initcode.o > initcode.asm

kernel: $(OBJS) entry.o initcode kernel.ld
	$(LD) $(LDFLAGS) -T kernel.ld -o kernel entry.o $(OBJS) -b binary initcode
	$(OBJDUMP) -S kernel > kernel.asm
	$(OBJDUMP) -t kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym

# kernelmemfs is a copy of kernel that maintains the
# disk image in memory instead of writing to a disk.
# This is not so useful for testing persistent storage or
# exploring disk buffering implementations, but it is
# great for testing the kernel on real hardware without
# needing a scratch disk.
MEMFSOBJS = $(filter-out ide.o,$(OBJS)) memide.o
kernelmemfs: $(MEMFSOBJS) entry.o initcode kernel.ld fs.img
	$(LD) $(LDFLAGS) -T kernel.ld -o kernelmemfs entry.o  $(MEMFSOBJS) -b binary initcode fs.img
	$(OBJDUMP) -S kernelmemfs > kernelmemfs.asm
	$(OBJDUMP) -t kernelmemfs | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernelmemfs.sym

tags: $(OBJS) _init
	etags *.S *.c

vectors.S: vectors.pl
	perl vectors.pl > vectors.S

ULIB = lib/ulib.o lib/usys.o lib/printf.o lib/umalloc.o

# claude: "-T user.ld" added (was just "-N -e main -Ttext 0") - see
# user.ld's own header comment for why: the default MIPS linker script
# gives .reginfo its own stray LOAD-typed program header at a non-page-
# aligned address, which exec()'s loaduvm() panics on for every single
# user program (not just initcode, which the pre-existing
# "$(OBJCOPY) -R .MIPS.abiflags" line below was a partial, insufficient
# fix for - see notes_arch_mips.txt bug 6). That objcopy line is now
# redundant (user.ld already discards .MIPS.abiflags at link time, so
# "-R .MIPS.abiflags" is a harmless no-op) but left in place since it's
# still needed to produce $@ from the $*.o intermediate ld actually
# writes to.
_%: usr/%.o $(ULIB) user.ld
	$(LD) $(LDFLAGS) -N -e main -T user.ld -o $*.o $< $(ULIB)
	$(OBJCOPY) -R .MIPS.abiflags $*.o $@
	$(OBJDUMP) -S $*.o > $*.asm
	$(OBJDUMP) -t $*.o | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

_forktest: usr/forktest.o $(ULIB) user.ld
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -T user.ld -o _forktest usr/forktest.o lib/ulib.o lib/usys.o
	$(OBJDUMP) -S _forktest > forktest.asm

mkfs: tools/mkfs.c fs.h
	gcc -g -Werror -Wall -std=gnu99 -o mkfs tools/mkfs.c -idirafter .

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

# claude: was two lists here, UPROGS and a BACK_UPROGS that added
# _usertests (excluded from UPROGS - see fs.h's own comment on why:
# it didn't fit, MAXFILE was too small) - now that's fixed, they were
# identical, so BACK_UPROGS (never referenced anywhere else) was dead
# weight and removed rather than kept as two lists to keep in sync.
UPROGS=\
	_cat\
	_echo\
	_forktest\
	_grep\
	_init\
	_kill\
	_ln\
	_ls\
	_mkdir\
	_rm\
	_sh\
	_stressfs\
	_usertests\
	_wc\
	_zombie\


fs.img: mkfs README $(UPROGS)
	./mkfs fs.img README $(UPROGS)

-include *.d

clean: 
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*.o *.d *.asm *.sym vectors.S bootblock \
	initcode initcode.out kernel xv6.img fs.img kernelmemfs mkfs \
	$(UPROGS) \
	lib/*.o lib/*.d \
	usr/*.o usr/*.d usr/*.sym usr/*.asm

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 1
endif
# QEMUOPTS = -hdb fs.img -kernel kernel -smp $(CPUS) -m 512 $(QEMUEXTRA)
# claude: "-M mips" no longer exists in modern QEMU ("unsupported machine
# type"; see notes_arch_mips.txt) - "-M malta" is the still-supported
# board this port actually boots on.
QEMUOPTS = -hdb fs.img -kernel kernel -smp 1 -m 256 -M malta
QEMUOPTSMEMFS = -kernel kernelmemfs -smp 1 -m 256 -M malta

qemu: fs.img kernel
	@echo -e "\033[0;31m'make qemu' is not implemented at current. Run 'make qemu-memfs'!\033[0m"
	$(QEMU) -serial mon:stdio $(QEMUOPTS)

qemu-memfs: kernelmemfs
	$(QEMU) -serial mon:stdio $(QEMUOPTSMEMFS)

# claude: upstream never paired -nographic with the memfs boot path (only
# "qemu-nox", which is the disk-based QEMUOPTS - the path this port's own
# Makefile admits "is not implemented at current", see notes_arch_mips.txt).
# Without -nographic, "qemu-memfs" opens a QEMU GTK window ("Guest has not
# initialized the display (yet)") since this kernel only ever writes to
# the serial port, never a framebuffer - the window is empty by design,
# not a hang, but it's not what "make run-mips" should show.
qemu-nox-memfs: kernelmemfs
	$(QEMU) -nographic $(QEMUOPTSMEMFS)

qemu-nox: fs.img xv6.img
	$(QEMU) -nographic $(QEMUOPTS)

#.gdbinit: .gdbinit.tmpl
#	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

#qemu-gdb: fs.img xv6.img .gdbinit
#	@echo "*** Now run 'gdb'." 1>&2
#	$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB)

qemu-gdb: fs.img kernel .gdbinit
	@echo -e "\033[0;31m'make qemu-gdb' is not implemented at current. Run 'make qemu-memfs-gdb'!\033[0m"
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB)

qemu-memfs-gdb: kernelmemfs .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -serial mon:stdio $(QEMUOPTSMEMFS) -S $(QEMUGDB)

qemu-nox-gdb: fs.img xv6.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -nographic $(QEMUOPTS) -S $(QEMUGDB)

