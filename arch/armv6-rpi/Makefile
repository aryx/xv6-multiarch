# specify path to QEMU, installed with MacPorts 
QEMU = qemu-system-arm

include makefile.inc

# link the libgcc.a for __aeabi_idiv. ARM has no native support for div
LIBS = $(LIBGCC) libcsud.a

OBJS = \
	bio.o\
	console.o\
	exception.o\
	exec.o\
	file.o\
	fs.o\
        kalloc.o\
        keyboard.o\
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

qemu: kernel.elf
	@clear
	@echo "Press Ctrl-A and then X to terminate QEMU session\n"
	$(QEMU) -M versatilepb -m 128 -cpu arm1176  -nographic -kernel kernel.elf

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
