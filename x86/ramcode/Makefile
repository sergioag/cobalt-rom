# $Id: Makefile,v 1.2 2003/12/11 00:55:04 thockin Exp $
#
# Copyright (c) 1999,2000 Cobalt Networks, Inc.
#

include ../../rom.mk

OBJS = head.o main.o

OUTFILE = ramcode

OBJDUMP = objdump --full-contents --disassemble-all
OBJCOPY = objcopy -O binary

DEFS = -DBUILD_INFO=$(BUILDINFO) $(VERSION_DEFS)

CFLAGS += -I../include -DRAMCODE -DINROM -nostdinc -I$(TOPDIR)/cobalt_romfs \
	$(DEFS)

LDOPTS = -Map $(OUTFILE).map -Bstatic -N -T $(OUTFILE).lds \
	-Ttext $(RAM_RAMCODE_ADDR)

all:
	@echo -ne "$(CURDIR)/: "
	$(MAKE) $(OUTFILE)
	@echo "done"

$(OUTFILE): $(OBJS) $(OUTFILE).lds Makefile ../common/libcommon.a
	$(LD) $(LDOPTS) $(OBJS) -o $(OUTFILE).bin ../common/libcommon.a \
		$(LIBGCC)
	cp $(OUTFILE).bin $(OUTFILE).elf
	strip $(OUTFILE).bin
	$(OBJDUMP) $(OUTFILE).elf > $(OUTFILE).lst
	$(OBJCOPY) $(OUTFILE).bin $(OUTFILE)

# build the ld script
.PHONY: $(OUTFILE).lds
$(OUTFILE).lds: 
	sed 's/BASE_ADDR/$(RAM_RAMCODE_ADDR)/g' $(OUTFILE).cfg > $@

clean:
	rm -f $(OUTFILE) $(OBJS)
	rm -f $(OUTFILE).{map,lst,bin,elf,lds}

