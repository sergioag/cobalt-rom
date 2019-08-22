# $Id: Makefile,v 1.3 2003/12/11 00:55:04 thockin Exp $
#
# Copyright 1999, Cobalt Networks, Inc.

include defaults.mk
include ROM.conf

.EXPORT_ALL_VARIABLES:

TOPDIR   := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
SPECS     = $(shell echo spec/*.spec)
LINUXDIR := $(TOPDIR)/linux

.PHONY: all
all: $(LINUXDIR)/vmlinux.bz2
	@echo $(VERSION) > .version
	$(MAKE) x86

x86: common_deps x86-dir cobalt_romfs-dir x86/cobalt-x86.rom
	@echo
	@for a in $(SPECS); do tools/makerom.pl $$a; done

x86-dir:
	$(MAKE) -C x86

cobalt_romfs-dir:
	$(MAKE) -C cobalt_romfs

clean: 
	@:> rom.mk
	$(MAKE) -C cobalt_romfs clean
	$(MAKE) -C x86 clean
	$(MAKE) -C tools clean
	$(RM) *.rom
	$(RM) buildinfo.mk rom.mk .version cobalt.logo

kclean:
	$(MAKE) -C $(LINUXDIR) clean

distclean: clean
	$(RM) linux


# these are support rules

common_deps: buildinfo romconf makerules tools cobalt.logo

buildinfo:
	@echo -ne "buildinfo.mk: "
	@rm -f buildinfo.mk
	@D=`date`; \
	H=${HOSTNAME:-`hostname -s`}; \
	N=${DOMAINNAME:-`dnsdomainname`}; \
	export BUILDDATE="$$D $$H.$$N";\
	echo BUILDDATE=$$D $$H.$$N > buildinfo.mk;\
	echo BUILDINFO=\"\\\"$$BUILDDATE\\\"\" >> buildinfo.mk
	@echo "done"

romconf:
	@echo -ne "romconf.h: "
	@./tools/buildh.pl ROM.conf > x86/include/common/romconf.h
	@echo "done"

makerules:
	@echo -ne "rom.mk: "
	@rm -f rom.mk
	@( \
	echo "TOPDIR = $(TOPDIR)"; \
	echo "LINUXDIR = $(LINUXDIR)"; \
	echo "CURDIR = \$$(shell pwd | sed 's|\$$(TOPDIR)/||')"; \
	echo "include \$$(TOPDIR)/defaults.mk"; \
	echo "include \$$(TOPDIR)/buildinfo.mk"; \
	echo "include \$$(TOPDIR)/ROM.conf"; \
	) > rom.mk
	@echo "done"

.PHONY: tools
tools:
	$(MAKE) -C tools

cobalt.logo: cobalt.title icons/null.xpm icons/cobalt.xpm icons/memory.xpm \
		icons/disk.xpm icons/clock.xpm
	@echo -ne "cobalt.logo: "
	@rm -f cobalt.logo
	@( \
	cat cobalt.title; \
	./tools/xpm2font.pl 0 < icons/cobalt.xpm; \
	./tools/xpm2font.pl 8 < icons/memory.xpm; \
	./tools/xpm2font.pl 16 < icons/disk.xpm; \
	./tools/xpm2font.pl 24 < icons/clock.xpm; \
	) > cobalt.logo
	@echo "done"

tags:
	find cobalt_romfs x86 -name '*.[ch]' | xargs etags

$(LINUXDIR)/vmlinux.bz2:
	$(MAKE) HOSTCC=gcc -C $(LINUXDIR) rom

