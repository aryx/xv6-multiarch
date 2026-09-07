###############################################################################
#	makefile
#	 by Alex Chadwick
#
#	A makefile script for generation of raspberry pi kernel images.
###############################################################################

# The toolchain to use. arm-none-eabi works, but there does exist 
# arm-bcm2708-linux-gnueabi.
#ARMGNU ?= arm-none-eabi

# The intermediate directory for compiled object files.
BUILD = build/

# The directory in which source files are stored.
SOURCE = source/

# The name of the output file to generate.
TARGET = kernel.img

# The name of the assembler listing file to generate.
LIST = kernel.list

# The name of the map file to generate.
MAP = kernel.map

# The name of the linker script to use.
LINKER = kernel.ld

# The names of libraries to use.
LIBRARIES := csud

#CFLAGS := -fno-pic -static -fno-builtin -fno-strict-aliasing -Wall -MD -ggdb -Werror -fno-omit-frame-pointer -nostdinc -nostdlib -fno-stack-protector
# claude: two build-correctness fixes, safe for real hardware AND QEMU
# alike (neither changes any runtime behavior, just what gcc is actually
# told to target) - see notes_arch_rpi1.txt for the fuller writeup:
#   - "-fcommon": include/proc.h's own "struct cpu cpus[NCPU];" (no
#     extern) needs this to avoid "multiple definition of `cpus`" under
#     GCC 10+'s -fno-common default.
#   - real "-mcpu=arm1176jzf-s -marm" instead of "-Wa,-march=armv6
#     -Wa,-mcpu=arm1176jzf-s" - "-Wa," flags only restrict what the
#     ASSEMBLER stage accepts, not gcc's own code generation target,
#     which without an explicit override defaults to this toolchain's
#     real default (armv7-a+fp) - meaning cross-object calls could
#     silently get linked as Thumb-switching BLX instructions that a
#     genuine ARMv6/arm1176jzf-s core (real hardware OR QEMU's own
#     "-cpu arm1176") cannot execute at all. Confirmed as a REAL,
#     reproduced bug (not just a latent risk) in the sibling armv6-rpi
#     port under QEMU (notes_arch_armv6_rpi.txt bug 4) - fixed here
#     proactively before it was hit, since the underlying cause is
#     identical and this is a genuine correctness fix for the real
#     target CPU, not a QEMU-only workaround. No "-mfloat-abi=soft"
#     needed here (unlike armv6-rpi) - "-mcpu=arm1176jzf-s" alone
#     already implies its real VFPv2 unit (confirmed via "readelf -A":
#     Tag_FP_arch VFPv2, Tag_ABI_VFP_args "VFP registers"), matching
#     the vendored libcsud.a's own prebuilt hard-float ABI - forcing
#     soft-float here would conflict with it ("uses VFP register
#     arguments, build/output.elf does not").
CFLAGS := -fno-pic -static -fcommon -Wno-packed-bitfield-compat -fno-builtin -fno-strict-aliasing -fshort-wchar -O2 -Wall -MD -ggdb -Werror -fno-omit-frame-pointer -fno-stack-protector -mcpu=arm1176jzf-s -marm -I include

# claude: was "CC := gcc" - ignored ARMGNU entirely, always invoking the
# HOST compiler regardless of cross-toolchain prefix (every .s file
# already correctly used "$(ARMGNU)-as"; only C compilation had this
# gap). A genuine bug, not a QEMU-vs-real-hardware distinction - native
# host gcc can never produce valid ARM code either way.
CC := $(ARMGNU)-gcc

# The names of all object files that must be generated. Deduced from the 
# assembly code files in source.
OBJECTS := $(patsubst $(SOURCE)%.s,$(BUILD)%.o,$(wildcard $(SOURCE)*.s))

C_OBJS := $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))

# Rule to make everything.
all: $(TARGET) $(LIST)

# Rule to remake everything. Does not include clean.
#rebuild: all

# Rule to make the listing file.
$(LIST) : $(BUILD)output.elf
	$(ARMGNU)-objdump -d $(BUILD)output.elf > $(LIST)

# Rule to make the image file.
$(TARGET) : $(BUILD)output.elf
	$(ARMGNU)-objcopy $(BUILD)output.elf -O binary $(TARGET) 

# Rule to make the elf file.
$(BUILD)output.elf : $(OBJECTS) $(C_OBJS) $(LINKER)
	$(ARMGNU)-ld --no-undefined $(OBJECTS) $(C_OBJS) -L. $(patsubst %,-l %,$(LIBRARIES)) -Map $(MAP) -o $(BUILD)output.elf -T $(LINKER)

# Rule to make the object files.
$(BUILD)%.o: $(SOURCE)%.s $(BUILD)
	$(ARMGNU)-as -I $(SOURCE) $< -o $@

$(BUILD)%.o: $(SOURCE)%.c $(BUILD)
	$(CC) -c $(CFLAGS) $<  -o $@

# claude: QEMU-only targets, added alongside the existing real-hardware
# ones above rather than replacing anything - "all"/"$(TARGET)" (which
# real Raspberry Pi 1 firmware boots from an SD card) are completely
# unchanged. Reuses the exact same $(OBJECTS)/$(C_OBJS) (same source,
# same CFLAGS/ASFLAGS - identical code either way) linked a second time
# against kernel-qemu.ld instead of kernel.ld, since QEMU's "-M
# raspi1ap" needs the image placed at a different physical address than
# real hardware's firmware uses (see kernel-qemu.ld's own header comment
# and notes_arch_rpi1.txt).
QEMU ?= qemu-system-arm

build/output-qemu.elf: $(OBJECTS) $(C_OBJS) kernel-qemu.ld
	$(ARMGNU)-ld --no-undefined $(OBJECTS) $(C_OBJS) -L. $(patsubst %,-l %,$(LIBRARIES)) -Map kernel-qemu.map -o build/output-qemu.elf -T kernel-qemu.ld

kernel-qemu.img: build/output-qemu.elf
	$(ARMGNU)-objcopy build/output-qemu.elf -O binary kernel-qemu.img

qemu: kernel-qemu.img
	@echo "Press Ctrl-A and then X to terminate QEMU session"
	$(QEMU) -M raspi1ap -nographic -kernel kernel-qemu.img

$(BUILD):
	mkdir $@

# Rule to clean files.
clean :
	-rm -rf $(BUILD)
	-rm -f $(TARGET)
	-rm -f $(LIST)
	-rm -f $(MAP)
	-rm -f kernel-qemu.img kernel-qemu.map
