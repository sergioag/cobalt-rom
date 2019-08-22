# $Id: Makefile,v 1.1.1.1 2003/06/10 22:42:27 iceblink Exp $
#
# Copyright (c) 2000 Cobalt Networks, Inc.
#

include ../../rom.mk

CPPFLAGS += -nostdinc -I../include

all: ../include/common/strings.h

# english is assumed to be the base language, always most current
../include/common/strings.h: raw-strings/strings.en.raw
	./genstrings.pl $< > $@
	make strings

strings: ../include/common/strings.h
	for a in raw-strings/*.raw; do \
		OUT=`echo $$a | sed -e 's/.raw$$//' -e 's/raw-strings\///'`; \
		$(CPP) $(CPPFLAGS) $$a $$OUT; \
	done

clean:
	rm -f ../include/common/strings.h strings.*
