/* $Id: co_mkfs.c,v 1.1.1.1 2003/06/10 22:41:34 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

#include <stdio.h>
#include <stdlib.h>
#include "crfs_block_stdio.h"
#include "crfs_func.h"

int cvt_int(char *numstr, int *errp);

int 
main(int argc, char *argv[])
{
	int val, err;

	if (argc != 3) {
		printf("usage: %s <fs file name> <size>\n", argv[0]);
		return -1;
	}
	
	init_stdio_block(argv[1]);
	
	val = cvt_int(argv[2], &err);
	if (err) {
		printf("%s is not a number\n", argv[2]);
		return -1;
	}
	
	printf("Making crfs of size %d %s\n", 
		val > 1024 ? val/1024 : val, 
		val > 1024 ? "kbytes" : "bytes");

	crfs_mkfs(val);
	
	deinit_stdio_block();

	return 0;
}

int 
cvt_int(char *numstr, int *errp)
{               
	long val;   
	char *cvt;

	val = strtol(numstr, &cvt, 0);

	if (cvt == numstr) {
		*errp = 1;  
		return 0;       
	}               

	switch (*cvt) {  
		case 'm': case 'M':
			val *= 1024 * 1024;
			break;
	
		case 'k': case 'K':
			val *= 1024;
			break;
	}

	*errp = 0;
	return val;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
