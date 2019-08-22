/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * exceptions.c
 */

#ifdef REMOTE_GDB

#include <string.h>

#include "common/rom.h"
#include "common/exceptions.h"

unsigned int IDT[ 18 * 2 ]; /* room for the 18 exceptions */

void init_exceptions( void )
{
	memset( (void *) &IDT, 0x0, 18*2);
	lidt( (void *) &IDT, 18*2 );
}

void lidt( void * base, unsigned int limit )
{
	unsigned int i[2];
	
	i[0] = limit<<16;
	i[1] = (unsigned int) base;
	asm( "lidt (%0)": :"p" (((char *) i)+2));
}

void set_exception( int num, void *addr )
{
	IDT[num*2] = (((unsigned int)addr)&0x0000ffff) |
		(0x18<<16);
	IDT[(num*2)+1] = (((unsigned int)addr)&0xffff0000) |
		0x8000 | /*present*/
		(15<<8); /*Trap Gate*/
	
	lidt( (void *) &IDT, 18*2 );
}

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
