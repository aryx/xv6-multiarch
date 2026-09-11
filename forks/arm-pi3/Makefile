UNAME := $(shell uname -m)

ifeq ($(UNAME), x86_64)
	# set FVP as default on x86_64 systems	
	hw?=rpi2
	TOOLCHAIN?=arm-none-eabi-
else
	# RPI reports 'uname -m' as: armv7l
	hw?=rpi2
endif

# PHYSTART  : start of memory (RAM)
# PHYSIZE   : size of memory
# KERNBASE  : address of high memory schema
# MMIO_PA   : physical address of peripherals (MMIO)
# MMIO_VA   : virtal address of peripherals
# MMIO_SIZE : size of peripherals
# PERIPHBASE: virtal address of PERIPHBASE[39:15]

# claude: each hw= block's own CFLAGS carries "-I../../kernel" -
# kernel/spinlock.h is shared there (kernel-only, so top-level kernel/
# rather than include/kernel/).
ifeq ($(hw), fvp)
	PHYSTART  = 0x80000000
	PHYSIZE   = 0x08000000
	KERNBASE  = 0xC0000000
	MMIO_PA   = 0x1C000000
	MMIO_VA   = 0xD0000000
	MMIO_SIZE = 0x04000000
	PERIPHBASE= 0xDF000000
	# claude: -fcommon added (here and below) - modern GCC (10+) defaults to
	# -fno-common, and include/proc.h's "struct cpu cpus[NCPU];" is a
	# non-extern global declaration included by every .c file, so without
	# -fcommon the linker sees it as a real multiple-definition conflict.
	# claude: -fno-pic -fno-pie added (here and below) - see the rpi2
	# block's own long comment for the full story (found chasing a real
	# "cpu0: panic: release" SMP race in this fork; identical root cause
	# and fix to forks/arm-pi2's own equivalent bug).
	CFLAGS    =   -ffreestanding -nostdlib -nostartfiles -fcommon -marm -fno-pic -fno-pie -O0 -Wall -MD -MP -ggdb -Wall -mcpu=cortex-a9 -mfloat-abi=hard -fno-short-enums -I kernel -I../../include -I../../include/kernel -I../../include/user -I../../kernel -I../../include/arch/arm
	#CFLAGS    =   -ffreestanding -nostdlib -nostartfiles -O2 -Wall -MD -ggdb -Wall -mcpu=cortex-a9 -mfloat-abi=hard -fno-short-enums -I kernel
	TARGET    = fvp.img
	CC_OPTIONS = -DFVP
else ifeq ($(hw), rpi1)
	PHYSTART  = 0x00000000
	# PHYSIZE set to 128mb
	PHYSIZE   = 0x08000000
	KERNBASE  = 0x80000000
	MMIO_PA   = 0x20000000
	MMIO_VA   = 0xD0000000
	MMIO_SIZE = 0x01000000
	PERIPHBASE= 0xDF000000
	#CFLAGS   = -fno-pic -static -Wno-packed-bitfield-compat -fno-builtin -fno-strict-aliasing -fshort-wchar -O2 -Wall -MD -ggdb -Werror -fno-omit-frame-pointer -fno-stack-protector -Wa,-march=armv6 -Wa,-mcpu=arm1176jzf-s -mfloat-abi=hard -fno-short-enums -I kernel
	# claude: -fno-pic -fno-pie restored from the commented-out
	# alternate CFLAGS line right above (which already had -fno-pic -
	# real evidence someone hit this before) - see the rpi2 block's own
	# long comment.
	CFLAGS    =   -ffreestanding -nostdlib -nostartfiles -fcommon -marm -fno-pic -fno-pie -O2 -Wall -MD -MP -ggdb -Wall -mcpu=arm1176jzf-s -mfloat-abi=hard -fno-short-enums -I kernel -I../../include -I../../include/kernel -I../../include/user -I../../kernel -I../../include/arch/arm
	TARGET    = kernel.img
	CC_OPTIONS = -DRPI1
else ifeq ($(hw), rpi2)
	 PHYSTART = 0x00000000
	# PHYSIZE set to 256mb
	PHYSIZE   = 0x10000000
	KERNBASE  = 0x80000000
	MMIO_PA   = 0x3F000000
	MMIO_VA   = 0xD0000000
	MMIO_SIZE = 0x01000000
	PERIPHBASE= 0xDF000000
	# claude: -marm added (here and below) - same class of bug as the
	# sibling forks/arm-pi1-bis fork's own Bug 4 (notes_arch_arm_pi1_bis.txt):
	# without an explicit ARM/Thumb choice, gcc's default codegen mode can
	# lead the linker to emit a Thumb-switching "blx" for some call
	# (confirmed here via QEMU: PSR showed Thumb mode active with an odd
	# (bit0-set) LR, and PC wandering into unrelated memory once in that
	# state) - "-marm" forces plain ARM code generation outright, removing
	# the state-switch question entirely for this freestanding kernel.
	#
	# claude: -fno-pic -fno-pie added - this toolchain defaults to
	# PIC/PIE code generation for ordinary C code: every global-variable
	# access (not just "cpus[]") compiled to a GOT-indirect load
	# ("ldr r5,[pc,#N]; add r5,pc,r5; ldr r3,[pc,#M]; ldr r5,[r5,r3]" -
	# confirmed via "objdump -d" on acquire()'s own compiled code,
	# identical signature to forks/arm-pi2's own "Bug 7"). GOT-indirect
	# addressing assumes a dynamic loader populates that table at
	# startup; this bare-metal kernel has no loader and never populates
	# any GOT, so every such access silently read/wrote through
	# whatever garbage happened to occupy that unpopulated memory
	# instead of the real variable. Found chasing this fork's own real
	# "cpu0: panic: release" crash (spinlock.c's own release() detecting
	# !holding(lk) - a lock-consistency check that should never fail on
	# a correctly-compiled kernel) under sustained SMP load
	# ("usertests") - "-fno-pic -fno-pie" restores direct "movw/movt"
	# absolute addressing (confirmed via the same objdump comparison).
	# A real ISA/ABI-correctness bug affecting every global variable in
	# the kernel and every core equally, not a QEMU quirk - real
	# hardware would behave identically (or worse - unpopulated-GOT
	# memory content isn't guaranteed to even be zero there either).
	CFLAGS    = -ffreestanding -nostdlib -nostartfiles -fcommon -marm -fno-pic -fno-pie -O2 -Wall -MD -MP -std=gnu99 -ggdb -Wall -mcpu=cortex-a7 -mfloat-abi=hard -fno-short-enums -fno-zero-initialized-in-bss -I kernel -I../../include -I../../include/kernel -I../../include/user -I../../kernel -I../../include/arch/arm
	#CFLAGS    = -ffreestanding -nostdlib -nostartfiles -O2 -Wall -MD -ggdb -Wall -mcpu=cortex-a7 -mfloat-abi=hard -fno-short-enums -I kernel
	TARGET    = kernel7.bin
	CC_OPTIONS = -DRPI2 -DRASPPI=2 -DNDEBUG
else
$(error Hardware (hw) should be fvp, rpi1 or rpi2, eg. make hw=rpi2)
endif


K_PDX_BASE=$(shell printf "0x%X\n" $$(( $(PHYSTART) + 0x4000 )) )
K_PTX_BASE=$(shell printf "0x%X\n" $$(( $(PHYSTART) + 0x3000 )) )
OFFSET=0x8000
PHYSOFFSET=$(shell printf "0x%X\n" $$(( $(PHYSTART) + $(OFFSET) )) )
KERNOFFSET=$(shell printf "0x%X\n" $$(( $(KERNBASE) + $(OFFSET) )) )

CC_OPTIONS += -DPHYSTART=$(PHYSTART) -DPHYSIZE=$(PHYSIZE) -DKERNBASE=$(KERNBASE) -DMMIO_PA=$(MMIO_PA) -DMMIO_VA=$(MMIO_VA) -DMMIO_SIZE=$(MMIO_SIZE) -DPERIPHBASE=$(PERIPHBASE)
CC_OPTIONS += -DK_PDX_BASE=$(K_PDX_BASE) -DK_PTX_BASE=$(K_PTX_BASE) -DPHYSOFFSET=$(PHYSOFFSET) -DKERNOFFSET=$(KERNOFFSET)

LD_OPTIONS = --defsym=PHYSTART=$(PHYSTART) --defsym=PHYSIZE=$(PHYSIZE) --defsym=KERNBASE=$(KERNBASE) --defsym=MMIO_PA=$(MMIO_PA) --defsym=MMIO_VA=$(MMIO_VA) --defsym=MMIO_SIZE=$(MMIO_SIZE) --defsym=PERIPHBASE=$(PERIPHBASE) 
LD_OPTIONS += --defsym=K_PDX_BASE=$(K_PDX_BASE) --defsym=K_PTX_BASE=$(K_PTX_BASE) --defsym=PHYSOFFSET=$(PHYSOFFSET) --defsym=KERNOFFSET=$(KERNOFFSET)

# Build directory
BUILD=build/
# Sources directory.
SOURCE=kernel/
# Generated listing file.
LIBRARIES=

# The names of all object files that must be generated. Deduced from the 
# assembly code files in source.
ASM_OBJECTS = $(patsubst $(SOURCE)%.S,$(BUILD)%.o,$(wildcard $(SOURCE)*.S))

C_OBJECTS = $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))

# Rule to make everything.
all : $(TARGET)

# Rule to make the elf file.
$(TARGET): $(ASM_OBJECTS) $(C_OBJECTS) kernel/kernel.ld
	$(TOOLCHAIN)ld $(ASM_OBJECTS) $(C_OBJECTS) -L. $(patsubst %,-l %,$(LIBRARIES)) $(LD_OPTIONS) -Map kernel.map -o $(BUILD)kernel.elf -T kernel/kernel.ld
	$(TOOLCHAIN)objdump -d $(BUILD)kernel.elf > kernel.list
	$(TOOLCHAIN)objcopy $(BUILD)kernel.elf -O binary $(TARGET)

# claude: QEMU-only target, added alongside the real-hardware ones above
# rather than replacing anything - "all"/"$(TARGET)" (what a real
# Raspberry Pi 3 boots as "kernel7.bin") are completely unchanged. Needs
# its own object directory (build-qemu/, not the real-hardware build/)
# because -DRPI3_QEMU selects a different uart.c code path at COMPILE
# time (PL011 instead of the Mini-UART QEMU doesn't wire to any chardev
# - see uart.c's own comment) - unlike the sibling forks/arm-pi2 fork's
# QEMU target, which only differs at LINK time (load address), this one
# needs a real recompile. Targets the real "-M raspi3b" - the board this
# fork was actually written for.
#
# The Cortex-A53 QEMU emulates for raspi3b always resets in AArch64 EL3
# (confirmed via gdb-multiarch: a 268-byte gdbstub register set) - real
# hardware's GPU firmware drops it to AArch32 before jumping to a
# 32-bit kernel7.img, but QEMU's raw "-kernel"/"-device loader" loaders
# skip that firmware stage entirely and just jump straight into the
# loaded image in whatever state the CPU already has. TCG can't paper
# over this with a "-cpu ...,aarch64=off" property either (tried -
# QEMU refuses: "'aarch64' feature cannot be disabled unless KVM is
# enabled and 32-bit EL1 is supported", and KVM itself refuses raspi3b
# separately: "Cannot enable KVM when guest CPU has EL3 enabled"). So
# this supplies the missing firmware step itself: armstub64.S is a
# small AArch64 stub - real hardware's own equivalent of
# "armstub8-32.bin" - that QEMU's "-kernel" loads and jumps to first (at
# physical 0x80000, where raspi3b's "-kernel" loader places a raw image
# - confirmed via the monitor's "info roms"). Turns out QEMU's own
# raspi3b board-setup ROM already does the EL3->EL2 half of the real
# firmware's drop itself (confirmed via gdb: this stub's own entry
# point is already EL2h AArch64, not EL3) - so the stub only needs to
# finish the job, EL2->AArch32 EL1 (see armstub64.S's own comment for
# the HCR_EL2 subtlety that took two tries to get right), then erets
# straight into this fork's own 32-bit kernel - loaded separately via a
# second "-device loader" at QEMU_ARM_ENTRY (see the "qemu" target
# below), clear of both the stub itself and QEMU's own low-memory boot
# scaffolding (bootloader/spintables/smpboot/board-setup, all below
# 0x500).
QEMU ?= qemu-system-aarch64
QEMU_AARCH64_TOOLPREFIX ?= aarch64-linux-gnu-
BUILD_QEMU = build-qemu/
ASM_OBJECTS_QEMU = $(patsubst $(SOURCE)%.S,$(BUILD_QEMU)%.o,$(wildcard $(SOURCE)*.S))
C_OBJECTS_QEMU = $(patsubst $(SOURCE)%.c,$(BUILD_QEMU)%.o,$(wildcard $(SOURCE)*.c))
QEMU_CC_OPTIONS = $(CC_OPTIONS) -DRPI3_QEMU
QEMU_ARM_ENTRY = 0x90000
QEMU_LD_OPTIONS = --defsym=PHYSTART=$(PHYSTART) --defsym=PHYSIZE=$(PHYSIZE) --defsym=KERNBASE=$(KERNBASE) --defsym=MMIO_PA=$(MMIO_PA) --defsym=MMIO_VA=$(MMIO_VA) --defsym=MMIO_SIZE=$(MMIO_SIZE) --defsym=PERIPHBASE=$(PERIPHBASE)
QEMU_LD_OPTIONS += --defsym=K_PDX_BASE=$(K_PDX_BASE) --defsym=K_PTX_BASE=$(K_PTX_BASE)
QEMU_LD_OPTIONS += --defsym=PHYSOFFSET=$(shell printf "0x%X\n" $$(( $(PHYSTART) + $(QEMU_ARM_ENTRY) )) )
QEMU_LD_OPTIONS += --defsym=KERNOFFSET=$(shell printf "0x%X\n" $$(( $(KERNBASE) + $(QEMU_ARM_ENTRY) )) )

kernel7-qemu.bin: $(ASM_OBJECTS_QEMU) $(C_OBJECTS_QEMU) kernel/kernel.ld
	$(TOOLCHAIN)ld $(ASM_OBJECTS_QEMU) $(C_OBJECTS_QEMU) -L. $(patsubst %,-l %,$(LIBRARIES)) $(QEMU_LD_OPTIONS) -Map kernel-qemu.map -o $(BUILD_QEMU)kernel-qemu.elf -T kernel/kernel.ld
	$(TOOLCHAIN)objcopy $(BUILD_QEMU)kernel-qemu.elf -O binary kernel7-qemu.bin

armstub64.bin: armstub64.S armstub64.ld | $(BUILD_QEMU)
	$(QEMU_AARCH64_TOOLPREFIX)gcc -c -DARM_ENTRY=$(QEMU_ARM_ENTRY) -o build-qemu/armstub64.o armstub64.S
	$(QEMU_AARCH64_TOOLPREFIX)ld -T armstub64.ld -o build-qemu/armstub64.elf build-qemu/armstub64.o
	$(QEMU_AARCH64_TOOLPREFIX)objcopy build-qemu/armstub64.elf -O binary armstub64.bin

qemu: kernel7-qemu.bin armstub64.bin
	@echo "Press Ctrl-A and then X to terminate QEMU session"
	$(QEMU) -M raspi3b -smp 4 -nographic -kernel armstub64.bin \
		-device loader,file=kernel7-qemu.bin,addr=$(QEMU_ARM_ENTRY),force-raw=on

# claude: same build and same board, but WITHOUT -nographic, so QEMU opens
# a real GTK window backed by the emulated bcm2835 framebuffer this
# kernel's own gpuinit()/gpuputc() (source/console.c) draw into - cprintf
# writes every character to BOTH the framebuffer and the UART, so the
# window and the serial console show the same session. "-serial mon:stdio"
# keeps that serial console (and the QEMU monitor) on the invoking
# terminal rather than moving it into the window. "-device usb-kbd"
# attaches a real emulated USB HID keyboard for the uspi stack to
# enumerate - same shape as forks/arm-pi1's and forks/arm-pi1-bis's own
# qemu-graphics targets. Needs $DISPLAY; deliberately not part of any
# test/-all target (nothing headless can assert against a real window -
# scripts/test_qemu_graphics.py is what drives it automatically, via the
# optional QMP_SOCK below).
#
# claude: "comma"/QMP_OPTS is the standard Make idiom for getting a
# literal "," into an "$(if ...)" call, which cannot contain an
# unescaped one - copied verbatim from forks/arm-pi1/Makefile.
comma := ,
QMP_OPTS := $(if $(QMP_SOCK),-qmp unix:$(QMP_SOCK)$(comma)server$(comma)nowait)

# claude: this kernel asks the firmware for a 1280x1024 framebuffer
# (source/console.c's own gpuinit()), which is what a real Pi 3 on a real
# monitor gets - so the resolution itself is NOT the thing to change; a
# smaller one would only be right for QEMU and wrong on the board.
# Without "zoom-to-fit" the GTK window opens at the guest's full
# 1280x1024 and REFUSES to shrink below it, so on a smaller desktop the
# bottom of the console ends up off-screen with no way to get at it.
# "zoom-to-fit=on" lets the window be resized freely and scales the guest
# display into whatever size it is - the whole console stays visible.
# Override on the command line (GTK_DISPLAY=gtk) for an unscaled,
# pixel-exact window.
GTK_DISPLAY ?= gtk$(comma)zoom-to-fit=on

qemu-graphics: kernel7-qemu.bin armstub64.bin
	@echo "Serial console + QEMU monitor share this terminal - Ctrl-A C to switch to the monitor (try: screendump /tmp/fb.ppm), Ctrl-A C again to switch back"
	@echo "Ctrl-A X (from the monitor side) to terminate"
	@echo "Click the QEMU window and type there for a real USB-keyboard test - the terminal keeps working as the serial console too"
	$(QEMU) -M raspi3b -smp 4 -kernel armstub64.bin \
		-device loader,file=kernel7-qemu.bin,addr=$(QEMU_ARM_ENTRY),force-raw=on \
		-serial mon:stdio -device usb-kbd -display $(GTK_DISPLAY) $(QMP_OPTS)

$(BUILD_QEMU)%.o: $(SOURCE)%.S | $(BUILD_QEMU)
	$(TOOLCHAIN)gcc -c $(CFLAGS) $(QEMU_CC_OPTIONS) -I kernel $< -o $@

$(BUILD_QEMU)%.o: $(SOURCE)%.c | $(BUILD_QEMU)
	$(TOOLCHAIN)gcc -c $(CFLAGS) $(QEMU_CC_OPTIONS) $< -o $@

# claude: order-only prerequisite ("| $(BUILD_QEMU)") on every rule that
# writes into this directory, and "mkdir -p" rather than "mkdir". Both
# matter: as a NORMAL prerequisite the directory's own mtime counts, and
# it changes every time a file is written into it - so make can decide
# the directory is "out of date" relative to already-built objects and
# re-run this recipe mid-build, which plain "mkdir" then fails on. An
# order-only prerequisite means "make sure this exists first" without
# comparing timestamps, which is the only thing actually wanted here.
$(BUILD_QEMU):
	mkdir -p $@

# Build ASM files
$(BUILD)%.o: $(SOURCE)%.S | $(BUILD)
	$(TOOLCHAIN)gcc -c $(CFLAGS) $(CC_OPTIONS) -I kernel $< -o $@

# Build C files
$(BUILD)%.o: $(SOURCE)%.c | $(BUILD)
	$(TOOLCHAIN)gcc -c $(CFLAGS) $(CC_OPTIONS) $<  -o $@

# claude: kernel/fs.img is GENERATED, not checked in - the output of uprogs/'s
# own build (mkfs plus every UPROGS binary). It used to be a committed 512KB
# blob, which is precisely how uprogs/ here came to be unbuildable without
# anyone noticing: bringing it up needed three real fixes (CFLAGS that asked
# for an FPU-less -march while the toolchain defaulted to hard-float, fs.h's
# NDIRECT too small for a modern-toolchain usertests, and mkfs.c's own block
# budget rebalanced to match). A checked-in image hides that kind of bit-rot
# indefinitely and can silently drift from the uprogs/*.c it represents.
# Same treatment and rule shape as the sibling forks/arm-pi2/Makefile.
#
# The phony target is "uprogs-fs-img", NOT "kernel/fs.img": a phony
# prerequisite of a REAL file makes that file's recipe run unconditionally
# (GNU make treats it as always newer), which would re-assemble entry.S and
# re-link the kernel on every build. This way the recursive make still runs
# every time and does its own up-to-date checking, while kernel/fs.img's
# timestamp only moves when the image content actually changed.
.PHONY: uprogs-fs-img
uprogs-fs-img:
	$(MAKE) -C user TOOLPREFIX=$(TOOLCHAIN) fs.img

kernel/fs.img: uprogs-fs-img
	cmp -s user/fs.img $@ || cp user/fs.img $@

# entry.S is the only source file that embeds the image (its own
# .incbin "fs.img"), so only ITS object needs the dependency - but this fork
# builds entry.S TWICE, into two separate object trees: build/ for the real
# Raspberry Pi 3 target and build-qemu/ (see BUILD_QEMU above) for the
# kernel7-qemu.bin that "make qemu"/test-xv6.py actually boot. Both need it.
# Naming only $(BUILD)entry.o leaves the QEMU kernel embedding a STALE image
# while the rest of the kernel is compiled against the current fs.h - which
# boots as far as the scheduler and then hangs with no shell and no panic,
# because mkfs wrote one on-disk layout and the kernel reads another.
$(BUILD)entry.o: kernel/fs.img
$(BUILD_QEMU)entry.o: kernel/fs.img

loader: loader.S kernel7.bin
	$(TOOLCHAIN)gcc -c loader.S -o loader.elf -fpic -ffreestanding -nostdlib -nostartfiles -O0 -Wall -ggdb -Wall -mcpu=cortex-a7 -mfloat-abi=hard -fno-short-enums -o loader.o
	$(TOOLCHAIN)ld loader.o -o loader.elf -T loader.ld
	#$(TOOLCHAIN)objdump -D loader.elf > loader.list
	$(TOOLCHAIN)objcopy loader.elf -O binary kernel7.img
	-rm loader.elf  loader.o


# claude: pull in the per-object header dependencies gcc already writes.
# CFLAGS above has carried "-MD" all along, so every compile emits a
# "$(BUILD)<name>.d" listing the headers that object really includes -
# but nothing ever read them, so editing a header rebuilt NOTHING and an
# incremental build was quietly wrong. That is what made forks/arm-pi3
# test-xv6.py reach for a "make clean" before every test run (a
# from-scratch build being the only way to be sure), which in turn made
# "make test-all" recompile this port from scratch even right after a
# full "make build-all".
#
# "-include" (leading dash) ignores the files when they do not exist yet,
# i.e. on the very first build, when there is nothing to be stale about.
# This is the same mechanism forks/riscv64, forks/loongarch, forks/mips
# and forks/arm64-pi4 already use, and the modern replacement for an
# explicit "make depend" pass: the deps are produced by the same compile
# that produces the object, so they can never go stale.
-include $(BUILD)*.d $(BUILD_QEMU)*.d

$(BUILD):
	mkdir -p $@

.PHONY: report
report:
	@echo 'Hardware  :' $(hw)
	@echo 'TARGET    :' $(TARGET)
	@echo 'PHYSTART  :' $(PHYSTART)
	@echo 'PHYSIZE   :' $(PHYSIZE)
	
	@echo 'K_PDX_BASE:' $(K_PDX_BASE)
	@echo 'K_PTX_BASE:' $(K_PTX_BASE)
	@echo 'PHYSOFFSET:' $(PHYSOFFSET)
	
	@echo 'KERNBASE  :' $(KERNBASE)
	@echo 'KERNOFFSET:' $(KERNOFFSET)

	@echo 'MMIO_PA   :' $(MMIO_PA)
	@echo 'MMIO_SIZE :' $(MMIO_SIZE)
	@echo 'MMIO_VA   :' $(MMIO_VA)
	@echo 'PERIPHBASE:' $(PERIPHBASE)
	
	@echo 'CFLAGS    :' $(CFLAGS)
	@echo 'CC_OPTIONS:' $(CC_OPTIONS)
	@echo 'LD_OPTIONS:' $(LD_OPTIONS)

.PHONY: install
install:
	cp kernel*.img /media/$(USER)/boot/
	sync
	umount /media/$(USER)/*
	
# Rule to clean files.
#
# claude: real bug, not hypothetical - see forks/arm-pi1/Makefile's own
# identical comment on its own "$(MAKE) -C user clean" line, added
# alongside this one for the same reason. This is the confirmed cause of
# a real "ls" regression here: a stale, pre-header-merge user/ls.o
# survived both a plain "make run-arm-pi3" and a "make clean-arm-pi3 &&
# make build-arm-pi3", because neither this clean nor user/Makefile's
# own dependency tracking (now fixed alongside this - see its own
# "-include $(wildcard *.d)") ever reached it.
clean :
	-rm -rf $(BUILD)
	-rm -rf $(BUILD_QEMU)
	-rm -f *.img
	-rm -f *.bin
	-rm -f kernel.list
	-rm -f kernel.map
	-rm -f kernel-qemu.map
	$(MAKE) -C user clean
