/* $Id: co_touch.c,v 1.1.1.1 2003/06/10 22:41:34 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

#include <stdio.h>
#include <stdlib.h>
#include "crfs_block_stdio.h"
#include "crfs_func.h"

int 
main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("usage: %s <fs file name> <crfs file>\n", argv[0]);
		return -1;
	}
	
	init_stdio_block(argv[1]);
	crfs_init();
	
	if (crfs_create(argv[2])) {
		perror("crfs_create()");
		return -1;
	}

	crfs_flush();
	deinit_stdio_block();

	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
