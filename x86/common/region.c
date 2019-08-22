/* $Id: region.c,v 1.2 2003/12/11 00:57:09 thockin Exp $
 *
 * Copyright (c) 1997-2000 Cobalt Networks, Inc.
 * Copyright (c) 2001 Sun Microsystems, Inc.
 */

#include <extra_types.h>

#include "common/region.h"
#include "common/rom.h"
#include "common/alloc.h"

/* region structure */
struct region_info {
	uint8 id;
	uint8 mask;
	uint32 limit;
	uint32 base;
	uint32 size;
	char *name;
	struct region_info *next;
};

static struct region_info *region_list;
static struct region_info *region_find(uint8);
static void roundup2(uint32 *);

/*
 * locate a region in the list if it exists
 */
static struct region_info *
region_find(uint8 id)
{
	struct region_info *cur = region_list;

	while (cur) {
		if (cur->id == id) {
			return cur;
		}
		cur = cur->next;
	}
	return NULL;
}

/*
 *  initialize io/memory region
 *  to keep track of allocations
 *
 *  needs unique ID,
 *  alignment mask,
 *  base address
 */
void
region_init(uint8 id, uint8 align, uint32 base, const char *name)
{
	struct region_info *new;
	struct region_info *cur = region_list;

	new = malloc(sizeof(*new));
	if (!new) {
		printf("malloc failed at %s:%d\n", __FILE__, __LINE__);
		return;
	}
	memset(new, 0, sizeof(*new));

	/* add region to end of region list */
	cur = region_list;
	if (!cur) {
		region_list = new;
	}
	else {
		while (cur->next) {
			cur = cur->next;
		}
		cur->next = new;
	}

	/* initialize region to passed values */
	new->id   = id;
	new->mask = align;
	new->base = new->limit = base;
	new->size = 0;
	new->next = NULL;
	new->name = (char *)name;
}

/*
 * allocate new block in region
 * return starting base address
 */
uint32
region_allocate(uint8 id, uint32 size)
{
	struct region_info *region = region_find(id);

	if (!region) return 0;
	if (!size) return region->size;

	/* round up size */
	roundup2(&size);

	/* minimum alignment is either the region-specific value
	 * or the size of the requested area, whichever is larger.
	 * alignment mask is the logical OR of both values 
	 */
	region->base &= ~((size-1) | region->mask);
	region->base -= size;
	region->size += size;

	return region->base;
}

uint32
region_align(uint8 id, uint32 mask)
{
	struct region_info *region = region_find(id);
	region->base &= ~mask;
	return region->base;
}

/*
 * round integer up to nearest aligned value
 *
 * inputs: pointer to unsigned integer
 * return: nothing
 *
 * !!! MODIFIES INPUT VARIABLE !!!
 */
static void
roundup2(uint32 *num)
{
	uint32 aligned = 0;
	uint32 shifts = 0;
	uint32 n = *num;

	while (n) {
		if ((n & 1) && (n > 1)) {
			aligned++;
		}
		n >>= 1;
		shifts++;
	}

	if (aligned) {
		*num = 1 << shifts;
	}
}

const char *
region_get_name(uint8 id)
{
	struct region_info *r = region_find(id);
	return (r) ? (const char *)r->name : NULL;
}

uint32
region_get_size(uint8 id)
{
	struct region_info *r = region_find(id);
	return (r) ? r->size : 0;
}

uint32
region_get_base(uint8 id)
{
	struct region_info *r = region_find(id);
	return (r) ? r->base : 0;
}

uint32
region_get_limit(uint8 id)
{
	struct region_info *r = region_find(id);
	return (r) ? r->limit : 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
