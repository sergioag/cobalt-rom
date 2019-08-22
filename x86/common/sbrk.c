/* $Id: sbrk.c,v 1.1.1.1 2003/06/10 22:41:53 iceblink Exp $
 * Copyright (c) 1997-2000 Cobalt Networks, Inc.
 */

#include <extra_types.h>
#include <io.h>

#include "common/rom.h"
#include "common/alloc.h"
#include "common/rammap.h"

static unsigned long high_water = RAM_HEAP_SPACE;
void *sbrk(int size)
{
	unsigned long brk = high_water;
	
	if( (high_water + size) > RAM_HEAP_SPACE_MAX )
	{
	    printf( "WARNING!  Rom out of heap space.\n" );
	    return NULL;
	}
	high_water += size;
	if( size > 0 )
	    memset((void *)brk, 0x0, size);

	return (void *)brk;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
