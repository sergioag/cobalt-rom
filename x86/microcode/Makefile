# $Id: Makefile,v 1.2 2003/12/11 00:55:04 thockin Exp $
# Copyright 2001 Sun Microsystems, Inc.
# All Rights Reserved

include ../../rom.mk

INFILE  = intel-ia32microcode.txt
OUTFILE = intel-ia32microcode.bin

all:
	@echo -ne "$(CURDIR)/: "
	$(MAKE) $(OUTFILE)
	@echo "done"

$(OUTFILE): $(INFILE)
	./intel-microcode2bin.pl < $(INFILE) > $(OUTFILE)

clean:
	rm -f $(OUTFILE)

