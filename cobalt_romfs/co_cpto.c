/* $Id: co_cpto.c,v 1.1.1.1 2003/06/10 22:41:34 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "crfs_block_stdio.h"
#include "crfs_func.h"

int 
main(int argc, char *argv[])
{
	CO_FILE *file_out;
	int fp_in;
	int r;
	unsigned char buff[CRFS_BLOCK_SIZE];
	
	if (argc != 4) {
		printf("usage: %s <fs file name> <unix file> <co file>\n",
			argv[0]);
		return -1;
	}
	
	init_stdio_block(argv[1]);
	crfs_init();
	
	fp_in = open(argv[2], O_RDONLY);
	if (fp_in < 0) {
		perror("open()");
		return -1;
	}
	
	if (crfs_create(argv[3])) {
		perror("crfs_create()");
		return -1;
	}
	
	file_out = crfs_open(argv[3]);
	if (file_out == NULL) {
		perror("crfs_open()");
		return -1;
	}

	r = read(fp_in, (void *)buff, CRFS_BLOCK_SIZE);
	while (r > 0) {
	    if( crfs_write(file_out, (void *)buff, r) < 0 )
	    {
		printf("crfs_write error\n");
		return -1;
	    }
	    r = read(fp_in, (void *)buff, CRFS_BLOCK_SIZE);
	}
	if (r < 0) {
		perror("read()");
		return -1;
	}

	crfs_flush();
	crfs_close(file_out);
	close(fp_in);
	deinit_stdio_block();

	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
