# $Id: Makefile,v 1.2 2003/12/11 00:55:04 thockin Exp $
#
# Copyright (c) 2000 Cobalt Networks, Inc.
#

include ../../rom.mk

OBJS  = pci.o pci_io.o pci_fixup.o pci_resource.o region.o \
	mp.o mptable.o trampoline.o apic.o ioapic.o pirq.o \
	memfuncs.o printf.o crfs_func.o eeprom.o spin.o \
	delay.o serial.o ctype.o i2c.o lcd.o fs.o parallel.o \
	eth.o ide.o rtc.o exceptions.o md5.o misczip.o unzip.o \
	regs.o inflate.o menu.o cache.o boot_main.o x86-boot.o elf.o \
	systype.o msr.o eepro100_eeprom.o macphyter_eeprom.o dallas.o \
	microcode.o acpi.o shadow.o sbrk.o systype_cpu.o cmos_io.o \
	cmos.o strbuf.o readline.o errno.o string.o unbzip.o \
	delayandclear.o cache_l1.o id_eeprom.o eeprom_ops.o bootdevs.o


LIBOBJS = $(OBJS) malloc/alloc.o

DEFS = $(VERSION_DEFS) -DBUILD_INFO=$(BUILDINFO)

# hack for old assemblers (syntax was 'fixed' post 2.9.1)
ASVER := $(shell \
	if [ "`as --version | awk '{print $$3; exit}'`" = "2.9.1" ]; then \
		echo 1; \
	else \
		echo 0; \
	fi)
ASDEFS = -DOLD_AS="$(ASVER)"  # used by the .S.o rule

CFLAGS += -I../include -nostdinc -DINROM -DCOMMON -I../../cobalt_romfs \
	$(DEFS)

### for remote debugging
#
# OBJS += gdb.o gdb-x86.o
# CFLAGS += -g -DREMOTE_GDB
#

all:
	@echo -ne "$(CURDIR)/: "
	$(MAKE) builddate libcommon.a
	@echo "done"

# touch menu.c - it uses $(BUILDDATE), which must be current
.PHONY: builddate
builddate:
	$(RM) menu.o cmos.o

malloc/alloc.o:
	$(MAKE) -C malloc

libcommon.a: $(LIBOBJS)
	$(AR) r libcommon.a $(LIBOBJS)

crfs_func.o: ../../cobalt_romfs/crfs_func.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	$(MAKE) -C malloc clean
	$(RM) $(OBJS) libcommon.a

