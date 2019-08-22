/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: regs.c,v 1.1.1.1 2003/06/10 22:41:53 iceblink Exp $
 */

#include <printf.h>
#include "common/regs.h"

void _regs_print(struct regs r)
{
    unsigned int cs, ds, es, fs, gs, ss;
    unsigned int cr0=0L, cr2=0L, cr3=0L, cr4=0L;

    __asm__("mov %%cs, %0" : "=r" (cs):);
    __asm__("mov %%ds, %0" : "=r" (ds):);
    __asm__("mov %%es, %0" : "=r" (es):);
    __asm__("mov %%fs, %0" : "=r" (fs):);
    __asm__("mov %%gs, %0" : "=r" (gs):);
    __asm__("mov %%ss, %0" : "=r" (ss):);

    printf("      eax: %08x  ebx: %08x  ecx: %08x  edx: %08x\r\n",
	   r.eax, r.ebx, r.ecx, r.edx);

    printf("      edi: %08x  esi: %08x  ebp: %08x  esp: %08x\r\n",
	   r.edi, r.esi, r.ebp, r.esp);

    __asm__("movl %%cr0, %0" : "=r" (cr0):);
    __asm__("movl %%cr2, %0" : "=r" (cr2):);
    __asm__("movl %%cr3, %0" : "=r" (cr3):);
    __asm__("movl %%cr4, %0" : "=r" (cr4):);

    printf("      cr0: %08x  cr2: %08x  cr3: %08x  cr4: %08x\r\n",
	   cr0, cr2, cr3, cr4);

    printf("      cs: %04x  ds: %04x  es: %04x  fs: %04x  gs: %04x  ss: %04x\r\n",
	   cs, ds, es, fs, gs, ss);

}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
