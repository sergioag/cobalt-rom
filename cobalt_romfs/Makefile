# $Id: Makefile,v 1.2 2003/12/11 00:55:04 thockin Exp $
#
# Copyright (c) 2000 Cobalt Networks, Inc.
#

include ../rom.mk

CFLAGS += -ggdb -O0 -DCOBALT_ROMFS_INTERNAL $(EXTRA_CFLAGS)
#DEBUG = -lefence

OBJS = crfs_block_stdio.o crfs_func.o
BINS = co_mkfs co_ls co_touch co_cpto co_cpfrom co_cat co_mkdir co_rm co_rmdir

all: $(BINS)

$(BINS): %: $(OBJS) *.h %.c
	@echo -ne "$(CURDIR)/$@: "
	$(CC) $(CFLAGS) -o $@ $(OBJS) $@.c $(DEBUG)
	@echo "done"

clean:
	rm -f *.o $(BINS) boot_fs
