/* unzip.c -- decompress files in gzip or pkzip format.
 * Copyright (C) 1992-1993 Jean-loup Gailly
 *
 * Adapted for Linux booting by Hannu Savolainen 1993
 *
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License, see the file COPYING.
 *
 * The code in this file is derived from the file funzip.c written
 * and put in the public domain by Mark Adler.
 */

/*
 * This version can extract files in gzip format
 */

#include "common/rom.h"
#include "common/rammap.h"
#include "common/gzip.h"

/*
 * IN assertions: the buffer inbuf contains already the beginning of
 *   the compressed data, from offsets inptr to insize-1 included.
 *   The magic header has already been checked. The output buffer is cleared.
 */
int  unzip(void)
{
	unsigned long orig_crc = 0;       /* original crc */
	unsigned long orig_len = 0;       /* original uncompressed length */
	int n;

	if (method != DEFLATED) {
		error("internal error, invalid method");
		return -1;
	}

	/* decompress (inflate) */
	n = inflate();

	if (n == 3) {
		error("out of memory");
		return -1;
	}
	else if (n != 0) {
		error("invalid compressed format");
		return -1;
	}

	/* Get the crc and original length */
	/* crc32  (see algorithm.doc)
	 * uncompressed input size modulo 2^32
	 */
	orig_crc  = (unsigned long) get_byte();
	orig_crc |= (unsigned long) get_byte() << 8;
	orig_crc |= (unsigned long) get_byte() << 16;
	orig_crc |= (unsigned long) get_byte() << 24;
	
	orig_len  = (unsigned long) get_byte();
	orig_len |= (unsigned long) get_byte() << 8;
	orig_len |= (unsigned long) get_byte() << 16;
	orig_len |= (unsigned long) get_byte() << 24;
	
#ifdef DEBUGROM
	printf(" gzip crc32: %0x\n", (unsigned int)orig_crc);
	printf(" gzip bytes: %d\n", (unsigned int)bytes_out);
#endif

	/* Validate decompression */
	if (orig_len != bytes_out) {
		error("length error");
		return -1;
	}
	return 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
