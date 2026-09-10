# Cross-compiling (e.g., on Mac OS X)
#TOOLPREFIX = i386-jos-elf-
# Using native tools (e.g., on X86 Linux)
#TOOLPREFIX = ~/gcc-arm-none-eabi-5_4-2016q3/bin/arm-none-eabi-


# The intermediate directory for compiled object files.
BUILD = build/

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
#CFLAGS := -fno-pic -static -fno-builtin -fno-strict-aliasing -fshort-wchar -O2 -Wall -MD -ggdb -Werror -fno-omit-frame-pointer -fno-stack-protector -Wa,-march=armv6 -Wa,-mcpu=arm1176jzf-s
# claude: -marm added - this toolchain defaults to Thumb-2 (confirmed
# via "readelf -s": usertests.c's own mem() function landed at an ODD
# address, the ELF convention for a Thumb interworking target), but
# xv6's own exec()/trapret path just sets the trapframe's PC to the
# ELF entry point with no CPSR.T/Thumb-state handling at all - so every
# exec'd program ran in ARM state decoding Thumb-2 instruction bytes as
# 32-bit ARM ones, the exact same bug class already found and fixed in
# the kernel proper (see notes_arch_arm_pi2.txt's own "Bug 3"). This
# went unnoticed for a surprisingly long stretch (echo/ls/sh and most
# of usertests' own suite ran fine) until usertests' own mem() test - a
# tight malloc()/free() pointer-chasing loop - hung indefinitely rather
# than crashing outright, exactly like the kernel-side bug's own
# behavior before it was diagnosed. "-march=armv7-a" alone also lacks
# an FPU (broke with "-mfloat-abi=hard": selected architecture lacks an
# FPU" once this Makefile was actually exercised) - replaced with
# "-mcpu=cortex-a7", matching the kernel's own Makefile.
# claude: -Wno-infinite-recursion added - sh.c's own genuinely-
# terminating mutually-recursive runcmd() (parses a real command tree
# and recurses over BOTH sides of pipes/lists/backgrounds, not itself
# unconditionally) trips a GCC false positive, same one already seen
# elsewhere in this repo's own sh.c ports.
CFLAGS := -fno-pic -static -fno-builtin -fno-strict-aliasing -fshort-wchar -O2 -Wall -MD -ggdb -Werror -Wno-infinite-recursion -fno-omit-frame-pointer -fno-stack-protector -marm -mcpu=cortex-a7 -mfloat-abi=hard

all: mkfs initcode fs.img

initcode: initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -c initcode.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
	$(OBJCOPY) -S -O binary initcode.out initcode
	$(OBJDUMP) -S initcode.o > initcode.asm


ULIB = ulib.o usys.o printf.o umalloc.o
	
_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

_forktest: forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o _forktest forktest.o ulib.o usys.o
	$(OBJDUMP) -S _forktest > forktest.asm

mkfs: mkfs.c ../include/fs.h
	gcc -o mkfs mkfs.c
	#gcc -Werror -Wall -o mkfs mkfs.c

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

UPROGS=\
        _cat\
        _echo\
        _forktest\
        _grep\
        _init\
        _kill\
        _ln\
        _ls\
        _mkdir\
        _rm\
        _sh\
        _stressfs\
        _usertests\
        _wc\
        _zombie\

fs.img: mkfs README $(UPROGS)
	./mkfs fs.img README $(UPROGS)

clean:
	rm -f *.o *.d *.asm *.sym fs.img mkfs initcode initcode.out $(UPROGS)
