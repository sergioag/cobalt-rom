/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: elf.c,v 1.1.1.1 2003/06/10 22:41:45 iceblink Exp $
 */


#include <stdlib.h>

#include "linux/elf.h"

#include "common/elf.h"

#define MY_ELF_CLASS 1 /* 32 bit objects */
#define MY_ELF_ENDIAN 1 /* little endian */
#define MY_ELF_MACHINE 3 /* Intel x86 */

int elf_valid( void * elf_obj )
{
    Elf32_Ehdr *eh;
    eh = (Elf32_Ehdr *) elf_obj;
    
    if ( (eh->e_ident[EI_MAG0] != 0x7f) ||
	 (eh->e_ident[EI_MAG1] != 'E') ||
	 (eh->e_ident[EI_MAG2] != 'L') ||
	 (eh->e_ident[EI_MAG3] != 'F') )
    {
	return 0;
    }
    
    if( eh->e_ident[EI_CLASS] != MY_ELF_CLASS )
	return 0;
    
    if( eh->e_ident[EI_DATA] != MY_ELF_ENDIAN )
	return 0;
    
    if( eh->e_type != 2 ) /* executable */
	return 0;
    
    if( eh->e_machine != MY_ELF_MACHINE ) 
	return 0;
    
    return 1;
   
}

int elf_relocate( void * from, void * to, int max_size )
{
    int i;
    Elf32_Ehdr *eh;
    Elf32_Phdr *ph;
    unsigned int ph_loc;

    if( ! elf_valid( from ) )
	return -1;
    
    eh = (Elf32_Ehdr *) from;
    
    ph_loc = ((unsigned int ) from) + eh->e_phoff;
    
    for( i=0 ; i<eh->e_phnum ; i++ )
    {
	ph = (Elf32_Phdr *) (ph_loc + (i*eh->e_phentsize));
	
	    /* check to make sure segment is within bounds */
	if( ph->p_vaddr < (unsigned int )to )
	    return -1;
	if( (ph->p_vaddr + ph->p_memsz) > 
	    ( ((unsigned int )to) + max_size ))
	    return -1;
	
	    /* relocate segment */
	memcpy( (unsigned char *) ph->p_vaddr, 
		((unsigned char *) from) + ph->p_offset,
		ph->p_filesz );

	    /* zero bss */
/*	memset( ((unsigned char *) ph->p_vaddr) + ph->p_filesz, 0x0,
	ph->p_memsz - ph->p_filesz ); */
    }

    return 0;
}

unsigned int elf_entry( void * elf_obj )
{
    Elf32_Ehdr *eh;
    
    if( ! elf_valid( elf_obj ) )
	return 0;

    eh = (Elf32_Ehdr *) elf_obj;
    return eh->e_entry;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
