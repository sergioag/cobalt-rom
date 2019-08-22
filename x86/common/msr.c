/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: msr.c,v 1.1.1.1 2003/06/10 22:41:50 iceblink Exp $
 */

#include "common/msr.h"
#include "common/delay.h"

unsigned long long msr_get(unsigned int index)
{
	unsigned long long msr;
    
	__asm__ __volatile__("rdmsr"
		: "=A" (msr)
		:  "c" (index));

	return msr;
}

void msr_set(unsigned int index, unsigned long long value)
{
	__asm__ __volatile__("wrmsr"
		:
		: "A" (value),
		  "c" (index));
}

void msr_set_on(unsigned int index, unsigned long long bits)
{
	unsigned long long value;

	__asm__ __volatile__("rdmsr" : "=A" (value) : "c" (index));

	if (!(value & bits)) {
		value |= bits;
		udelay(100);
		__asm__ __volatile__("wbinvd" ::: "memory");
		__asm__ __volatile__("wrmsr" :: "A" (value), "c" (index));
	}
}

void msr_set_off(unsigned int index, unsigned long long bits)
{
	unsigned long long value;

	__asm__ __volatile__("rdmsr" : "=A" (value) : "c" (index));

	if (value & bits) {
		value &= ~bits;
		udelay(100);
		__asm__ __volatile__("wbinvd" ::: "memory");
		__asm__ __volatile__("wrmsr" :: "A" (value), "c" (index));
	}
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
