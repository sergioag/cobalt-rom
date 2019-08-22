/* $Id: region.h,v 1.1.1.1 2003/06/10 22:42:03 iceblink Exp $
 *
 * Copyright (c) 2000 Cobalt Networks, Inc.
 * Copyright (c) 2001 Sun Microsystems, Inc.
 */
#ifndef COMMON_REGION_H
#define COMMON_REGION_H

#include <extra_types.h>

void region_init(uint8, uint8, uint32, const char *);
uint32 region_align(uint8, uint32);
uint32 region_allocate(uint8, uint32);
uint32 region_get_size(uint8);
uint32 region_get_base(uint8);
uint32 region_get_limit(uint8);
const char * region_get_name(uint8);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
