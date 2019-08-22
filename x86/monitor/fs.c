/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: fs.c,v 1.1.1.1 2003/06/10 22:42:19 iceblink Exp $
 */

#include <errno.h>
#include <extra_types.h>

#include "common/rom.h"
#include "common/fs.h"

#include "fs.h"

int 
ls_func(int argc, char *argv[])
{
	crfs_dirent *de;
	CO_DIR *dir;
	char *dirname;
	
	if (argc != 2 && argc != 1) {
		return EBADNUMARGS; 
	}
	if (argc == 1) {
		dirname = ".";
	} else {
		dirname = argv[1];
	}
	
	dir = crfs_opendir(dirname);
	if (dir == NULL) {
		printf("can't crfs_opendir(\"%s\"): %d\n", dirname, errno);
		return -1;
	}
	
	de = crfs_readdir(dir);
	while (de && de->de_inode && de->de_name) {
		struct crfs_stat stbuf;
		char buf[strlen(dirname) + strlen(de->de_name) + 2];
		snprintf(buf, sizeof(buf), "%s/%s", dirname, de->de_name);

		if (crfs_stat(buf, &stbuf))
			printf("can't crfs_stat(\"%s\"): %d\n", buf, errno);
		else if (stbuf.st_fmask & CRFS_FMASK_DIR)
			printf("%8d %s/\n", stbuf.st_size, de->de_name); 
		else
			printf("%8d %s\n", stbuf.st_size, de->de_name); 

		de = crfs_readdir(dir);		
	}
	crfs_closedir(dir);
	
	return 0;
}

int 
load_func(int argc, char *argv[])
{
	if (argc != 2)
		return EBADNUMARGS;
		
	if (load_from_romfs(argv[1]) < 1)
		return -1;
	
	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
