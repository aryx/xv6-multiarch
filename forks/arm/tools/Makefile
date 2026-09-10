include ../makefile.inc

CFLAGS = -Werror -Wall -I../../../include
CFLAGS += -iquote ../kernel

all: mkfs

mkfs: mkfs.c
	$(HOSTCC) $(CFLAGS) -o $@ $^

clean:
	rm -f mkfs	
