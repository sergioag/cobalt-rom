/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: regs.h,v 1.1.1.1 2003/06/10 22:42:03 iceblink Exp $
 */

#ifndef COMMON_REGS_H
#define COMMON_REGS_H

struct regs {
   unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
};

#define regs_print() asm("pusha\n\t" "call _regs_print\n\t" "popa" : :)

void _regs_print(struct regs r);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
