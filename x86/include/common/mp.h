/*
 * $Id: mp.h,v 1.1.1.1 2003/06/10 22:42:02 iceblink Exp $
 * Copyright (c) 1999,2000 Cobalt Networks, Inc.
 */

#ifndef COMMON_MP_H
#define COMMON_MP_H

#define TRAMPOLINE_MEM		0x9000
#define TRAMPOLINE_BASE		0xa000
#define TRAMPOLINE_GDT_LIMIT	0x2000
#define CPU_COUNTER_AP		0xa008
#define CPU_COUNTER_BSP		0x70008
#define CPU_COUNTER		0x7008

int mp_init(void);
void mp_list(void);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
