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
	CFLAGS    =   -ffreestanding -nostdlib -nostartfiles -fcommon -marm -fno-pic -fno-pie -O0 -Wall -MD -ggdb -Wall -mcpu=cortex-a9 -mfloat-abi=hard -fno-short-enums -I include
	#CFLAGS    =   -ffreestanding -nostdlib -nostartfiles -O2 -Wall -MD -ggdb -Wall -mcpu=cortex-a9 -mfloat-abi=hard -fno-short-enums -I include
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
	#CFLAGS   = -fno-pic -static -Wno-packed-bitfield-compat -fno-builtin -fno-strict-aliasing -fshort-wchar -O2 -Wall -MD -ggdb -Werror -fno-omit-frame-pointer -fno-stack-protector -Wa,-march=armv6 -Wa,-mcpu=arm1176jzf-s -mfloat-abi=hard -fno-short-enums -I include
	# claude: -fno-pic -fno-pie restored from the commented-out
	# alternate CFLAGS line right above (which already had -fno-pic -
	# real evidence someone hit this before) - see the rpi2 block's own
	# long comment.
	CFLAGS    =   -ffreestanding -nostdlib -nostartfiles -fcommon -marm -fno-pic -fno-pie -O2 -Wall -MD -ggdb -Wall -mcpu=arm1176jzf-s -mfloat-abi=hard -fno-short-enums -I include
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
	CFLAGS    = -ffreestanding -nostdlib -nostartfiles -fcommon -marm -fno-pic -fno-pie -O2 -Wall -MD -std=gnu99 -ggdb -Wall -mcpu=cortex-a7 -mfloat-abi=hard -fno-short-enums -fno-zero-initialized-in-bss -I include
	#CFLAGS    = -ffreestanding -nostdlib -nostartfiles -O2 -Wall -MD -ggdb -Wall -mcpu=cortex-a7 -mfloat-abi=hard -fno-short-enums -I include
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
SOURCE=source/
# Generated listing file.
LIBRARIES=

# The names of all object files that must be generated. Deduced from the 
# assembly code files in source.
ASM_OBJECTS = $(patsubst $(SOURCE)%.S,$(BUILD)%.o,$(wildcard $(SOURCE)*.S))

C_OBJECTS = $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))

# Rule to make everything.
all : $(TARGET)

# Rule to make the elf file.
$(TARGET): $(ASM_OBJECTS) $(C_OBJECTS) kernel.ld
	$(TOOLCHAIN)ld $(ASM_OBJECTS) $(C_OBJECTS) -L. $(patsubst %,-l %,$(LIBRARIES)) $(LD_OPTIONS) -Map kernel.map -o $(BUILD)kernel.elf -T kernel.ld
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

kernel7-qemu.bin: $(ASM_OBJECTS_QEMU) $(C_OBJECTS_QEMU) kernel.ld
	$(TOOLCHAIN)ld $(ASM_OBJECTS_QEMU) $(C_OBJECTS_QEMU) -L. $(patsubst %,-l %,$(LIBRARIES)) $(QEMU_LD_OPTIONS) -Map kernel-qemu.map -o $(BUILD_QEMU)kernel-qemu.elf -T kernel.ld
	$(TOOLCHAIN)objcopy $(BUILD_QEMU)kernel-qemu.elf -O binary kernel7-qemu.bin

armstub64.bin: armstub64.S armstub64.ld $(BUILD_QEMU)
	$(QEMU_AARCH64_TOOLPREFIX)gcc -c -DARM_ENTRY=$(QEMU_ARM_ENTRY) -o build-qemu/armstub64.o armstub64.S
	$(QEMU_AARCH64_TOOLPREFIX)ld -T armstub64.ld -o build-qemu/armstub64.elf build-qemu/armstub64.o
	$(QEMU_AARCH64_TOOLPREFIX)objcopy build-qemu/armstub64.elf -O binary armstub64.bin

qemu: kernel7-qemu.bin armstub64.bin
	@echo "Press Ctrl-A and then X to terminate QEMU session"
	$(QEMU) -M raspi3b -smp 4 -nographic -kernel armstub64.bin \
		-device loader,file=kernel7-qemu.bin,addr=$(QEMU_ARM_ENTRY),force-raw=on

$(BUILD_QEMU)%.o: $(SOURCE)%.S $(BUILD_QEMU)
	$(TOOLCHAIN)gcc -c $(CFLAGS) $(QEMU_CC_OPTIONS) -I source $< -o $@

$(BUILD_QEMU)%.o: $(SOURCE)%.c $(BUILD_QEMU)
	$(TOOLCHAIN)gcc -c $(CFLAGS) $(QEMU_CC_OPTIONS) $< -o $@

$(BUILD_QEMU):
	mkdir $@

# Build ASM files
$(BUILD)%.o: $(SOURCE)%.S $(BUILD)
	$(TOOLCHAIN)gcc -c $(CFLAGS) $(CC_OPTIONS) -I source $< -o $@

# Build C files
$(BUILD)%.o: $(SOURCE)%.c $(BUILD)
	$(TOOLCHAIN)gcc -c $(CFLAGS) $(CC_OPTIONS) $<  -o $@

loader: loader.S kernel7.bin
	$(TOOLCHAIN)gcc -c loader.S -o loader.elf -fpic -ffreestanding -nostdlib -nostartfiles -O0 -Wall -ggdb -Wall -mcpu=cortex-a7 -mfloat-abi=hard -fno-short-enums -o loader.o
	$(TOOLCHAIN)ld loader.o -o loader.elf -T loader.ld
	#$(TOOLCHAIN)objdump -D loader.elf > loader.list
	$(TOOLCHAIN)objcopy loader.elf -O binary kernel7.img
	-rm loader.elf  loader.o

$(BUILD):
	mkdir $@

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
clean :
	-rm -rf $(BUILD)
	-rm -rf $(BUILD_QEMU)
	-rm -f *.img
	-rm -f *.bin
	-rm -f kernel.list
	-rm -f kernel.map
	-rm -f kernel-qemu.map
