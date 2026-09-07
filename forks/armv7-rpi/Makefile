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
qemu: kernel.elf
	@clear
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

build/fs.img:
	make -C tools
	make -C usr

clean: 
	rm -rf build
	rm -f *.o *.d *.asm *.sym vectors.S bootblock entryother \
	initcode initcode.out kernel xv6.img fs.img kernel.elf memfs
	make -C tools clean
	make -C usr clean
