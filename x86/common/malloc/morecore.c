/* $Id: morecore.c,v 1.1.1.1 2003/06/10 22:42:00 iceblink Exp $ */

/* morecore.c - C library support routine for UNIX.
   Copyright (c) 1989, 1993  Michael J. Haertel
   You may redistribute this library under the terms of the
   GNU Library General Public License (version 2 or any later
   version) as published by the Free Software Foundation.
   THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY EXPRESS OR IMPLIED
   WARRANTY.  IN PARTICULAR, THE AUTHOR MAKES NO REPRESENTATION OR
   WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY OF THIS
   SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE. */

/*#include <limits.h>
#include <stddef.h>*/

#include "common/rom.h"
#include "common/memory.h"
#include "malloc.h"

/* Note that morecore has to take a signed argument so
   that negative values can return memory to the system. */
void *
_default_morecore(long size)
{
    void *result;

    result = sbrk(size);
    if (result == (void *) -1)
	return NULL;
    return result;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
