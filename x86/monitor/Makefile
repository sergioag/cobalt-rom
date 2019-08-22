# $Id: Makefile,v 1.2 2003/12/11 00:55:04 thockin Exp $
#
# Copyright (c) 1999-2000 Cobalt Networks, Inc.
#

include ../../rom.mk

OBJS = main.o boot.o cache.o cmos.o eeprom.o fs.o i2c.o ide.o lcd.o \
     memory.o pci.o rtc.o eth.o menu.o microcode.o smp.o
#OBJS += gdb.o   # for serial debugging

OUTFILE = monitor
BZOUTFILE = $(OUTFILE).bz2

OBJDUMP = objdump --full-contents --disassemble-all
OBJCOPY = objcopy -O binary

DEFS = -DBUILD_INFO=$(BUILDINFO) $(VERSION_DEFS) -DLOGO_OFF=$(ROM_LOGO_OFFSET)
#DEFS += -DMD5                        # for md5 checksums
#DEFS += -DREMOTE_GDB -g              # for serial debugging

CFLAGS += -I../include -nostdinc -DMONITOR -DINROM -I$(TOPDIR)/cobalt_romfs \
	$(DEFS)

LDOPTS = -Map $(OUTFILE).map -Bstatic -N -T $(OUTFILE).lds \
             -Ttext $(RAM_MONITOR_ADDR)

all:
	@echo -ne "$(CURDIR)/: "
	$(MAKE) builddate $(BZOUTFILE)
	@echo "done"

${BZOUTFILE}: ${OUTFILE}
	bzip2 -f ${OUTFILE}

$(OUTFILE): $(OBJS) $(OUTFILE).lds Makefile ../common/libcommon.a
	$(LD) $(LDOPTS) $(OBJS) -o $(OUTFILE) ../common/libcommon.a $(LIBGCC)
	cp $(OUTFILE) $(OUTFILE).elf
	strip $(OUTFILE)
	$(OBJDUMP) $(OUTFILE).elf > $(OUTFILE).lst

# build the ld script
.PHONY: $(OUTFILE).lds
$(OUTFILE).lds:
	sed 's/BASE_ADDR/$(RAM_MONITOR_ADDR)/g' monitor.cfg > $@

# touch main.c - it uses $(BUILDDATE), which must be current
.PHONY: builddate
builddate:
	$(RM) main.o

clean:
	$(RM) $(OUTFILE) $(OBJS)
	$(RM) $(OUTFILE).{map,lst,bin,elf,lds} *.o
	$(RM) $(BZOUTFILE)

