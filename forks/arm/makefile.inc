# Cross-compiling (e.g., on Mac OS X, install arm-none-eabi-gcc with MacPorts)
CROSSCOMPILE := arm-none-eabi-
# try linux eabi
# CROSSCOMPILE := arm-linux-gnueabi-
CC = $(CROSSCOMPILE)gcc
AS = $(CROSSCOMPILE)as
LD = $(CROSSCOMPILE)ld
OBJCOPY = $(CROSSCOMPILE)objcopy
OBJDUMP = $(CROSSCOMPILE)objdump

# claude: added -mfloat-abi=soft to both - this modern arm-linux-gnueabihf-
# cross toolchain defaults to "-mfloat-abi=hard" (confirmed via "gcc -Q
# --help=target"; its own real -march default is "armv7-a+fp", note the
# "+fp"), but this kernel's own explicit "-march=armv7-a" (no "+fp"
# suffix) drops that implied FPU extension back out - gcc then refuses
# the still-active hard-float default outright ("selected architecture
# lacks an FPU"). Same class of bug as notes_arch_armv6_rpi.txt's own
# bug 1/4 in the sibling port; soft-float is fine for a freestanding
# kernel that never touches floating point.
# claude: added -fno-stack-protector - modern gcc defaults to
# -fstack-protector-strong, inserting __stack_chk_guard/__stack_chk_fail
# calls this freestanding, libc-less environment has no definitions for
# (real linker errors: "undefined reference to `__stack_chk_fail'" from
# any function with a local array/buffer, e.g. usr/ulib.c's gets()).
# claude: added -marm - without it, this toolchain's default for
# "-march=armv7-a" is Thumb(-2) code generation, and the linker then
# auto-generates ARM<->Thumb interworking veneers ("__kmain_veneer" etc)
# for every call crossing the boundary from entry.S's own hand-written
# plain-ARM assembly (".code 32") into C code. One such veneer
# ("__start_from_arm" -> "__kmain_veneer") loads kmain's address WITH
# the Thumb bit set (0x80022e69) even though kmain's own ELF symbol
# value is even/ARM-mode (0x80022e68) - a genuine ARM/Thumb mismatch
# that sends the CPU into Thumb state trying to execute ARM-encoded
# instructions, causing a real, reproduced Undefined Instruction crash
# right at kernel entry (confirmed via "-d in_asm" plus "nm"/objdump
# cross-referencing - see notes_arch_armv7_rpi.txt). "-marm" removes
# the Thumb/interworking question entirely by making every C function
# plain ARM, matching entry.S's own mode and matching what already
# avoided this class of bug in the sibling forks/rpi2 port (whose own
# CFLAGS happens to default to ARM for its own -mcpu=cortex-a7 choice).
# claude: -MD added - this was the ONE wired-up ARM port generating no
# header dependencies at all, so touching a header rebuilt nothing and
# an incremental build was quietly wrong. See
# docs/claude_notes/notes_build_system.txt. The .d lands next to the
# object (build/foo.d for build/foo.o) and is read back by the
# "-include" at the bottom of Makefile.
CFLAGS = -march=armv7-a -mtune=cortex-a7 -fno-pic -static -fno-builtin -fno-strict-aliasing -Wall -Werror -Ikernel -g -mfloat-abi=soft -fno-stack-protector -marm -MD -MP -I../../include -I../../include/user
LDFLAGS = -L.
#ASFLAGS = -march=armv7-a
ASFLAGS = -mcpu=cortex-a7 -mfloat-abi=soft -MD -MP

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

# claude: kernel objects all land in build/kernel/ now that lib/ and
# device/ have been folded into a flat kernel/ (Phase 0).
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
