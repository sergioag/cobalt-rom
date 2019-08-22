# $Id: Makefile,v 1.2 2003/12/11 00:55:04 thockin Exp $
#
# Copyright (c) 2000 Cobalt Networks, Inc.
#

include ../rom.mk

SUBDIRS = common monitor ramcode romcode microcode

all: signature
	@for a in $(SUBDIRS); do \
		$(MAKE) -C $$a || exit 1; \
	done
	@echo "$(CURDIR)/cobalt-x86.rom"
	$(BUILDBIN) -o cobalt-x86.rom -l $(ROM_PAGE_SIZE) \
		-s $(ROM_SIG_OFFSET) romsig.txt \
		-s $(ROM_RAMCODE_OFFSET) ramcode/ramcode \
		-s $(ROM_ROMCODE_OFFSET) romcode/romcode
	$(SETLOGO) cobalt-x86.rom ../cobalt.logo $(ROM_LOGO_OFFSET)

signature:
	@echo -ne "$(CURDIR)/romsig.txt: "
	@( \
	echo "Cobalt Networks Inc."; \
	echo "Firmware version $(VERSION)"; \
	echo "Built $(BUILDDATE)"; \
	echo -ne "\000"; \
	) > romsig.txt
	@echo "done"

clean:
	@for a in $(SUBDIRS); do \
		$(MAKE) -C $$a clean; \
	done
	rm -f include/common/romconf.h
	rm -f cobalt-x86.rom
	rm -f romsig.txt

dump-x86: x86
	x86/emulator_upload/emup /dev/ports/serial1 cobalt-x86.rom	

