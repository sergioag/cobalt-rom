/* $Id: io.c,v 1.1.1.1 2003/06/10 22:41:48 iceblink Exp $
 * Copyright (c) 1997-2000 Cobalt Networks, Inc.
 */

#include <extra_types.h>
#include <io.h>

#include "common/rom.h"
#include "common/alloc.h"

static unsigned int roundup2(unsigned int num);

static unsigned int io_base = COBALT_IO_BASE;
unsigned int 
io_get_region(unsigned int size)
{
	unsigned int i;

	if (size == 0) {
		return io_base;
	}

	/* uh oh! */
	if (io_base + size > COBALT_IO_LIMIT) {
		return 0;
	}

	io_align(size);

	/* save this value to return */
	i = io_base;

	/* advance current pointer */
	io_base += size;

	return i;
}

unsigned int 
io_align(unsigned int size)
{
	unsigned int align_sz;

	/* round up size */
	size = roundup2(size);

	/* minimum alignment is 4 */
	align_sz = (size-1) | 0x3;

	/* align to size of requested space */
	if (io_base & align_sz) {
		io_base = (io_base & (~align_sz)) + size;
	}

	return io_base;
}

static unsigned long mmio_base = COBALT_MMIO_BASE;
unsigned long 
mmio_get_region(unsigned int size)
{
	unsigned int i;

	if (size == 0) {
		return mmio_base;
	}

	/* uh oh! (avoid overflow) */
	if (size > (COBALT_MMIO_LIMIT - mmio_base)) {
		return 0;
	}

	mmio_align(size);

	/* save this value to return */
	i = mmio_base;

	/* advance current pointer */
	mmio_base += size;

	return i;
}

unsigned long 
mmio_align(unsigned int size)
{
	unsigned int align_sz;

	/* round up size */
	size = roundup2(size);

	/* minimum alignment is 16 */
	align_sz = (size-1) | 0xf;

	/* align to size of requested space */
	if (mmio_base & align_sz) {
		mmio_base = (mmio_base & (~align_sz)) + size;
	}

	return mmio_base;
}

static unsigned long pmmio_base = COBALT_PMMIO_BASE;
unsigned long 
pmmio_get_region(unsigned int size)
{
	unsigned int i;

	if (size == 0) {
		return pmmio_base;
	}

	/* uh oh! */
	if (size > (COBALT_PMMIO_LIMIT - pmmio_base)) {
		return 0;
	}

	pmmio_align(size);

	/* save this value to return */
	i = pmmio_base;

	/* advance current pointer */
	pmmio_base += size;

	return i;
}

unsigned long 
pmmio_align(unsigned int size)
{
	unsigned int align_sz;

	/* round up size */
	size = roundup2(size);

	/* minimum alignment is 16 */
	align_sz = (size-1) | 0xf;

	/* align to size of requested space */
	if (pmmio_base & align_sz) {
		pmmio_base = (pmmio_base & (~align_sz)) + size;
	}

	return pmmio_base;
}

static unsigned int
roundup2(unsigned int num)
{
	int shifts = 0;
	int aligned = 1;
	int n = num;

	while (n) {
		if ((n > 1) && (n&1)) {
			aligned = 0;
		}
		n >>= 1;
		shifts++;
	}

	return (aligned ? (num) : (1 << shifts));
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
