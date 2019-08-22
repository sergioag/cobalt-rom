/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: i18n.c,v 1.1.1.1 2003/06/10 22:42:19 iceblink Exp $
 */

#include <string.h>

#include "common/rom.h"
#include "common/i18n.h"

#include "i18n.h"

int print_str_func( int argc, char *argv[] )
{
    unsigned long num;
    
    if( argc != 2 )
	return EBADNUMARGS;

    if( stoli( argv[1], &num ) != 0 )
	return EBADNUM;

    printf( "\"%s\"\r\n", i18n_lookup_str( num ) );

    return 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
