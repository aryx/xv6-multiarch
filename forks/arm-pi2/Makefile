UNAME := $(shell uname -m)

ifeq ($(UNAME), x86_64)
	# set FVP as default on x86_64 systems	
	hw?=rpi2
	TOOLCHAIN?=~/gcc-arm-none-eabi-5_4-2016q3/bin/arm-none-eabi-
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
	# claude: -marm added - this toolchain's default is Thumb-2, but
	# kernel/entry.S (hand-written assembly, all three hw= targets
	# share it) uses plain "bl"/"b" to call into C code with no
	# ARM<->Thumb interworking, so the C side must stay ARM too. See
	# the rpi2 block's own longer comment below for the full story
	# (found via QEMU/gdb; this fvp block's own bug is untested but
	# shares the identical root cause). -fno-pic -fno-pie added for the
	# same reason, also documented at length in the rpi2 block below.
	CFLAGS    =   -ffreestanding -nostdlib -nostartfiles -O0 -Wall -MD -MP -ggdb -Wall -marm -fno-pic -fno-pie -mcpu=cortex-a9 -mfloat-abi=hard -fno-short-enums -I kernel -I../../include -I../../include/kernel -I../../include/user -I../../kernel
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
	# claude: -marm added, same reason as the fvp block above. Also
	# restored "-fno-pic" from the commented-out alternate CFLAGS line
	# right above (plus "-fno-pie", same reason) - see the rpi2 block's
	# own long comment below for why an active kernel build needs both.
	CFLAGS    =   -ffreestanding -nostdlib -nostartfiles -O2 -Wall -MD -MP -ggdb -Wall -marm -fno-pic -fno-pie -mcpu=arm1176jzf-s -mfloat-abi=hard -fno-short-enums -I kernel -I../../include -I../../include/kernel -I../../include/user -I../../kernel
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
	# claude: -marm added - this arm-linux-gnueabihf- toolchain
	# defaults to Thumb-2 (confirmed via "readelf -s build/kernel-qemu.elf":
	# every C function symbol - cmain, uartinit, mmuinit0, etc - had its
	# low bit set, the ELF convention for a Thumb interworking target),
	# but kernel/entry.S is hand-written pure ARM assembly whose
	# "bl cmain" doesn't perform ARM<->Thumb interworking (unlike a
	# "blx", "bl" never changes CPSR.T). The CPU stayed in ARM state
	# after jumping into cmain() and executed Thumb-2 instruction bytes
	# as if they were 32-bit ARM instructions - mostly harmless-looking
	# garbage for a surprisingly long stretch (mmuinit0()'s 256-
	# iteration loop and several later functions still appeared to
	# "complete" under a gdb breakpoint sweep, since a breakpoint only
	# checks the PC value, not whether the CPU decoded its way there
	# correctly) until a genuinely undecodable bit pattern was hit
	# inside uartinit(), producing a repeating "Undefined Instruction"
	# exception loop with zero console output - see
	# notes_arch_arm_pi2.txt's own "Bug 2" for the full gdb-based
	# diagnosis. This is a real ISA-correctness bug, not a QEMU quirk -
	# real hardware would hit the identical fault - so -marm (make the
	# whole C codebase ARM, matching entry.S, rather than patching every
	# asm-to-C call site with "blx") is a straightforward, safe fix for
	# both targets, matching how forks/arm-pi1's own toolchain already
	# defaults to (or is pinned to) ARM mode.
	#
	# claude: -fno-pic -fno-pie added too - a separate bug from the
	# Thumb one above, found while chasing a later "popcli" kernel
	# panic (see notes_arch_arm_pi2.txt's own "Bug/gap 7", now Bug 8).
	# This toolchain defaults to PIC/PIE codegen for ordinary C code:
	# every global-variable access (not just this fork's own "cpus[]")
	# compiled as a double-indirect load through a GOT-style table -
	# "ldr r6, [pc, #N]; add r6, pc, r6; ldr r3, [pc, #M]; ldr r6, [r6,
	# r3]" (confirmed via "objdump -d" on acquire()'s own compiled
	# code) - instead of a direct absolute-address load. GOT-indirect
	# addressing assumes a dynamic loader populates that table at
	# startup; this bare-metal kernel has no loader and never
	# populates any GOT, so every such access silently reads/writes
	# through whatever garbage happens to occupy that memory instead
	# of the real variable - here, "curr_cpu->ncli" (spinlock.c's own
	# pushcli/popcli nesting counter) always read back 0 no matter how
	# many times pushcli() had incremented it, since pushcli() and
	# popcli() were two independently-broken GOT lookups that just
	# happened to each resolve to SOME zeroed memory rather than the
	# real, same cpus[0].ncli field - popcli()'s own underflow check
	# ("if(--curr_cpu->ncli < 0) panic("popcli")") then fired on the
	# very first release() call. "-fno-pic -fno-pie" restores direct
	# "movw/movt" absolute addressing (confirmed via the same objdump
	# comparison) - notably, forks/arm-pi2's own commented-out
	# alternate CFLAGS line in the rpi1 block above already had
	# "-fno-pic" in it, evidence the original author hit this same
	# class of bug at some point; it just wasn't in the ACTIVE CFLAGS
	# line any of the three hw= blocks actually build with. A real
	# ISA/ABI-correctness bug affecting every global variable in the
	# kernel, not a QEMU quirk - real hardware would behave identically
	# (or worse: whatever happened to be in that unpopulated GOT-shaped
	# memory on real silicon is not guaranteed to even be zero).
	CFLAGS    = -ffreestanding -nostdlib -nostartfiles -O2 -Wall -MD -MP -ggdb -Wall -marm -fno-pic -fno-pie -mcpu=cortex-a7 -mfloat-abi=hard -fno-short-enums -I kernel -I../../include -I../../include/kernel -I../../include/user -I../../kernel
	TARGET    = kernel7.bin
	CC_OPTIONS = -DRPI2
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

# claude: "all" MUST stay the first real target rule in this file - it's
# what a bare "make hw=rpi2 TOOLCHAIN=..." (no explicit target, exactly
# what the top-level Makefile's own build-arm-pi2 does) falls back to as
# Make's own implicit default goal. The kernel/fs.img rule below was
# originally placed HERE instead and silently became the new default
# goal - bare "make" started stopping after just regenerating fs.img,
# never actually building the kernel at all. Kept below "all" instead.
# Rule to make everything.
all : $(TARGET)

# Rule to make the elf file.
$(TARGET): $(ASM_OBJECTS) $(C_OBJECTS) kernel/kernel.ld
	$(TOOLCHAIN)ld $(ASM_OBJECTS) $(C_OBJECTS) -L. $(patsubst %,-l %,$(LIBRARIES)) $(LD_OPTIONS) -Map kernel.map -o $(BUILD)kernel.elf -T kernel/kernel.ld
	$(TOOLCHAIN)objdump -d $(BUILD)kernel.elf > kernel.list
	$(TOOLCHAIN)objcopy $(BUILD)kernel.elf -O binary $(TARGET)

# claude: kernel/fs.img is not checked into git - it's the OUTPUT of
# user/'s own separate build (mkfs + every UPROGS binary), regenerated
# here on every build rather than committed as a binary blob that could
# silently drift from user/*.c (see notes_arch_arm_pi2.txt's own
# "Bug 11"/"Bug 12" - this repo's own user/ build already needed real
# fixes once actually exercised, exactly the kind of bit-rot a
# checked-in fs.img would hide indefinitely). user/'s own Makefile
# names its toolchain variable "TOOLPREFIX" (not this Makefile's own
# "TOOLCHAIN") - passed through under its own name.
#
# Marked ".PHONY" (rather than listing every user/*.c as a real
# prerequisite, which would drift from user/Makefile's own file list)
# so this recipe runs on every build unconditionally - up-to-date
# checking is delegated to user/'s own recursive "make fs.img" (its
# own objects are individually timestamp-tracked via "-MD"), which is a
# no-op print ("'fs.img' is up to date.") when nothing changed there,
# rather than trying to duplicate that tracking here.
#
# claude: the phony target is "uprogs-fs-img", NOT "kernel/fs.img"
# itself. A phony target used as a prerequisite of a REAL file makes
# that file's recipe run unconditionally (GNU make treats a phony
# prerequisite as always newer), so with ".PHONY: kernel/fs.img" the
# $(BUILD)entry.o dependency below re-assembled entry.S and re-linked
# the whole kernel on EVERY build even when nothing had changed - the
# unconditional "cp" bumped kernel/fs.img's mtime to match. Delegating
# the up-to-date check to user/ still works exactly as described
# above (the recursive make still runs every time); the difference is
# that kernel/fs.img's own timestamp now only moves when the image
# CONTENT actually changed, so entry.o rebuilds only when it must.
.PHONY: uprogs-fs-img
uprogs-fs-img:
	$(MAKE) -C user TOOLPREFIX=$(TOOLCHAIN) fs.img

kernel/fs.img: uprogs-fs-img
	cmp -s user/fs.img $@ || cp user/fs.img $@

# entry.S is the only source file that actually embeds fs.img (via its
# own ".incbin \"fs.img\"", kernel/entry.S) - only ITS object needs the
# dependency, not every .S file the generic pattern rule below covers.
$(BUILD)entry.o: kernel/fs.img

# claude: QEMU-only target, added alongside the real-hardware ones above
# rather than replacing anything - "all"/"$(TARGET)" (what a real
# Raspberry Pi 2/3 boots from an SD card as "kernel7.bin") are completely
# unchanged. Relinks the exact same $(ASM_OBJECTS)/$(C_OBJECTS) - same
# source, same CFLAGS/CC_OPTIONS, only the LOAD ADDRESS differs - against
# the same kernel.ld (already fully parameterized via PHYSOFFSET/
# KERNOFFSET --defsym symbols, so no separate linker script is needed
# here unlike the sibling forks/arm-pi1 fork). QEMU's "-M raspi2b" needs the
# image placed at physical 0x10000, not the real-hardware 0x8000
# (confirmed for the same board via forks/arm's own bring-up, forks/
# armv7-rpi at the time - see notes_arch_arm.txt bug 7 - and
# re-confirmed for this specific kernel image below). Every other
# --defsym (MMIO_PA/MMIO_VA/PHYSTART/
# PHYSIZE/KERNBASE/...) stays identical to the real-hardware rpi2
# build - only the load offset changes.
QEMU ?= qemu-system-arm
QEMU_OFFSET = 0x10000
QEMU_LD_OPTIONS = --defsym=PHYSTART=$(PHYSTART) --defsym=PHYSIZE=$(PHYSIZE) --defsym=KERNBASE=$(KERNBASE) --defsym=MMIO_PA=$(MMIO_PA) --defsym=MMIO_VA=$(MMIO_VA) --defsym=MMIO_SIZE=$(MMIO_SIZE) --defsym=PERIPHBASE=$(PERIPHBASE)
QEMU_LD_OPTIONS += --defsym=K_PDX_BASE=$(K_PDX_BASE) --defsym=K_PTX_BASE=$(K_PTX_BASE)
QEMU_LD_OPTIONS += --defsym=PHYSOFFSET=$(shell printf "0x%X\n" $$(( $(PHYSTART) + $(QEMU_OFFSET) )) )
QEMU_LD_OPTIONS += --defsym=KERNOFFSET=$(shell printf "0x%X\n" $$(( $(KERNBASE) + $(QEMU_OFFSET) )) )

kernel7-qemu.bin: $(ASM_OBJECTS) $(C_OBJECTS) kernel/kernel.ld
	$(TOOLCHAIN)ld $(ASM_OBJECTS) $(C_OBJECTS) -L. $(patsubst %,-l %,$(LIBRARIES)) $(QEMU_LD_OPTIONS) -Map kernel-qemu.map -o $(BUILD)kernel-qemu.elf -T kernel/kernel.ld
	$(TOOLCHAIN)objcopy $(BUILD)kernel-qemu.elf -O binary kernel7-qemu.bin

qemu: kernel7-qemu.bin
	@echo "Press Ctrl-A and then X to terminate QEMU session"
	$(QEMU) -M raspi2b -nographic -kernel kernel7-qemu.bin

# Build ASM files
# claude: order-only "|" on $(BUILD) - see forks/arm-pi1/Makefile's own
# copy of this comment: as a normal prerequisite the build directory's
# mtime, bumped by writing the first .o into it, makes every later make
# rebuild the entire kernel. forks/arm-pi3/Makefile already had it right.
$(BUILD)%.o: $(SOURCE)%.S | $(BUILD)
	$(TOOLCHAIN)gcc -c $(CFLAGS) $(CC_OPTIONS) -I kernel $< -o $@

# Build C files
$(BUILD)%.o: $(SOURCE)%.c | $(BUILD)
	$(TOOLCHAIN)gcc -c $(CFLAGS) $(CC_OPTIONS) $<  -o $@

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
# incremental build was quietly wrong. That is what made forks/arm-pi2
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
-include $(BUILD)*.d

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
#
# claude: real bug, not hypothetical - see forks/arm-pi1/Makefile's own
# identical comment on its own "$(MAKE) -C user clean" line, added
# alongside this one for the same reason (this fork's own user/ had the
# exact same untracked-staleness gap - see user/Makefile's own
# "-include $(wildcard *.d)").
clean :
	-rm -rf $(BUILD)
	-rm -f *.img
	-rm -f *.bin
	-rm -f kernel.list
	-rm -f kernel.map
	-rm -f kernel-qemu.map
	$(MAKE) -C user clean
