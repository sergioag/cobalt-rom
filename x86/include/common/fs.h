/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: fs.h,v 1.1.1.1 2003/06/10 22:42:01 iceblink Exp $ */

/*
 * fs.h
 */
#ifndef COMMON_FS_H
#define COMMON_FS_H

#include "crfs_func.h"
#include "crfs_block.h"

int fs_init(void);
int load_from_romfs(char *fname);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
