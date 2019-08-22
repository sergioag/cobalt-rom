/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: alloc.h,v 1.1.1.1 2003/06/10 22:42:00 iceblink Exp $ */

/*
 * alloc.h
 */
#ifndef COMMON_ALLOC_H
#define COMMON_ALLOC_H

#include "common/memory.h"

void *realloc(void *ptr, size_t size);
void *calloc(size_t nelem, size_t size);
void free(void *ptr);
void *malloc(size_t size);
void *valloc(size_t size);

#endif

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
