# specify path to QEMU, installed with MacPorts 
QEMU = qemu-system-arm

include makefile.inc

# link the libgcc.a for __aeabi_idiv. ARM has no native support for div
LIBS = $(LIBGCC)

OBJS = \
	lib/string.o \
	\
	arm.o\
	asm.o\
	bio.o\
	buddy.o\
	cache.o\
	console.o\
	exec.o\
	file.o\
	fs.o\
	log.o\
	main.o\
	memide.o\
	pipe.o\
	proc.o\
	spinlock.o\
	start.o\
	swtch.o\
	syscall.o\
	sysfile.o\
	sysproc.o\
	trap_asm.o\
	trap.o\
	vm.o \
	\
	device/timer.o \
	device/uart.o \
	device/gic.o

#device/picirq.o \
	
KERN_OBJS = $(OBJS) entry.o
kernel.elf: $(addprefix build/,$(KERN_OBJS)) kernel.ld build/initcode build/fs.img
	cp -f build/initcode initcode
	cp -f build/fs.img fs.img
	$(call LINK_BIN, kernel.ld, kernel.elf, \
		$(addprefix build/,$(KERN_OBJS)), \
		initcode fs.img)
	$(OBJCOPY) kernel.elf -O binary xv6.img
	@echo kernel image has been built.
	$(OBJDUMP) -d kernel.elf > kernel.dis
	$(OBJDUMP) -S kernel.elf > kernel.asm
	$(OBJDUMP) -t kernel.elf | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym
	rm -f initcode fs.img

# claude: was "-M versatilepb -cpu arm1176 -kernel kernel.elf" - same
# class of board mismatch as the sibling armv6-rpi port (see
# notes_arch_armv6_rpi.txt): versatilepb is an unrelated ARM926EJ-S
# reference board, and "-cpu arm1176" (ARMv6, no Hyp mode, no Thumb-2)
# doesn't even match this kernel's own "-march=armv7-a -mtune=cortex-a7"
# codegen target or its Hyp-mode-aware entry.S. "-M raspi2b" is QEMU's
# real Raspberry Pi 2B (BCM2836/Cortex-A7, quad-core) machine, matching
# this kernel's own hardcoded 0x3f000000-range peripheral addresses -
# but a raw "kernel.img"-style image is needed for -kernel here (an ELF
# skips raspi2b's own armstub/firmware handoff and starts at EL3/Monitor
# mode instead of the expected Hyp/SVC mode - see
# notes_arch_armv7_rpi.txt for the fuller investigation, including a
# still-open, more substantial gap past this point: no secondary-CPU
# boot-gating anywhere in this codebase, so all 4 emulated cores race).
# claude: "-" (not just "@") on the clear line - a TTY-less caller (a
# Docker/CI build has no TERM set at all) makes plain "clear" exit
# non-zero ("TERM environment variable not set"), which without the "-"
# aborts this whole recipe before qemu ever runs - confirmed via
# "docker build --build-arg ARCH=arm" failing at exactly this line.
# Interactive use from a real terminal is unaffected either way.
qemu: kernel.elf
	-@clear
	@echo "Press Ctrl-A and then X to terminate QEMU session\n"
	$(QEMU) -M raspi2b -nographic -kernel xv6.img

INITCODE_OBJ = initcode.o
$(addprefix build/,$(INITCODE_OBJ)): initcode.S
	$(call build-directory)
	$(call AS_WITH, -nostdinc -I.)

#initcode is linked into the kernel, it will be used to craft the first process
build/initcode: $(addprefix build/,$(INITCODE_OBJ))
	$(call LINK_INIT, -N -e start -Ttext 0)
	$(call OBJCOPY_INIT)
	$(OBJDUMP) -S $< > initcode.asm

# claude: "build/fs.img" had NO prerequisites, so once the file existed make
# considered it final forever - editing anything in usr/ (or tools/) rebuilt
# nothing, and the kernel kept embedding the previously built image. That is
# not a theoretical hazard: it silently invalidated a debugging run during
# this port's own kill()-fix investigation, where an instrumented usertests.c
# produced a kernel still containing the OLD binary and therefore no output.
#
# The phony target is "usr-fs-img", not "build/fs.img" itself: a phony
# prerequisite of a real file forces that file's recipe to run every time,
# which is what we want for the recursive make (usr/ and tools/ do their own
# up-to-date checking), while build/fs.img's own timestamp still only moves
# when the sub-make actually rewrites it - so kernel.elf is re-linked when the
# image really changed, and not otherwise.
.PHONY: usr-fs-img
usr-fs-img:
	$(MAKE) -C tools
	$(MAKE) -C usr

build/fs.img: usr-fs-img
	@:


# claude: read back the per-object header dependencies gcc writes next to
# each object ("-MD" in makefile.inc's CFLAGS/ASFLAGS). Without this the
# .d files were either never generated (forks/arm) or generated and
# thrown away (every other Pi port), so editing a header rebuilt NOTHING
# - which is exactly why this port's test-xv6.py used to run "make clean"
# before every test run, and why "make test-all" recompiled from scratch
# even straight after "make build-all". The leading "-" ignores the files
# on a first build, when they do not exist yet. See
# docs/claude_notes/notes_build_system.txt for why this replaces an
# explicit "make depend" pass rather than adding one.
-include build/*.d build/*/*.d

clean: 
	rm -rf build
	rm -f *.o *.d *.asm *.sym vectors.S bootblock entryother \
	initcode initcode.out kernel xv6.img fs.img kernel.elf memfs
	make -C tools clean
	make -C usr clean
