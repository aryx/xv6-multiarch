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

# claude: csud/ is Alex Chadwick's CSUD ("Chadderz's Simple USB Driver",
# MIT, github.com/Chadderz121/csud @ e13b935), vendored 2026-09-09 to
# replace the libcsud.a BINARY BLOB this fork used to ship with no
# source. Confirmed to be that blob's own source first: built from this
# tree it exports the same 141 symbols (plus LogPrintF), same object
# names. Vendoring it makes the USB stack patchable - specifically so
# QEMU's dwc2 model can be accommodated at RUNTIME (see designware20.c's
# own HcdEmulating(), and notes_arch_arm_pi1.txt) rather than being
# stuck with whatever the prebuilt .a happened to do.
#
# Built here rather than by csud/makefile so it gets the SAME real
# codegen flags as the rest of this kernel ("-mcpu=arm1176jzf-s -marm",
# see CFLAGS below). csud's own makefile only passes "-Wa,-march=armv6
# -Wa,-mcpu=arm1176jzf-s", which constrains the assembler but NOT gcc's
# code generation - the exact trap documented for this fork's own CFLAGS
# below, and a real (not theoretical) source of ARMv7/Thumb-interworking
# instructions inside a library linked into an ARMv6 kernel.
CSUDDIR := csud/
CSUD_SRCS := $(wildcard $(CSUDDIR)source/*.c) $(wildcard $(CSUDDIR)source/*/*.c) \
             $(wildcard $(CSUDDIR)source/*/*/*.c)
CSUD_OBJS := $(patsubst $(CSUDDIR)source/%.c,$(BUILD)csud/%.o,$(CSUD_SRCS))
CSUD_CFLAGS := -std=c99 -fpack-struct -Wno-packed-bitfield-compat -fshort-wchar \
               -O2 -Wno-strict-aliasing -fno-pic -static -fno-stack-protector \
               -mcpu=arm1176jzf-s -marm \
               -DFINAL -DTYPE_LOWLEVEL -DTARGET_RPI \
               -DLIB_HUB -DLIB_HID -DLIB_KBD -DLIB_MOUSE

# claude: "rcs", not csud's own makefile's "rc" - the "s" writes the
# archive symbol index. Without it ld cannot tell which members define
# what, pulls in members nothing references, and then fails
# --no-undefined on CSUD's own unresolved LogPrint/LogPrintF (which the
# host application is expected to supply only if it actually uses the
# logging build). The shipped prebuilt libcsud.a carries an index too.
$(CSUDDIR)libcsud.a: $(CSUD_OBJS)
	-rm -f $@
	$(ARMGNU)-ar rcs $@ $(CSUD_OBJS)

$(BUILD)csud/%.o: $(CSUDDIR)source/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CSUD_CFLAGS) -I $(CSUDDIR)include -c $< -o $@

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
$(BUILD)output.elf : $(OBJECTS) $(C_OBJS) $(LINKER) $(CSUDDIR)libcsud.a
	$(ARMGNU)-ld --no-undefined $(OBJECTS) $(C_OBJS) -L$(CSUDDIR) $(patsubst %,-l %,$(LIBRARIES)) -Map $(MAP) -o $(BUILD)output.elf -T $(LINKER)

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

build/output-qemu.elf: $(OBJECTS) $(C_OBJS) kernel-qemu.ld $(CSUDDIR)libcsud.a
	$(ARMGNU)-ld --no-undefined $(OBJECTS) $(C_OBJS) -L$(CSUDDIR) $(patsubst %,-l %,$(LIBRARIES)) -Map kernel-qemu.map -o build/output-qemu.elf -T kernel-qemu.ld

kernel-qemu.img: build/output-qemu.elf
	$(ARMGNU)-objcopy build/output-qemu.elf -O binary kernel-qemu.img

qemu: kernel-qemu.img
	@echo "Press Ctrl-A and then X to terminate QEMU session"
	$(QEMU) -M raspi1ap -nographic -kernel kernel-qemu.img

# claude: same kernel-qemu.img, but with a real GTK window instead of
# -nographic, so the emulated HDMI/composite framebuffer this fork's own
# gpuputc()/drawpixel() draws into (when fb_ready is true) is actually
# visible - see notes_arch_arm_pi1.txt's "Open gap" section. Requires a
# working $DISPLAY. "-serial mon:stdio" (not plain "-monitor stdio")
# multiplexes BOTH the interactive serial console (the actual xv6 shell)
# AND the QEMU monitor onto this same terminal, Ctrl-A C to switch
# between them - same convention as -nographic's own Ctrl-A shortcuts,
# so a shell session and a "screendump /tmp/fb.ppm" capture both work
# from one terminal without needing a second window.
qemu-graphics: kernel-qemu.img
	@echo "Serial console + QEMU monitor share this terminal - Ctrl-A C to switch to the monitor (try: screendump /tmp/fb.ppm), Ctrl-A C again to switch back"
	@echo "Ctrl-A X (from the monitor side) to terminate"
	$(QEMU) -M raspi1ap -kernel kernel-qemu.img -serial mon:stdio

$(BUILD):
	mkdir $@

# Rule to clean files.
clean :
	-rm -rf $(BUILD)
	-rm -f $(TARGET)
	-rm -f $(LIST)
	-rm -f $(MAP)
	-rm -f kernel-qemu.img kernel-qemu.map
