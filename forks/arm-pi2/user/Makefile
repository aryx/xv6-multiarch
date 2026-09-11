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
CFLAGS := -fno-pic -static -fno-builtin -fno-strict-aliasing -fshort-wchar -O2 -Wall -MD -ggdb -Werror -Wno-infinite-recursion -fno-omit-frame-pointer -fno-stack-protector -marm -mcpu=cortex-a7 -mfloat-abi=hard -iquote . -I.. -I../../../include -I../../../include/user   # claude: fork root, so shared utilities/ files can say "kernel/types.h"

all: mkfs initcode fs.img

initcode: initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -c initcode.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
	$(OBJCOPY) -S -O binary initcode.out initcode
	$(OBJDUMP) -S initcode.o > initcode.asm


# claude: ASFLAGS was never set here - it did not need to be while usys.S
# sat next to the traps.h it includes. make's built-in .S rule uses
# $(ASFLAGS)/$(CPPFLAGS), NOT $(CFLAGS), so the -iquote added above does
# not reach it (gotcha 9 in plan_factorization.md).
ASFLAGS = -iquote . -I ../../../include/kernel

# claude: the test programs and the user library live in their own
# directories since Phase 0, but their objects and linked binaries still
# land HERE - so UPROGS below, the ./mkfs invocation and ULIB are all
# unchanged, and mkfs still sees bare names with no '/' in them. Only the
# source lookup moves, which is what vpath is for. "-iquote ." is what
# makes it work: a quoted #include resolves from the INCLUDER's directory,
# which for a source compiled out of ../tests or ../ulib is not where this
# fork's userland headers live - they are here.
# claude: ../../../utilities and hold the portable programs
# shared by every fork. vpath works HERE (unlike the flat forks) because this
# Makefile's prerequisites are bare names - "cat.o: cat.c" - with no directory
# component for vpath to have to reproduce.
vpath %.c ../tests ../ulib ../../../utilities ../../../shells ../../../lib_core/libc/ulib ../../../kernel/init/user ../../../tests
vpath %.S ../ulib

# claude: explicit rule, not covered by vpath above - the shared file's
# name (tests/usertests-pi.c, shared with forks/arm-pi1, forks/arm-pi1-bis
# and forks/arm-pi3 - see its own header comment) doesn't match what
# vpath's "usertests.o: usertests.c" search would look for.
usertests.o: ../../../tests/usertests-pi.c
	$(CC) $(CFLAGS) -c -o $@ $<

ULIB = arith.o ulib.o usys.o printf.o umalloc.o
	
_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

_forktest: forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o _forktest forktest.o ulib.o usys.o
	$(OBJDUMP) -S _forktest > forktest.asm

# claude: mkfs.c moved to ../tools/ but still includes this directory's own
# "types.h"/"fs.h"/"stat.h"/"param.h" - the USERLAND header set, which is
# what it always compiled against, having lived here. -iquote, not -I: mkfs
# is a host program that also includes the system <fcntl.h>, and -I would
# let xv6's own fcntl.h shadow it (gotcha 6). The binary is still built
# here, so ./mkfs below and .gitignore are unchanged.
mkfs: ../../../tools/mkfs-margincheck.c fs.h
	gcc -iquote . -o mkfs ../../../tools/mkfs-margincheck.c -I../../../include -I../../../include/user
	#gcc -Werror -Wall -o mkfs mkfs.c -I../../../include -I../../../include/user

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
