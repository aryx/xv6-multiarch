# Cross-compiling (e.g., on Mac OS X)
#TOOLPREFIX = i386-jos-elf-
# Using native tools (e.g., on X86 Linux)
TOOLPREFIX =


# The intermediate directory for compiled object files.
BUILD = build/

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
# claude: real -mcpu/-marm instead of "-Wa," flags, and -MP alongside -MD.
# "-Wa,-march=armv6 -Wa,-mcpu=arm1176jzf-s" only constrains what the
# ASSEMBLER stage accepts - it does not set gcc's own codegen target, which
# then defaults to this toolchain's armv7-a+fp and can emit Thumb-interworking
# calls a real arm1176jzf-s cannot execute. This is the identical bug the
# kernel's own Makefile documents at its CFLAGS line and fixes the same way;
# uprogs/ was simply never built here, so it kept the broken form.
# claude: -Wno-infinite-recursion is a confirmed GCC 12+ false positive on
# sh.c's runcmd(), which recurses but always terminates via exec()/exit() in
# its base case - see notes_debugging_techniques.txt item 1, and the same
# flag in forks/arm-pi2/uprogs/Makefile.
CFLAGS := -fno-pic -static -fno-builtin -fno-strict-aliasing -fshort-wchar -O2 -Wall -MD -MP -ggdb -Werror -Wno-infinite-recursion -fno-omit-frame-pointer -fno-stack-protector -mcpu=arm1176jzf-s -marm -iquote . -I..   # claude: fork root, so shared utilities/ files can say "kernel/types.h"

initcode: initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -c initcode.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
	$(OBJCOPY) -S -O binary initcode.out initcode
	$(OBJDUMP) -S initcode.o > initcode.asm


# claude: ASFLAGS was never set here - it did not need to be, since usys.S
# sat in this directory next to the traps.h it includes. make's built-in
# .S rule uses $(ASFLAGS)/$(CPPFLAGS), NOT $(CFLAGS), so the -iquote added
# to CFLAGS above does not reach it, and usys.S alone fails with
# "traps.h: No such file or directory".
ASFLAGS = -iquote .

# claude: the test programs and the user library live in their own
# directories since Phase 0, but their objects and linked binaries still
# land HERE - so UPROGS below, the ./mkfs invocation and ULIB are all
# unchanged, and mkfs still sees bare names with no '/' in them. Only the
# source lookup moves, which is what vpath is for. The "-iquote ." above
# is what makes that work: a quoted #include resolves from the INCLUDER's
# own directory, which for a source compiled out of ../tests or ../ulib is
# not where this fork's userland headers live - they are here.
# claude: ../../../utilities and hold the portable programs
# shared by every fork. vpath works HERE (unlike the flat forks) because this
# Makefile's prerequisites are bare names - "cat.o: cat.c" - with no directory
# component for vpath to have to reproduce.
vpath %.c ../tests ../ulib ../../../utilities
vpath %.S ../ulib

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

# claude: mkfs.c moved to ../tools/ but still includes this directory's own
# "types.h"/"fs.h"/"stat.h"/"param.h" - the USERLAND header set, which is
# what it always compiled against, having lived here. -iquote, not -I:
# mkfs is a host program that also includes the system <fcntl.h>, and -I
# would let xv6's own fcntl.h shadow it and lose O_TRUNC (gotcha 6).
# -iquote is searched for "..." includes only, never for <...>. The binary
# keeps being built here, so ./mkfs below and .gitignore are unchanged.
mkfs: ../tools/mkfs.c fs.h
	gcc -Werror -Wall -iquote . -o mkfs ../tools/mkfs.c

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
	rm -f *.o *.d fs.img mkfs $(UPROGS)
