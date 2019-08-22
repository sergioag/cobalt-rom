# $Id: defaults.mk,v 1.4 2003/12/11 00:55:04 thockin Exp $
#
# Copyright (c) 1999-2000 Cobalt Networks, Inc.
# Copyright (c) 2001 Sun Microsystems, Inc.
#

# ROM version information
#
V_MAJ   = 2
V_MIN   = 10
V_REV   = 3
V_EXTRA = 
VERSION = $(V_MAJ).$(V_MIN).$(V_REV)$(V_EXTRA)
VERSION_DEFS = -DVERSION=\"$(VERSION)\" \
	-DV_MAJ=$(V_MAJ) -DV_MIN=$(V_MIN) -DV_REV=$(V_REV)

# Debug flags
#
DEBUG_FLAGS  := -DDEBUG_BOOT=0 -DDEBUG_PCI=0

# Make rules
# 
CFLAGS := -Os -Wall -W -Werror -Winline \
	  -fno-pic -fno-builtin -fno-strict-aliasing \
	  -I$(LINUXDIR)/include $(DEBUG_FLAGS) $(DEBUG)

# various system-specific variables
#
LINUXCFG := config-cobalt-ROM

# programs used for building
#
CC       := $(shell which kgcc >/dev/null 2>&1 && echo kgcc || echo gcc)
CCC      := g++
CPP      := /lib/cpp
LIBGCC   := $(shell ($(CC) -print-libgcc-file-name))
LD       := ld
AR       := ar
AS       := as
ASM      := nasm
MAKE     := make VERBOSE=$(VERBOSE)
BUILDBIN := $(TOPDIR)/tools/buildbin
SETLOGO  := $(TOPDIR)/tools/romsetlogo.pl
CSUM     := $(TOPDIR)/tools/romcsum


.c.o:
	$(CC) -c $(CFLAGS) $(CDEFS) -o $@ $<

.S.o:
	$(CC) -c $(CFLAGS) $(ASDEFS) -o $@ $<

.s.o:
	$(AS) -o $@ $<

ifeq "$(VERBOSE)" ""
.SILENT:
endif

