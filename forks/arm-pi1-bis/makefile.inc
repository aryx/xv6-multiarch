# Cross-compiling (e.g., on Mac OS X, install arm-none-eabi-gcc with MacPorts)
CROSSCOMPILE := arm-none-eabi-
# try linux eabi
# CROSSCOMPILE := arm-linux-gnueabi-
CC = $(CROSSCOMPILE)gcc
AS = $(CROSSCOMPILE)as
LD = $(CROSSCOMPILE)ld
OBJCOPY = $(CROSSCOMPILE)objcopy
OBJDUMP = $(CROSSCOMPILE)objdump

#CFLAGS = -march=armv7-a -mtune=cortex-a15 -fno-pic -static -fno-builtin -fno-strict-aliasing -Wall -Werror -I. -g
# claude: added -fcommon - proc.h declares "struct cpu cpus[NCPU];" (no
# extern) and gets #included into every .c file, which used to work
# because GCC defaulted to -fcommon (tentative definitions across
# translation units get merged by the linker as a "common" symbol) - GCC
# 10 flipped the default to -fno-common, so every one of those
# translation units now provides its own real, non-mergeable definition
# of "cpus", and the link fails with "multiple definition of `cpus`".
# -fcommon restores the old behavior this 2017-era code was written
# against, rather than restructuring proc.h's own declaration.
#
# claude: replaced the old "-Wa,-march=armv6 -Wa,-mcpu=arm1176jzf-s"
# (which ONLY restricts what the assembler *stage* accepts - it does
# nothing to tell gcc's own code generator what to target) with real
# gcc-level "-march=armv6 -mcpu=arm1176jzf-s -marm". Without this, gcc
# silently used its own toolchain default instead - this
# arm-linux-gnueabihf- cross-gcc defaults to "-march=armv7-a+fp
# -mfloat-abi=hard" (confirmed via "gcc -Q --help=target") - and the
# real, observed consequence: entry.s's "bl mmuinit0" call into mmu.c
# got linked as a Thumb-switching BLX instruction, then QEMU's -cpu
# arm1176 (a true ARMv6 core with no Thumb-2 support) hit an actual
# ARMv7/Thumb-2-only instruction inside the resulting code and faulted
# with a repeating "Undefined Instruction" trap at PC 0x00000004 (traced
# via "-d int,guest_errors" + "-d in_asm" - see
# notes_arch_armv6_rpi.txt). "-marm" additionally forces plain ARM
# (not Thumb) instruction generation outright, removing the
# state-switch question entirely - simplest, most robust fix for a
# freestanding kernel with no reason to ever be in Thumb state.
# claude: -I../../kernel - kernel/spinlock.h is shared there (kernel-only,
# so top-level kernel/ rather than include/kernel/).
CFLAGS = -fno-pic -static -fcommon -Wno-packed-bitfield-compat -fno-builtin -fno-strict-aliasing -fshort-wchar -O2 -Wall -MD -MP -ggdb -Werror -fno-omit-frame-pointer -fno-stack-protector -mcpu=arm1176jzf-s -marm -mfloat-abi=soft -I../../include -I../../include/kernel -I../../include/user -I../../kernel -I../../include/arch/arm
LDFLAGS = -L.
# claude: added -mfloat-abi=soft - this modern arm-linux-gnueabihf- cross
# toolchain defaults to "-march=armv7-a+fp -mfloat-abi=hard" (confirmed
# via "gcc -Q --help=target"), and plain "-march=armv6" alone (no -mcpu,
# no float-abi override) has no guaranteed FPU, so gcc refused to build
# initcode.S at all ("selected architecture lacks an FPU") - this is the
# only file actually compiled with ASFLAGS (see makefile.inc's own
# "build/%.o: %.S" rule); every .c file goes through CFLAGS instead,
# which happens to avoid this specific error already (see
# notes_arch_armv6_rpi.txt for the fuller picture: CFLAGS's own
# "-Wa,-march=armv6" only restricts what the assembler *stage* accepts,
# not gcc's own codegen target, so those files are still actually
# code-generated for the toolchain's armv7-a+hard-float default - this
# currently works only because none of this simple freestanding C code
# happens to emit an armv7-only instruction or touch floating point).
# claude: -mcpu=arm1176jzf-s -marm added for the same reason as CFLAGS
# above - consistent target between hand-written .S/.s files and C code.
# claude: -Ikernel added - wrapper.s does .incbin "font1.bin", and both
# moved into kernel/ in Phase 0, so the assembler needs to be told where to
# look. Without it: 'kernel/wrapper.s: Error: file not found: font1.bin'.
ASFLAGS = -mcpu=arm1176jzf-s -marm -mfloat-abi=soft -Ikernel -I../../include/kernel

LIBGCC = $(shell $(CC) -print-libgcc-file-name)

# host compiler
HOSTCC_preferred = gcc
define get_hostcc
    $(if $(shell which $(HOSTCC_preferred)),$(HOSTCC_preferred),"cc")
endef
HOSTCC := $(call get_hostcc)

# general rules
quiet-command = $(if $(V),$1,$(if $(2),@echo $2 && $1, @$1))

LINK_BIN = $(call quiet-command,$(LD) $(LDFLAGS) \
	-T $(1) -o $(2) $(3) $(LIBS) -b binary $(4), "  LINK     $(TARGET_DIR)$@")

LINK_INIT = $(call quiet-command,$(LD) $(LDFLAGS) \
	$(1) -o $@.out $<, "  LINK     $(TARGET_DIR)$@")

OBJCOPY_INIT = $(call quiet-command,$(OBJCOPY) \
	-S -O binary --prefix-symbols="_binary_$@" $@.out $@, "  OBJCOPY  $(TARGET_DIR)$@")

# claude: kernel objects all land in build/kernel/ since Phase 0; this
# fork never had device/ or lib/ directories to begin with.
build-directory = $(shell mkdir -p build build/kernel)

build/%.o: %.c
	$(call build-directory)
	$(call quiet-command,$(CC) $(CFLAGS) \
		-c -o $@ $<,"  CC       $(TARGET_DIR)$@")

AS_WITH = $(call quiet-command,$(CC) $(ASFLAGS) \
		$(1) -c -o $@ $<,"  AS       $(TARGET_DIR)$@")

build/%.o: %.S
	$(call build-directory)
	$(call AS_WITH, )

build/%.o: %.s
	$(call build-directory)
	$(call AS_WITH, )

