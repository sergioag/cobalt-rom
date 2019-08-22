/* $Id: crfs_block_stdio.h,v 1.1.1.1 2003/06/10 22:41:35 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

/*
 * crfs_block_stdio.h
 *
 * provides access to the blocks from a file.
 */

#ifndef COBALT_ROMFS_BLOCK_STDIO_H
#define COBALT_ROMFS_BLOCK_STDIO_H

#include "crfs_block.h"

int init_stdio_block( char * fname );
void deinit_stdio_block( void );


#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
