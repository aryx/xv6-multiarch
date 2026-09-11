include ../makefile.inc

# claude: -Wno-infinite-recursion - GCC false positive on sh.c's own
# runcmd(), which is genuinely mutually-recursive across several
# switch(cmd->type) cases (exec/redir/pipe/list/back), not actually
# infinite - the same false positive already documented/fixed in every
# other fork this project has brought up (see notes_debugging_techniques.txt
# item 1).
# claude: "-iquote ." is for THIS directory's own userland headers, which a
# source compiled out of ../tests or ../ulib can no longer reach by sitting
# next to them. Dropped the "-iquote ../kernel" this used to carry (on top
# of "-I.." below, which already resolves "kernel/types.h"-style includes
# fork-root-relative): it doesn't help anything "-I.." doesn't already
# cover, and it actively shadows the shared "user.h" from
# ../../../include/user with this fork's own stale, pre-2019-API
# forks/arm-pi1-bis/kernel/user.h - dead since the 2019-API migration gave
# this fork its own user/user.h, deleted in 819ff8c ("one shared
# user-level API") on the assumption nothing still shadowed it.
CFLAGS += -iquote . -Wno-infinite-recursion -I.. -I../../../include -I../../../include/user   # claude: fork root, so shared utilities/ files can say "kernel/types.h"
# claude: usys.S needs the same treatment - make's built-in .S rule uses
# $(ASFLAGS), never $(CFLAGS) (gotcha 9 in plan_factorization.md).
ASFLAGS += -iquote . -I ../kernel -I ../../../include/kernel
# claude: the test programs and the user library live in their own
# directories since Phase 0, but their objects and linked binaries still
# land HERE - so UPROGS below, the $(MKFS) invocation and ULIB are all
# unchanged, and mkfs still sees bare names with no '/' in them.
# claude: ../../../utilities and hold the portable programs
# shared by every fork. vpath works HERE (unlike the flat forks) because this
# Makefile's prerequisites are bare names - "cat.o: cat.c" - with no directory
# component for vpath to have to reproduce.
vpath %.c ../tests ../ulib ../../../utilities ../../../shells ../../../lib_core/libc/ulib ../../../kernel/init/user ../../../tests
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

# claude: explicit rule, not covered by vpath above - the shared file's
# name (tests/usertests-pi.c, shared with forks/arm-pi1, forks/arm-pi2 and
# forks/arm-pi3 - see its own header comment) doesn't match what vpath's
# "usertests.o: usertests.c" search would look for. Placed AFTER "all:"
# above deliberately - this fork's own top Makefile invokes "make -C user"
# with no explicit target, so whatever rule appears FIRST in this file
# becomes the default goal; putting a new rule before "all:" silently
# replaces it (the exact bug forks/arm-pi2's own notes_arch_arm_pi2.txt
# Bug 13 already documents, hit again here while wiring this file up).
usertests.o: ../../../tests/usertests-pi.c
	$(CC) $(CFLAGS) -c -o $@ $<

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

# claude: real bug, not hypothetical - see forks/arm-pi1/user/Makefile's
# own identical comment (this directory had the exact same gap: the
# base CFLAGS this Makefile inherits via "include ../makefile.inc" has
# carried "-MD -MP" all along, generating a per-object .d file here on
# every compile, but nothing ever -included them, so a shared header's
# content changing never invalidated an already-built .o in this
# directory - and this fork's own top-level "clean" target, unlike its
# three siblings', DOES already reach into user/ via "make -C user
# clean", but that alone was never enough without this).
-include $(wildcard *.d)
