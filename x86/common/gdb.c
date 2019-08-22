/*
 * $Id: gdb.c,v 1.1.1.1 2003/06/10 22:41:46 iceblink Exp $
 */

#ifdef REMOTE_GDB

#include <io.h>
#include <printf.h>

#include "common/rom.h"
#include "common/exceptions.h"
#include "common/serial.h"
#include "common/gdb.h"

void set_debug_traps();

int getDebugChar(void)
{
	unsigned char b;
	serial_inb(&b);
	return b;
}

int putDebugChar(int ch)
{
	print_char(ch);
	return 1;
}

void exceptionHandler(int exc, void *addr)
{
    set_exception(exc, addr);
}

void flush_i_cache(void)
{
   __asm__ __volatile__ ("jmp 1f\n1:");
}

void init_debug( void )
{
	set_debug_traps();
}

void exceptionHook( int ex )
{
	printf("Exception: %d\n", ex );
}

/* 
void *memset(void *ptr, int val, unsigned int len); 

   needs to be defined if it hasn't been already
*/

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
