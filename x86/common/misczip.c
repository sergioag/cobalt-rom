/*
 * 
 * Filename: misczip.c
 * 
 * Description: This file contains some support routines for the kernel 
 *              decompression (much borrowed from milo)
 * 
 * Author(s): Timothy Stonis
 * 
 * Copyright 1997,1999 Cobalt Networks, Inc.
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <printf.h>

#include "common/rom.h"
#include "common/gzip.h"
#include "common/serial.h"
#include "common/misczip.h"
#include "common/spin.h"
#include "common/rammap.h"


unsigned char *input_buffer;

extern struct bootfs *bfs;
extern char *dest_addr;
extern long bytes_to_copy;

unsigned char *inbuf;
unsigned char *window;
unsigned outcnt;
unsigned insize;
unsigned inptr;
unsigned long bytes_out;
int method;
int block_number;

char *dest_addr;

/*
 * Clear input and output buffers
 */
void
clear_bufs(void)
{
	outcnt = 0;
	insize = inptr = 0;
	bytes_out = 0;
}

void
error(char *x)
{
	printf("ERROR: %s\n", x);
#if 0
	_main ();
#endif
}

/*
 * Check the magic number of the input file and update ofname if an
 * original name was given and to_stdout is not set.
 * Return the compression method, -1 for error, -2 for warning.
 * Set inptr to the offset of the next byte to be processed.
 * This function may be called repeatedly for an input file consisting
 * of several contiguous gzip'ed members.
 * IN assertions: there is at least one remaining compressed member.
 */
int
get_method (void)
{
	unsigned char flags;
	char magic[2]; /* magic header */

	magic[0] = get_byte();
	magic[1] = get_byte();

	method = -1;                 /* unknown yet */
	if (memcmp(magic, GZIP_MAGIC, 2) == 0
	    || memcmp(magic, OLD_GZIP_MAGIC, 2) == 0) {

		method = get_byte();
		flags  = get_byte();

		if ((flags & ENCRYPTED) != 0)
			error("Input is encrypted");
		if ((flags & CONTINUATION) != 0)
			error("Multi part input");
		if ((flags & RESERVED) != 0)
			error("Input has invalid flags");
		  
		get_byte();	/* skip over timestamp */
		get_byte();
		get_byte();
		get_byte();

		get_byte();	/* skip extra flags */
		get_byte();	/* skip OS type */

		if ((flags & EXTRA_FIELD) != 0) {
			unsigned len = get_byte();
			len |= get_byte() << 8;
			while (len--) get_byte();
		}

		/* Get original file name if it was truncated */
		if ((flags & ORIG_NAME) != 0) {
			/* skip name */
			while (get_byte() != 0) /* null */ ;
		}

		/* Discard file comment if any */
		if ((flags & COMMENT) != 0) {
			while (get_byte() != 0) /* null */ ;
		}
	} else error("unknown compression method");
		
	return method;
}

/*
 * write the output window window[0..outcnt-1] holding uncompressed data
 */
void
flush_window (void)
{
	spin();

	if (!outcnt)
		return;

	bytes_out += outcnt;

	if (dest_addr) {
		/* Simply copy adjacent to previous block */
		memcopy((char*)dest_addr, window, outcnt);
		dest_addr += outcnt;
	}
}

/*
 * Fill the input buffer and return the first byte in it. This is called
 * only when the buffer is empty and at least one byte is really needed.
 */
int
fill_inbuf (void)
{
	memcopy(inbuf, (unsigned char *)(input_buffer + (block_number * INBUFSIZ)), INBUFSIZ);
	block_number++;
	insize = INBUFSIZ;
	inptr = 1;
	return inbuf[0];
}

int
uncompress_kernel (void *where)
{
	make_tables();
	block_number = 0;

	inbuf  = malloc(INBUFSIZ);
	window = malloc(WSIZE);

	dest_addr    = (unsigned char *) RAM_DECOMP_AREA;
	input_buffer = (unsigned char *) where;
    
	clear_bufs();
	method = get_method();
	return unzip();
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
