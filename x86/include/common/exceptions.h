/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: exceptions.h,v 1.1.1.1 2003/06/10 22:42:01 iceblink Exp $ */

/*
 * exceptions.h
 */
#ifndef COMMON_EXCEPTIONS_H
#define COMMON_EXCEPTIONS_H

void init_exceptions( void );
void lidt( void * base, unsigned int limit );
void set_exception( int num, void *addr );

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
