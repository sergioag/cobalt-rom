/* $Id: crfs_block_stdio.c,v 1.1.1.1 2003/06/10 22:41:34 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

/*
 * crfs_block_stdio.c
 *
 * provides access to the blocks from a file.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "crfs_block_stdio.h"

int co_romfs_fd = 0;

int 
init_stdio_block(char *fname)
{
	co_romfs_fd = open(fname, O_RDWR | O_CREAT, 0666);
	if (co_romfs_fd < 0) {
		printf("init_stdio_block(\"%s\") failed\n", fname);
		exit(1);
	}
	
	return 0;
}

void 
deinit_stdio_block(void)
{
	if (co_romfs_fd) {
		close(co_romfs_fd);
		co_romfs_fd = 0;
	}
}

int 
read_block(uint16 block_ndx, void *block)
{
	if (!co_romfs_fd) {
		return -1;
	}
	
	lseek(co_romfs_fd, block_ndx * CRFS_BLOCK_SIZE, SEEK_SET);	

	/* FIXME: could read short */
	read(co_romfs_fd, block, CRFS_BLOCK_SIZE);
	return 0;
}

int 
write_block(uint16 block_ndx, void *block)
{
	if (!co_romfs_fd) {
		return -1;
	}

	lseek(co_romfs_fd, block_ndx * CRFS_BLOCK_SIZE, SEEK_SET);	

	write(co_romfs_fd, block, CRFS_BLOCK_SIZE);
	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
