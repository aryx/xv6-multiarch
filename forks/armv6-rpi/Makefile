# specify path to QEMU, installed with MacPorts 
QEMU = qemu-system-arm

include makefile.inc

# link the libgcc.a for __aeabi_idiv. ARM has no native support for div
#
# claude: dropped "libcsud.a" (Chadderz's Simple USB Driver, for real
# Raspberry Pi USB keyboard input per this port's own README) - it was
# never vendored into this tree at all (README: "recompile csud... and
# copy libcsud.a in the project root" - an external, user-supplied
# dependency), and nothing actually references it: keyboard.s is a
# separate, self-contained assembly stub with no CSUD symbols anywhere
# (confirmed via a repo-wide grep for "csud"/"CSUD" - zero hits outside
# this Makefile line and the README). It was pure dead linkage, and
# without a real libcsud.a present the link fails outright ("cannot find
# libcsud.a") regardless of whether anything needs it. Not relevant to a
# QEMU/-nographic boot anyway (no real Pi USB hardware to drive; input
# goes through the emulated UART like every other fork in this repo).
LIBS = $(LIBGCC)

# claude: dropped keyboard.o - its own entry points (getKeyboard/
# saveKeys/keyLoop) are called from nowhere else in this tree (real
# Raspberry Pi USB keyboard support, via the same missing libcsud.a
# above) and link-fail with undefined references to CSUD functions
# without it. Genuinely dead code for this bring-up: interactive input
# goes through the UART console like every other fork here, not a real
# USB HID keyboard QEMU's versatilepb doesn't emulate anyway.
OBJS = \
	bio.o\
	console.o\
	exception.o\
	exec.o\
	file.o\
	fs.o\
        kalloc.o\
	log.o\
	mailbox.o\
	main.o\
	memide.o\
	mmu.o\
	pipe.o\
	proc.o\
	spinlock.o\
	string.o\
	syscall.o\
	sysfile.o\
	sysproc.o\
	timer.o\
	trap.o\
	uart.o\
	wrapper.o\
	vm.o \

#device/picirq.o \
	
KERN_OBJS = $(OBJS) entry.o

kernel.elf: $(addprefix build/,$(KERN_OBJS)) kernel.ld build/initcode build/fs.img
	cp -f build/initcode initcode
	cp -f build/fs.img fs.img
	$(call LINK_BIN, kernel.ld, kernel.elf, \
		$(addprefix build/,$(KERN_OBJS)), \
		initcode fs.img)
	$(OBJCOPY) kernel.elf -O binary kernel.img
	@echo kernel image has been built.
	@$(OBJDUMP) -d kernel.elf > kernel.dis
	$(OBJDUMP) -S kernel.elf > kernel.asm
	$(OBJDUMP) -t kernel.elf | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym
	rm -f initcode fs.img

# claude: was "-M versatilepb -cpu arm1176 -kernel kernel.elf" - versatilepb
# is a completely unrelated ARM926EJ-S reference board (a QEMU-only
# workaround from before real Raspberry Pi board emulation existed) whose
# peripheral memory map shares NOTHING with the real BCM2835 GPIO/mailbox/
# interrupt-controller addresses this kernel hardcodes (confirmed via
# "-d int,guest_errors": every GPIO/mailbox access was a rejected invalid
# MMIO write). "-M raspi1ap" is QEMU's actual Raspberry Pi 1 A+ (BCM2835)
# machine, whose addresses genuinely match - and "-kernel" needs the raw
# "kernel.img" (real Pi/raspi1ap firmware boot convention), not
# "kernel.elf" (this port's own kernel.ld links at 0x80010000/AT(0x10000),
# matching raspi1ap's own "-kernel" load address - see
# notes_arch_armv6_rpi.txt for the fuller investigation, including a
# still-open issue past this point: no console output yet).
qemu: kernel.elf
	@clear
	@echo "Press Ctrl-A and then X to terminate QEMU session\n"
	$(QEMU) -M raspi1ap -nographic -kernel kernel.img

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
	rm -f *.o *.d *.asm *.sym vectors.S bootblock \
	initcode initcode.out fs.img kernel.elf kernel.dis kernel.img
	make -C tools clean
	make -C usr clean
