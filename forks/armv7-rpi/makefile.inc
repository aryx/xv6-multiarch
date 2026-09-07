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
CFLAGS = -march=armv7-a -mtune=cortex-a7 -fno-pic -static -fno-builtin -fno-strict-aliasing -Wall -Werror -I. -g -mfloat-abi=soft -fno-stack-protector
LDFLAGS = -L.
#ASFLAGS = -march=armv7-a
ASFLAGS = -mcpu=cortex-a7 -mfloat-abi=soft

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

build-directory = $(shell mkdir -p build build/device build/lib)

build/%.o: %.c
	$(call build-directory)
	$(call quiet-command,$(CC) $(CFLAGS) \
		-c -o $@ $<,"  CC       $(TARGET_DIR)$@")

AS_WITH = $(call quiet-command,$(CC) $(ASFLAGS) \
		$(1) -c -o $@ $<,"  AS       $(TARGET_DIR)$@")

build/%.o: %.S
	$(call build-directory)
	$(call AS_WITH, )
