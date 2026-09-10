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
SOURCE = kernel/

# The name of the output file to generate.
TARGET = kernel.img

# The name of the assembler listing file to generate.
LIST = kernel.list

# The name of the map file to generate.
MAP = kernel.map

# The name of the linker script to use.
LINKER = kernel/kernel.ld

# The names of libraries to use.
LIBRARIES := csud

# claude: csud/ (repo root) is Alex Chadwick's CSUD ("Chadderz's Simple
# USB Driver", MIT, github.com/Chadderz121/csud @ e13b935), vendored
# 2026-09-09 to replace the libcsud.a BINARY BLOB this fork used to
# ship with no source. Confirmed to be that blob's own source first:
# built from this tree it exports the same 141 symbols (plus
# LogPrintF), same object names. Vendoring it makes the USB stack
# patchable - specifically so QEMU's dwc2 model can be accommodated at
# RUNTIME (see designware20.c's own HcdEmulating(), and
# notes_arch_arm_pi1.txt) rather than being stuck with whatever the
# prebuilt .a happened to do.
#
# Shared at the repo's top level (not vendored separately per fork)
# once forks/arm-pi1-bis turned out to need the exact same driver
# (same DEVSPACE/PHYSIO layout, byte-for-byte identical keyboard.s) -
# see notes_arch_arm_pi1_bis.txt. Only the SOURCE is shared; every
# fork still builds its own compiled objects/archive into its own
# $(BUILD) (below), so two forks building concurrently (build-all)
# never race on the same output file, and each fork's own CSUD_CFLAGS
# can diverge if a future port ever needs that.
#
# Built here rather than by csud/makefile so it gets the SAME real
# codegen flags as the rest of this kernel ("-mcpu=arm1176jzf-s -marm",
# see CFLAGS below). csud's own makefile only passes "-Wa,-march=armv6
# -Wa,-mcpu=arm1176jzf-s", which constrains the assembler but NOT gcc's
# code generation - the exact trap documented for this fork's own CFLAGS
# below, and a real (not theoretical) source of ARMv7/Thumb-interworking
# instructions inside a library linked into an ARMv6 kernel.
CSUDDIR := ../../csud/
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
# claude: output goes to $(BUILD) (this fork's OWN build dir), not
# $(CSUDDIR) (the shared source tree) - see the shared-source comment
# above for why.
$(BUILD)libcsud.a: $(CSUD_OBJS)
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
CFLAGS := -fno-pic -static -fcommon -Wno-packed-bitfield-compat -fno-builtin -fno-strict-aliasing -fshort-wchar -O2 -Wall -MD -MP -ggdb -Werror -fno-omit-frame-pointer -fno-stack-protector -mcpu=arm1176jzf-s -marm -I kernel

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
$(BUILD)output.elf : $(OBJECTS) $(C_OBJS) $(LINKER) $(BUILD)libcsud.a
	$(ARMGNU)-ld --no-undefined $(OBJECTS) $(C_OBJS) -L$(BUILD) $(patsubst %,-l %,$(LIBRARIES)) -Map $(MAP) -o $(BUILD)output.elf -T $(LINKER)

# Rule to make the object files.
# claude: $(BUILD) is an ORDER-ONLY prerequisite ("|"), not a normal
# one - compiling into build/ updates that directory's own mtime, so as
# a normal prerequisite it is newer than every .o the moment the first
# one is written, and EVERY later make rebuilds the whole kernel from
# scratch (visible as forks/arm-pi1 recompiling on a second, unchanged
# "make build-all"). Order-only means "create the directory first if
# missing" without its timestamp ever forcing a rebuild - which is what
# the sibling forks/arm-pi3/Makefile already does.
$(BUILD)%.o: $(SOURCE)%.s | $(BUILD)
	$(ARMGNU)-as -I $(SOURCE) $< -o $@

$(BUILD)%.o: $(SOURCE)%.c | $(BUILD)
	$(CC) -c $(CFLAGS) $<  -o $@

# claude: source/fs.img is GENERATED, not checked in - it is the output of
# uprogs/'s own build (mkfs plus every UPROGS binary). It used to be a
# committed 512KB binary blob, which is exactly how uprogs/ came to be
# unbuildable without anyone noticing: bringing it up for this change needed
# two real fixes (a C11 static_assert redefinition in mkfs.c, and CFLAGS that
# only constrained the assembler rather than gcc's codegen). A checked-in
# image hides that kind of bit-rot indefinitely, and can silently drift from
# the uprogs/*.c it is supposed to represent. Same treatment, and the same
# rule shape, as the sibling forks/arm-pi2/Makefile.
#
# The phony target is "uprogs-fs-img", NOT "source/fs.img": a phony
# prerequisite of a REAL file makes that file's recipe run unconditionally
# (GNU make treats it as always newer), which would re-assemble entry.s and
# re-link the kernel on every single build. This way the recursive make still
# runs every time and does its own up-to-date checking, while source/fs.img's
# timestamp only moves when the image content actually changed.
#
# uprogs/Makefile names its toolchain variable TOOLPREFIX and expects a
# trailing dash; this Makefile's own ARMGNU carries none (it is used as
# "$(ARMGNU)-gcc"), hence the "-" here.
.PHONY: uprogs-fs-img
uprogs-fs-img:
	$(MAKE) -C user TOOLPREFIX=$(ARMGNU)- fs.img

kernel/fs.img: uprogs-fs-img
	cmp -s user/fs.img $@ || cp user/fs.img $@

# entry.s is the only source file that embeds the image (its own
# .incbin "fs.img", resolved via the "-I $(SOURCE)" on the assembler line
# above), so only ITS object needs the dependency.
$(BUILD)entry.o: kernel/fs.img

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

build/output-qemu.elf: $(OBJECTS) $(C_OBJS) kernel/kernel-qemu.ld $(BUILD)libcsud.a
	$(ARMGNU)-ld --no-undefined $(OBJECTS) $(C_OBJS) -L$(BUILD) $(patsubst %,-l %,$(LIBRARIES)) -Map kernel-qemu.map -o build/output-qemu.elf -T kernel/kernel-qemu.ld

kernel-qemu.img: build/output-qemu.elf
	$(ARMGNU)-objcopy build/output-qemu.elf -O binary kernel-qemu.img

qemu: kernel-qemu.img
	@echo "Press Ctrl-A and then X to terminate QEMU session"
	$(QEMU) -M raspi1ap -nographic -kernel kernel-qemu.img

# claude: same kernel-qemu.img, but with a real GTK window instead of
# -nographic, so the emulated HDMI/composite framebuffer this fork's own
# gpuputc()/drawpixel() draws into (when fb_ready is true) is actually
# visible - see notes_arch_arm_pi1.txt's "Bug 7" section. Requires a
# working $DISPLAY. "-serial mon:stdio" (not plain "-monitor stdio")
# multiplexes BOTH the interactive serial console (the actual xv6 shell)
# AND the QEMU monitor onto this same terminal, Ctrl-A C to switch
# between them - same convention as -nographic's own Ctrl-A shortcuts,
# so a shell session and a "screendump /tmp/fb.ppm" capture both work
# from one terminal without needing a second window.
#
# claude: "-device usb-kbd" (added once source/timer.c's own USB
# keyboard wiring and the csud/ fixes landed - see notes_arch_arm_pi1.txt's
# "Bug 8" through "Bug 10") gives QEMU an actual USB keyboard attached
# to this board's dwc2 controller, so keys typed directly into the GTK
# window itself now reach the shell - not just the serial console.
# claude: QMP_SOCK (optional, unset by default - a plain interactive
# "make qemu-graphics" is unaffected) adds a QMP socket alongside the
# monitor, for scripted regression testing (screendump/send-key) - see
# scripts/qemu_graphics.py and scripts/test_qemu_graphics.py. "comma"
# is the standard Make idiom for getting a literal "," into a
# "$(if ...)" call, which itself can't contain an unescaped comma.
comma := ,
QMP_OPTS := $(if $(QMP_SOCK),-qmp unix:$(QMP_SOCK)$(comma)server$(comma)nowait)

qemu-graphics: kernel-qemu.img
	@echo "Serial console + QEMU monitor share this terminal - Ctrl-A C to switch to the monitor (try: screendump /tmp/fb.ppm), Ctrl-A C again to switch back"
	@echo "Ctrl-A X (from the monitor side) to terminate"
	@echo "Click the QEMU window and type there for a real USB-keyboard test - the terminal keeps working as the serial console too"
	$(QEMU) -M raspi1ap -kernel kernel-qemu.img -serial mon:stdio -device usb-kbd $(QMP_OPTS)


# claude: pull in the per-object header dependencies gcc already writes.
# CFLAGS above has carried "-MD" all along, so every compile emits a
# "$(BUILD)<name>.d" listing the headers that object really includes -
# but nothing ever read them, so editing a header rebuilt NOTHING and an
# incremental build was quietly wrong. That is what made forks/arm-pi1
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
-include $(BUILD)*.d $(BUILD)csud/*.d $(BUILD)csud/*/*.d $(BUILD)csud/*/*/*.d

$(BUILD):
	mkdir $@

# Rule to clean files.
clean :
	-rm -rf $(BUILD)
	-rm -f $(TARGET)
	-rm -f $(LIST)
	-rm -f $(MAP)
	-rm -f kernel-qemu.img kernel-qemu.map
