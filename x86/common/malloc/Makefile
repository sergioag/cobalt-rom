# $Id: Makefile,v 1.1.1.1 2003/06/10 22:42:00 iceblink Exp $
#
# Copyright (c) 2000 Cobalt Networks, Inc.
#

include ../../../rom.mk

# you need a compiler that does ANSI prototypes
CFLAGS += -O -Wall -fno-pic -I../../include -I$(LINUXDIR)/include \
	-nostdinc -fno-builtin

LG_OBJS = calloc.o free.o malloc.o morecore.o realloc.o valloc.o
SA_OBJS = free.o malloc.o morecore.o realloc.o

OBJS = $(SA_OBJS)

all: alloc.o

# If you are mixing this malloc with another C library.
alloc.o: $(OBJS)
	$(LD) -r -o alloc.o $(OBJS)

# If you are building a C library.
libmalloc.a: $(OBJS)
	$(AR) r libmalloc.a $(OBJS)

clean:
	rm -f alloc.o libmalloc.a $(OBJS)

$(OBJS): malloc.h
