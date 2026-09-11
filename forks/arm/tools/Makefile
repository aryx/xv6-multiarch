include ../makefile.inc

CFLAGS = -Werror -Wall -I../../../include -I../../../include/arch/arm
CFLAGS += -iquote ../kernel -iquote ../../../include/kernel

all: mkfs

mkfs: ../../../tools/mkfs-fixedbudget.c ../../../include/kernel/fs.h ../kernel/conf.h
	$(HOSTCC) $(CFLAGS) -o $@ ../../../tools/mkfs-fixedbudget.c

clean:
	rm -f mkfs	
