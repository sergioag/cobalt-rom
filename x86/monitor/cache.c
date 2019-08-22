/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: cache.c,v 1.1.1.1 2003/06/10 22:42:18 iceblink Exp $
 */

#include <string.h>

#include "common/rom.h"
#include "common/cache.h"

#include "cache.h"

int 
enl1_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if(argc != 1)
		return EBADNUMARGS;

	cache_set_caches( 1, cache_l2_should_be() );
	cache_l1_on();

	
	return 0;
}

int 
enl2_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if(argc != 1)
		return EBADNUMARGS;
		
	cache_set_caches( cache_l1_should_be(), 1 );
	cache_l2_on();

	return 0;
}

int 
set_caches_func(int argc, char *argv[])
{
	long l1 = 1;
	long l2 = 1;
	
	if(argc != 3)
		return EBADNUMARGS;

	if((stoli(argv[1], &l1) != 0) || (stoli(argv[2], &l2) != 0)) 
		return EBADNUM;

	cache_set_caches( l1, l2 );
	return 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
