# $Id: Makefile,v 1.2 2003/12/11 00:55:04 thockin Exp $
#
# Copyright (c) 1997-2000 Cobalt Networks, Inc.
#

include ../rom.mk

BINS = buildbin raw2hex romcsum

all: $(BINS)

$(BINS): %: %.c Makefile
	@echo -ne "$(CURDIR)/$@: "
	$(CC) -ggdb -O2 -o $@ $<
	@echo "done"

clean:
	rm -f $(BINS)
