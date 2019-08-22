/* $Id: mfg-rtc-setup.c,v 1.1 2004/01/23 02:16:51 thockin Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

/* 
 * Tim Hockin
 * Cobalt Networks
 * (c) 2000
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <string.h>

int main(int argc, char *argv[])
{
	unsigned char addr, val, tmp;

	if (ioperm(0x70, 2, 1)) {
		perror("ioperm");
		return 2;
	}

	/* turn on SET bit */
	outb(0xb, 0x70);
	tmp = inb(0x71);
	outb(tmp | 0x80, 0x71);

	outb(0xa, 0x70);
	outb(0x26, 0x71);

	outb(0xb, 0x70);
	outb(0x2, 0x71);
	
	/* turn off SET bit */
	outb(0xb, 0x70);
	tmp = inb(0x71);
	outb(tmp & ~0x80, 0x71);

	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
