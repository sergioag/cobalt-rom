# $Id: Makefile,v 1.1.1.1 2003/06/10 22:42:27 iceblink Exp $

TESTS = memtest

all:
	@for a in $(TESTS); do \
		make -C $$a || exit 1; \
	done

clean:
	@for a in $(TESTS); do \
		make -C $$a clean || exit 1; \
	done
