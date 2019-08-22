/* $Id: memfuncs.c,v 1.1.1.1 2003/06/10 22:41:49 iceblink Exp $
 * Copyright (c) 1997-2000 Cobalt Networks, Inc.
 */

#include "common/rom.h"
#include "common/memory.h"

/*********************
 *                   *
 *     memory.h      *
 *                   *
 *********************/

void *
memcpy(void *reg2, const void *reg1, unsigned int n)
{
#ifdef DISABLE_ASM_OPT
	while (n--)
		*((unsigned char *)reg2)++ = *((unsigned char *)reg1)++;
#else
	asm("cld\n\t" "rep\n\t" "movsb": :"c"(n), "S"(reg1), "D"(reg2) 
		: "memory");
#endif
	
	return reg2;
}

void *
memmove(void *dest, const void *src, unsigned int n)
{
	if (dest < src)
		memcpy(dest, src, n);
	else {
		dest += n-1;
		src += n-1;
		while (n--)
			*((unsigned char *)dest)-- = *((unsigned char *)src)--;
	}

	return dest;
}

void *
memset(void *mem, int c, size_t n)
{
#ifdef DISABLE_ASM_OPT
	size_t i = 0;
	unsigned char *p = mem;
	while (i++ < n) {
		*p++ = c;
	}
#else
	__asm__ __volatile__ (
		"cld \n\t" 
		"rep \n\t" 
		"stosb \n\t"
		: :"c" (n), "a" (c), "D" (mem) : "memory");
#endif

	return mem;
}

int 
memcmp(const void *s1, const void *s2, size_t n)
{
	unsigned char *p1, *p2;

	p1 = (unsigned char *)s1;
	p2 = (unsigned char *)s2;

	while (n--) {
		if (*p1 - *p2)
			return (*p1 - *p2);
		p1++;
		p2++;
	}
	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
