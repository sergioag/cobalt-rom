/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: x86.h,v 1.1.1.1 2003/06/10 22:42:04 iceblink Exp $ */

#ifndef COMMON_X86_H
#define COMMON_X86_H

/* x86 specific macros, functions, and definitons */

/* get both halves of TSC */
static inline void
rdtsc(unsigned long *lo, unsigned long *hi)
{
	__asm__ __volatile__("rdtsc" : "=a" (*lo), "=d" (*hi));
}

/* get low half of TSC */
static inline unsigned long
rdtscl(void)
{
	unsigned long time;

	__asm__ __volatile__("rdtsc" : "=a" (time) : : "edx");

	return time;
}

/* get both halves of TSC as a long long */
static inline unsigned long long
rdtscll(void)
{
	unsigned long long time;

	__asm__ __volatile__("rdtsc" : "=A" (time));

	return time;
}

/* run the cpuid instruction */
static inline void
cpuid(int op, int *a, int *b, int *c, int *d)
{
	__asm__ __volatile__("cpuid" \
		: "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d) \
		: "a" (op) \
		: "cc");
}

/* force a serialization of the cpu */
static inline void
cpu_serialize()
{
	__asm__ __volatile__("cpuid" \
		: \
		: "a" (0) \
		: "ebx", "ecx", "edx", "cc");
}

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
