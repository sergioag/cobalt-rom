# $Id: Makefile,v 1.1.1.1 2003/06/10 22:42:28 iceblink Exp $
#
# Copyright (c) 1999-2000 Cobalt Networks, Inc.
#

include ../../../rom.mk

OBJS = main.o memtest.o

OUTFILE = memtest
RELOC = 0x1000

OBJDUMP = objdump --full-contents --disassemble-all
OBJCOPY = objcopy -O binary

CFLAGS += -I../../include -nostdinc -DMEMTEST -DINROM $(DEFS)

LDOPTS = -Map $(OUTFILE).map -Bstatic -N -T $(OUTFILE).lds \
             -Ttext $(RELOC)

all: $(OUTFILE)

$(OUTFILE): $(OBJS) $(OUTFILE).lds Makefile ../../common/libcommon.a
	$(LD) $(LDOPTS) $(OBJS) -o $(OUTFILE) ../../common/libcommon.a $(LIBGCC)
	cp $(OUTFILE) $(OUTFILE).elf
	strip $(OUTFILE)
	$(OBJDUMP) $(OUTFILE).elf > $(OUTFILE).lst

# build the ld script
.PHONY: $(OUTFILE).lds
$(OUTFILE).lds:
	sed 's/BASE_ADDR/$(RELOC)/g' memtest.cfg > $@

clean:
	-rm -f $(OUTFILE) $(OBJS)
	-rm -f $(OUTFILE).{map,lst,bin,elf,lds} *.o

