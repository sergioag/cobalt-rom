/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* 
 * $Id: elf.h,v 1.1.1.1 2003/06/10 22:42:01 iceblink Exp $
 */

#ifndef COMMON_ELF_H
#define COMMON_ELF_H

int elf_valid( void * elf_obj );
int elf_relocate( void * from, void * to, int max_size );
unsigned int elf_entry( void * elf_obj );

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
