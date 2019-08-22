# $Id: Makefile,v 1.1.1.1 2003/06/10 22:42:00 iceblink Exp $
#
# Copyright (c) 2000 Cobalt Networks, Inc.
#

.PHONY: all

include ../../ROM.mk

OBJS=emup.o
LIBS=-lbe -ldevice

all: emup

emup: $(OBJS)
	$(CCC) -o emup $(OBJS) $(LIBS)
	
clean:
	rm -f $(OBJS) emup

.cc.o:
	$(CCC) -c $(CFLAGS) $*.cc -o $*.o

.c.o:
	$(CC) -c $(CFLAGS) $*.c -o $*.o
