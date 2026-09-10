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
# claude: was "-march=armv7-a", which drops the implied FPU extension back
# out while this toolchain's own -mfloat-abi default stays "hard" - gcc then
# refuses outright ("selected architecture lacks an FPU"). uprogs/ had never
# been built here, so it kept the broken form; the kernel side of this same
# fork already builds with -mcpu=cortex-a7 -mfloat-abi=hard, and the sibling
# forks/arm-pi2/uprogs/Makefile already carries the identical line. Same bug
# class as forks/arm/makefile.inc's own -mfloat-abi comment.
# -MP joins -MD so a vanished header is a rebuild, not a hard error;
# -Wno-infinite-recursion is the GCC 12+ false positive on sh.c's runcmd().
CFLAGS := -fno-pic -static -fno-builtin -fno-strict-aliasing -fshort-wchar -O2 -Wall -MD -MP -ggdb -Werror -Wno-infinite-recursion -fno-omit-frame-pointer -fno-stack-protector -marm -mcpu=cortex-a7 -mfloat-abi=hard -iquote . -I..   # claude: fork root, so shared utilities/ files can say "kernel/types.h"

all: mkfs initcode fs.img

initcode: initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -c initcode.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
	$(OBJCOPY) -S -O binary initcode.out initcode
	$(OBJDUMP) -S initcode.o > initcode.asm


# claude: ASFLAGS was never set here - it did not need to be while usys.S
# sat next to the traps.h it includes. make's built-in .S rule uses
# $(ASFLAGS)/$(CPPFLAGS), NOT $(CFLAGS) (gotcha 9).
ASFLAGS = -iquote .

# claude: the test programs and the user library live in their own
# directories since Phase 0, but their objects and linked binaries still
# land HERE - so UPROGS below, the ./mkfs invocation and ULIB are all
# unchanged, and mkfs still sees bare names with no '/' in them. "-iquote ."
# above is what makes it work: a quoted #include resolves from the
# INCLUDER's directory, which for a source compiled out of ../tests or
# ../ulib is not where this fork's userland headers live.
# claude: ../../../utilities and hold the portable programs
# shared by every fork. vpath works HERE (unlike the flat forks) because this
# Makefile's prerequisites are bare names - "cat.o: cat.c" - with no directory
# component for vpath to have to reproduce.
vpath %.c ../tests ../ulib ../../../utilities ../../../shells ../../../lib_core/libc/ulib
vpath %.S ../ulib

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
# "types.h"/"fs.h"/"stat.h"/"param.h" - the USERLAND header set, which it
# always compiled against, having lived here. -iquote, not -I: mkfs is a
# host program that also includes the system <fcntl.h> (gotcha 6).
mkfs: ../tools/mkfs.c fs.h
	gcc -iquote . -o mkfs ../tools/mkfs.c
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
	_timetest\
	_benchmark\

fs.img: mkfs README $(UPROGS)
	./mkfs fs.img README $(UPROGS)

clean:
	rm -f *.o *.d *.asm *.sym fs.img mkfs initcode initcode.out $(UPROGS)
