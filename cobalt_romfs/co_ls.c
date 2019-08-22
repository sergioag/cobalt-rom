/* $Id: co_ls.c,v 1.1.1.1 2003/06/10 22:41:34 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "crfs_block_stdio.h"
#include "crfs_func.h"


int 
main(int argc, char *argv[])
{
	crfs_dirent *co_dirent;
	CO_DIR *dir;
	char *dirname;
	struct crfs_stat stbuf;
	
	if (argc < 2) {
		printf("usage: %s <fs file name> <dir>\n", argv[0]);
		return -1;
	}

	if (argc != 3) {
		dirname = ".";
	} else {
		dirname = argv[2];
	}
	
	init_stdio_block( argv[1]);
	crfs_init();
	
	dir = crfs_opendir(dirname);
	if (dir == NULL) {
		perror("crfs_opendir()");
		return -1;
	}

	co_dirent = crfs_readdir(dir);
	while (co_dirent) {
		char buf[strlen(dirname) + strlen(co_dirent->de_name) + 2];
		snprintf(buf, sizeof(buf), "%s/%s", dirname,
			co_dirent->de_name);
		if (crfs_stat(buf, &stbuf)) {
			fprintf(stderr, "crfs_stat(\"%s\"): %s\n",
				buf, strerror(errno));
			break;
		}
		if (stbuf.st_fmask & CRFS_FMASK_DIR) {
			printf("%8d %s/\n", stbuf.st_size, co_dirent->de_name); 
		} else {
			printf("%8d %s\n", stbuf.st_size, co_dirent->de_name); 
		}
			
		co_dirent = crfs_readdir(dir);		
	}
	crfs_closedir(dir);

	deinit_stdio_block();

	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
