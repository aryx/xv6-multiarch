include ../makefile.inc

CFLAGS = -Werror -Wall -I../../../include
CFLAGS += -iquote ../kernel -iquote ../../../include/kernel

all: mkfs

mkfs: ../../../tools/mkfs-fixedbudget.c
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f mkfs	
