/* $Id: microcode.c,v 1.1.1.1 2003/06/10 22:42:21 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 *
 * Duncan Laurie
 */
#include <string.h>
#include <printf.h>

#include "common/rom.h"
#include "common/microcode.h"
#include "common/systype.h"

#include "microcode.h"

int microcode_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;

	(void) update_cpu_microcode();

	return 0;
}    

int cpuid_func(int argc, char *argv[])
{
	unsigned int eax,ebx,ecx,edx;
	unsigned long op;

	if (argc != 2)
		return EBADNUMARGS;

	if (stoli(argv[1], &op) != 0)
		return EBADNUM;

	cpuid(op, &eax, &ebx, &ecx, &edx);

	printf("\nCPUID input %d\n", (unsigned int)op);
	printf("eax = 0x%08x\nebx = 0x%08x\necx = 0x%08x\nedx = 0x%08x\n",
	       eax, ebx, ecx, edx);

	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
