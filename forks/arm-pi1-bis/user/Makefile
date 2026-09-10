include ../makefile.inc

# claude: -Wno-infinite-recursion - GCC false positive on sh.c's own
# runcmd(), which is genuinely mutually-recursive across several
# switch(cmd->type) cases (exec/redir/pipe/list/back), not actually
# infinite - the same false positive already documented/fixed in every
# other fork this project has brought up (see notes_debugging_techniques.txt
# item 1).
# claude: "-iquote ." is for THIS directory's own userland headers, which a
# source compiled out of ../tests or ../ulib can no longer reach by sitting
# next to them; "-iquote ../kernel" replaces the old "-iquote ../", the
# fork root having stopped being where the kernel headers live.
CFLAGS += -iquote . -iquote ../kernel -Wno-infinite-recursion -I.. -I../../../include -I../../../include/user   # claude: fork root, so shared utilities/ files can say "kernel/types.h"
# claude: usys.S needs the same treatment - make's built-in .S rule uses
# $(ASFLAGS), never $(CFLAGS) (gotcha 9 in plan_factorization.md).
ASFLAGS += -iquote . -I ../kernel
# claude: the test programs and the user library live in their own
# directories since Phase 0, but their objects and linked binaries still
# land HERE - so UPROGS below, the $(MKFS) invocation and ULIB are all
# unchanged, and mkfs still sees bare names with no '/' in them.
# claude: ../../../utilities and hold the portable programs
# shared by every fork. vpath works HERE (unlike the flat forks) because this
# Makefile's prerequisites are bare names - "cat.o: cat.c" - with no directory
# component for vpath to have to reproduce.
vpath %.c ../tests ../ulib ../../../utilities ../../../shells ../../../lib_core/libc/ulib
vpath %.S ../ulib

ULIB = arith.o ulib.o usys.o printf.o umalloc.o

MKFS = ../tools/mkfs
FS_IMAGE = ../build/fs.img

UPROGS=\
	_cat\
	_echo\
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


all: $(FS_IMAGE)

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^  -L ../ $(LIBGCC)
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

_forktest: forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o _forktest forktest.o ulib.o usys.o
	$(OBJDUMP) -S _forktest > forktest.asm

$(FS_IMAGE): $(MKFS)  $(UPROGS)
	$(MKFS) $@  $(UPROGS) README
	$(OBJDUMP) -S usys.o > usys.asm

clean: 
	rm -f *.o *.d *.asm *.sym $(FS_IMAGE) \
	.gdbinit \
	$(UPROGS)
