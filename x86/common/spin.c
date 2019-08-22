/* $Id: spin.c,v 1.1.1.1 2003/06/10 22:41:54 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

/*
 * spin.c - ascii spinner
 * 
 * Gideon Glass  <gid@cobaltnet.com>  
 * copyright (c) 1998,1999  Cobalt Networks, Inc
 */

#include <printf.h>

#include "common/rom.h"
#include "common/spin.h"

static int spinstat; 

void spin(void)
{
    char * phases[] = { "\b", "\\", "\b", "|", "\b", "/", "\b", "-" };

    printf(phases[spinstat++]);
    spinstat %= 8;
    printf(phases[spinstat++]);
    spinstat %= 8;
}

void spin_reset( void )
{
    spinstat = 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
