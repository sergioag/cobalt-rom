/* $Id: memory.h,v 1.1.1.1 2003/06/10 22:42:02 iceblink Exp $
 * Copyright (c) 1998-2000 Cobalt Networks Inc
 */

#ifndef COMMON_MEMORY_H
#define COMMON_MEMORY_H

#include <extra_types.h>

#define memcopy(a,b,c) memcpy((void *)a,(void *)b,c);

extern unsigned int mem_size;

void *memcpy(void *, const void *, unsigned int);
void *memmove(void *, const void *, unsigned int);
void *memset(void *, int, size_t);
int  memcmp(const void *, const void *, size_t);
void *sbrk(int);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
