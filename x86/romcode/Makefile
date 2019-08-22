# $Id: Makefile,v 1.2 2003/12/11 00:55:04 thockin Exp $
#
# Copyright (c) 1999-2000 Cobalt Networks, Inc.
#

.PHONY: romcode

include ../../rom.mk

RAM_SIZE := $(shell (test -e ../ramcode/ramcode && ls -l ../ramcode/ramcode | awk '{print $$5}'))

ASFLAGS = \
	-dRAM_SIZE=$(RAM_SIZE) \
        -dROMCODE_OFFSET=$(ROM_ROMCODE_OFFSET) \
        -dRAMCODE_OFFSET=$(ROM_RAMCODE_OFFSET) \
	-dRAMCODE_ADDR=$(RAM_RAMCODE_ADDR)

SRC3K = reset3k.S dramprobe3k.S
SRC5K = reset5k.S dramprobe5k.S delay5k.S i2c5k.S

all:
	@echo -ne "$(CURDIR)/: "
	$(MAKE) romcode
	@echo "done"

romcode: reset.S $(SRC3K) $(SRC5K) Makefile ../ramcode/ramcode
	$(ASM) $(ASFLAGS) -f bin -o romcode.tmp -l reset.lst reset.S
	split -b 4k romcode.tmp
	cat xa[n-z] > romcode

clean:
	rm -f romcode romcode.tmp reset.lst
	rm -f xa[a-z]
