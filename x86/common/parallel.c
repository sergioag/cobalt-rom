/*
 *
 * Filename: parallel.c
 *
 * Description: Functions for supporting parallel I/O
 *
 * Author(s): Timothy Stonis, Andrew Bose, Erik Gilling, Tim Hockin, Duncan Laurie
 *
 * Copyright 1997,1998, 1999 Cobalt Networks, Inc.
 *
 */

#include <string.h>
#include <io.h>

#include "common/rom.h"
#include "common/parallel.h"
#include "common/isapnp.h"
#include "common/systype.h"

void init_parallel_port(void)
{
	switch (systype())
	{
	case SYSTYPE_3K:

		/* Enter Config Mode */
		outb(0x51, PNP_CONFIG_PORT);
		outb(0x23, PNP_CONFIG_PORT);
	
		/* enable Parrallel Port */
		outb(0x07, PNP_CONFIG_PORT); /* Point to Device Select Register */
		outb(0x03, PNP_DATA_PORT);   /* logic device 3 (parrallel port)*/
	
		outb(0x30, PNP_CONFIG_PORT); /* Register 0x30 */
		outb(0x01, PNP_DATA_PORT);   /* enable || port */
	
		outb(0xf0, PNP_CONFIG_PORT); /* Register 0x30 */
		outb(0x01, PNP_DATA_PORT);   /* enable EPP 1.9 */
	
		/* Exit config mode */
		outb(0xBB, PNP_CONFIG_PORT);

		break;

	}
}




// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
