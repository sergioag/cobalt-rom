/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: gdb.c,v 1.1.1.1 2003/06/10 22:42:19 iceblink Exp $
 */

#include "common/gdb.h"
#include "common/rom.h"

#include "gdb.h"

#ifdef REMOTE_GDB
int debug_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;
		
	breakpoint();
	
	return 0;
}
#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
