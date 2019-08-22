/* $Id: crfs_block.h,v 1.1.1.1 2003/06/10 22:41:34 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

/*
 * crfs_block.h
 *
 * provides access to the blocks.  The actual source will differ
 * depending on the underlying storage mechanism
 */

#ifndef COBALT_ROMFS_BLOCK_H
#define COBALT_ROMFS_BLOCK_H

#include "crfs.h"

int read_block(block_id_t block_ndx, void *block);
int write_block(block_id_t block_ndx, void *block);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
