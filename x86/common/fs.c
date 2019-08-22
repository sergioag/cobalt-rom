/* $Id: fs.c,v 1.1.1.1 2003/06/10 22:41:45 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

/*
 * fs.c
 */

#include <stdlib.h>
#include <printf.h>

#include "common/rom.h"
#include "common/fs.h"
#include "common/rammap.h"
#include "common/bzlib.h"

#include "crfs_func.h"
#include "crfs_block.h"

unsigned int serial_load_size;

int fs_init(void)
{
	return crfs_init();
}

int 
load_from_romfs(char *fname)
{
	int read_bytes, total_read = 0;
	CO_FILE *fp=NULL;
	char buff[CRFS_BLOCK_SIZE];
	int compressed=0;

	if( strlen( fname ) < (int)(sizeof(buff) - 4) ) /* enough room for .bz2? */
	{
	    strcpy( buff, fname );
	    strcat( buff, ".bz2" );
	    fp = crfs_open( buff );
	    if( fp != NULL )
		compressed = 1;
	}
	
	if( fp == NULL )
	    fp = crfs_open(fname);
	if (fp == NULL) {
		printf("Can't open crfs file \"%s\"\n", fname);
		return -1;
	}
	
	read_bytes = crfs_read(fp, (void *)&buff, CRFS_BLOCK_SIZE);
	while (read_bytes) {
		memcpy((void *)(RAM_SERIAL_LOAD+total_read),
			(void *)&buff, read_bytes );
		total_read += read_bytes;
		read_bytes = crfs_read(fp, (void *)&buff, 
			CRFS_BLOCK_SIZE);
	}
	
	if( compressed )
	{
	    int err;
	    unsigned int destLen= RAM_DECOMP_AREA_MAX-RAM_DECOMP_AREA;

	    if( (err = BZ2_bzBuffToBuffDecompress( (char *)RAM_DECOMP_AREA, &destLen,
						   (char *)RAM_SERIAL_LOAD, total_read, 
						   1, 0 )) != BZ_OK )
	    {
		printf( "Error bunziping monitor:\n" );
		return -1;
	    }
	    
	    memcpy( (char *)RAM_SERIAL_LOAD, (char *)RAM_DECOMP_AREA, destLen );
	    total_read=destLen;
	}

	serial_load_size = total_read;
	
	
	return total_read;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
