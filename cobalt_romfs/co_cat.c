/* $Id: co_cat.c,v 1.1.1.1 2003/06/10 22:41:34 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "crfs_block_stdio.h"
#include "crfs_func.h"

int 
main(int argc, char *argv[])
{
	CO_FILE *file_in;
	int read_bytes;
	unsigned char buff[CRFS_BLOCK_SIZE];
	
	if (argc != 3) {
		printf("usage: %s <fs file name> <co file>\n", argv[0]);
		return -1;
	}
	
	init_stdio_block(argv[1]);
	crfs_init();
	
	file_in = crfs_open(argv[2]);
	if (file_in == NULL) {
		perror("crfs_open()");
		return -1;
	}

	memset(buff, 0, sizeof(buff));
	read_bytes = crfs_read(file_in, (void *)buff, CRFS_BLOCK_SIZE);
	while (read_bytes > 0) {
		write(1, (void *)buff, read_bytes);
		memset(buff, 0, sizeof(buff));
		read_bytes = crfs_read(file_in, (void *)buff, CRFS_BLOCK_SIZE);
	}

	if (read_bytes < 0) {
		perror("crfs_read()");
	}
	
	crfs_close(file_in);
	deinit_stdio_block();

	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
