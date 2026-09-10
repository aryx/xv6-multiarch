# specify path to QEMU, installed with MacPorts 
QEMU = qemu-system-arm

include makefile.inc

# claude: csud/ (repo root) is Alex Chadwick's CSUD ("Chadderz's Simple
# USB Driver", MIT), vendored 2026-09-09 - the SAME already-fixed tree
# forks/arm-pi1 uses, shared at the repo's top level rather than
# vendored per-fork (this fork's own keyboard.s is byte-for-byte
# identical to arm-pi1's, confirmed via "diff -w", and both ports share
# the same DEVSPACE/PHYSIO/mailbox layout, so every fix that got CSUD
# working under QEMU for arm-pi1 - see notes_arch_arm_pi1.txt bug 8/9 -
# applies unchanged here). Replaces the "dropped libcsud.a, it was
# never vendored" state this Makefile used to be in: that was correct
# at the time (no source, no way to build it), but no longer applies
# now that csud/ has real, buildable source in this repo.
#
# Only the SOURCE is shared; this fork still builds its own compiled
# objects/archive into its own build/ (below), so building this fork
# and forks/arm-pi1 concurrently (build-all) never races on the same
# output file.
#
# Built here (not via csud/makefile) so it gets the SAME real codegen
# flags as the rest of this kernel (-mcpu=arm1176jzf-s -marm) - see
# forks/arm-pi1/Makefile's own identical comment on why csud's own
# makefile's "-Wa,-march=armv6" is not equivalent (assembler-only, not
# gcc codegen).
CSUDDIR := ../../csud/
CSUD_SRCS := $(wildcard $(CSUDDIR)source/*.c) $(wildcard $(CSUDDIR)source/*/*.c) \
             $(wildcard $(CSUDDIR)source/*/*/*.c)
CSUD_OBJS := $(patsubst $(CSUDDIR)source/%.c,build/csud/%.o,$(CSUD_SRCS))
# claude: "-mfloat-abi=soft" - unlike forks/arm-pi1 (whose own CFLAGS
# has no float-abi override at all, so gcc's hard-float VFPv2 default
# for -mcpu=arm1176jzf-s is fine), THIS fork's own makefile.inc
# explicitly forces -mfloat-abi=soft kernel-wide (needed for
# initcode.S to build at all - see makefile.inc's own comment). CSUD
# built without it here linked as hard-float, causing "uses VFP
# register arguments, kernel.elf does not" - a real ABI mismatch, not
# a false positive, confirmed by the exact linker error.
CSUD_CFLAGS := -std=c99 -fpack-struct -Wno-packed-bitfield-compat -fshort-wchar \
               -O2 -Wno-strict-aliasing -fno-pic -static -fno-stack-protector \
               -mcpu=arm1176jzf-s -marm -mfloat-abi=soft \
               -DFINAL -DTYPE_LOWLEVEL -DTARGET_RPI \
               -DLIB_HUB -DLIB_HID -DLIB_KBD -DLIB_MOUSE

# claude: "rcs", not csud's own makefile's "rc" - the "s" writes the
# archive symbol index, needed for ld's own --no-undefined-equivalent
# resolution to pull in every object that's actually referenced -
# without it, CSUD's own unresolved LogPrint/LogPrintF (only ever
# supplied by csud_glue.c if the referencing objects are even linked
# in) breaks the build. See forks/arm-pi1/Makefile's own identical fix.
# claude: output goes to build/ (this fork's own build dir), not
# $(CSUDDIR) (the shared source tree) - see the shared-source comment
# above for why.
build/libcsud.a: $(CSUD_OBJS)
	-rm -f $@
	$(CROSSCOMPILE)ar rcs $@ $(CSUD_OBJS)

build/csud/%.o: $(CSUDDIR)source/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CSUD_CFLAGS) -I $(CSUDDIR)include -c $< -o $@

# link the libgcc.a for __aeabi_idiv. ARM has no native support for div
LIBS = -Lbuild -lcsud $(LIBGCC)

OBJS = \
	kernel/bio.o\
	kernel/console.o\
	kernel/csud_glue.o\
	kernel/exception.o\
	kernel/exec.o\
	kernel/file.o\
	kernel/fs.o\
        kernel/kalloc.o\
	kernel/keyboard.o\
	kernel/log.o\
	kernel/mailbox.o\
	kernel/main.o\
	kernel/memide.o\
	kernel/mmu.o\
	kernel/pipe.o\
	kernel/proc.o\
	kernel/spinlock.o\
	kernel/string.o\
	kernel/syscall.o\
	kernel/sysfile.o\
	kernel/sysproc.o\
	kernel/timer.o\
	kernel/trap.o\
	kernel/uart.o\
	kernel/wrapper.o\
	kernel/vm.o \

#device/picirq.o \

KERN_OBJS = $(OBJS) kernel/entry.o

kernel.elf: $(addprefix build/,$(KERN_OBJS)) kernel/kernel.ld build/initcode build/fs.img build/libcsud.a
	cp -f build/initcode initcode
	cp -f build/fs.img fs.img
	$(call LINK_BIN, kernel/kernel.ld, kernel.elf, \
		$(addprefix build/,$(KERN_OBJS)), \
		initcode fs.img)
	$(OBJCOPY) kernel.elf -O binary kernel.img
	@echo kernel image has been built.
	@$(OBJDUMP) -d kernel.elf > kernel.dis
	$(OBJDUMP) -S kernel.elf > kernel.asm
	$(OBJDUMP) -t kernel.elf | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym
	rm -f initcode fs.img

# claude: was "-M versatilepb -cpu arm1176 -kernel kernel.elf" - versatilepb
# is a completely unrelated ARM926EJ-S reference board (a QEMU-only
# workaround from before real Raspberry Pi board emulation existed) whose
# peripheral memory map shares NOTHING with the real BCM2835 GPIO/mailbox/
# interrupt-controller addresses this kernel hardcodes (confirmed via
# "-d int,guest_errors": every GPIO/mailbox access was a rejected invalid
# MMIO write). "-M raspi1ap" is QEMU's actual Raspberry Pi 1 A+ (BCM2835)
# machine, whose addresses genuinely match - and "-kernel" needs the raw
# "kernel.img" (real Pi/raspi1ap firmware boot convention), not
# "kernel.elf" (this port's own kernel.ld links at 0x80010000/AT(0x10000),
# matching raspi1ap's own "-kernel" load address - see
# notes_arch_arm_pi1_bis.txt for the fuller investigation - by the end
# of a later session this port reaches a full interactive shell, a
# working framebuffer console, and a working USB keyboard, the same as
# forks/arm-pi1).
# claude: "|| true" on "clear" - a real bug caught by this repo's own
# "verify Docker locally before touching CI" convention: "clear" exits
# nonzero ("TERM environment variable not set") in a headless container
# with no TTY, which made "make qemu" (and so test-xv6.py, which drives
# it) fail outright with no QEMU ever launched at all - not a QEMU or
# xv6 bug, this Makefile target itself was never runnable outside an
# interactive terminal until now.
qemu: kernel.elf
	@clear || true
	@echo "Press Ctrl-A and then X to terminate QEMU session\n"
	$(QEMU) -M raspi1ap -nographic -kernel kernel.img

# claude: same kernel.img, but with a real GTK window and a USB
# keyboard attached instead of -nographic - see forks/arm-pi1/Makefile's
# own "qemu-graphics" target for the identical pattern/reasoning
# (requires a working $DISPLAY; "-serial mon:stdio" multiplexes the
# interactive shell AND the QEMU monitor onto this terminal, Ctrl-A C
# to switch between them).
# claude: QMP_SOCK (optional, unset by default - a plain interactive
# "make qemu-graphics" is unaffected) adds a QMP socket alongside the
# monitor, for scripted regression testing (screendump/send-key) - see
# scripts/qemu_graphics.py and scripts/test_qemu_graphics.py. Same
# "comma" idiom as forks/arm-pi1/Makefile's own identical addition.
comma := ,
QMP_OPTS := $(if $(QMP_SOCK),-qmp unix:$(QMP_SOCK)$(comma)server$(comma)nowait)

qemu-graphics: kernel.elf
	@echo "Serial console + QEMU monitor share this terminal - Ctrl-A C to switch to the monitor (try: screendump /tmp/fb.ppm), Ctrl-A C again to switch back"
	@echo "Ctrl-A X (from the monitor side) to terminate"
	@echo "Click the QEMU window and type there for a real USB-keyboard test - the terminal keeps working as the serial console too"
	$(QEMU) -M raspi1ap -kernel kernel.img -serial mon:stdio -device usb-kbd $(QMP_OPTS)

INITCODE_OBJ = initcode.o
$(addprefix build/,$(INITCODE_OBJ)): kernel/initcode.S
	$(call build-directory)
	$(call AS_WITH, -nostdinc -Ikernel)

#initcode is linked into the kernel, it will be used to craft the first process
build/initcode: $(addprefix build/,$(INITCODE_OBJ))
	$(call LINK_INIT, -N -e start -Ttext 0)
	$(call OBJCOPY_INIT)
	$(OBJDUMP) -S $< > initcode.asm

build/fs.img:
	make -C tools
	make -C user


# claude: read back the per-object header dependencies gcc writes next to
# each object ("-MD" in makefile.inc's CFLAGS/ASFLAGS). Without this the
# .d files were either never generated (forks/arm) or generated and
# thrown away (every other Pi port), so editing a header rebuilt NOTHING
# - which is exactly why this port's test-xv6.py used to run "make clean"
# before every test run, and why "make test-all" recompiled from scratch
# even straight after "make build-all". The leading "-" ignores the files
# on a first build, when they do not exist yet. See
# docs/claude_notes/notes_build_system.txt for why this replaces an
# explicit "make depend" pass rather than adding one.
-include build/*.d build/*/*.d

clean: 
	rm -rf build
	rm -f *.o *.d *.asm *.sym vectors.S bootblock \
	initcode initcode.out fs.img kernel.elf kernel.dis kernel.img
	make -C tools clean
	make -C user clean
