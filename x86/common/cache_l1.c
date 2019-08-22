/*
 * $Id: cache_l1.c,v 1.1.1.1 2003/06/10 22:41:40 iceblink Exp $
 * Copyright (c) 2001 Sun Microsystem, Inc
 * All Rights Reserved
 *
 * L1 routines sperated for inclusion in ramcode
 */

#include <printf.h>
#include <string.h>
#include <stdlib.h>

#include "common/cache.h"
#include "common/pci.h"
#include "common/systype.h"
#include "common/memory.h"
#include "common/delay.h"
#include "common/msr.h"
#include "common/rom.h"
#include "common/cmos.h"

static int l1_is = 0;
int l2_is = 0;
static void l1_on(void);
#if 0
static void l1_off(void);
#endif
void 
cache_init(int in_monitor)
{
    if( (in_monitor && cmos_read_flag(CMOS_DELAY_CACHE_FLAG)) ||
	(!in_monitor) )
    {
	    /* FIXME: there will one day be more than just 5k/3k */
	if (systype_is_5k()) 
	{
		/* disable L2 */
	    msr_set_off(MSR_BBL_CR_CTL3, (1 << 8));
	    
		/* disable fixed & variable MTRRs */
	    msr_set_off(MSR_MTRRdefType, MSR_MTRRfixEnable | MSR_MTRRvarEnable);
	}
    }
    else
    {
	l1_is = 1;
	if( systype_is_5k() )
	{
	    l2_is = 1;
	}
    }
}
 
void 
cache_l1_on(void)
{
	if (!l1_is)
		l1_on();
}
#if 0
void 
cache_l1_off(void)
{
	if (l1_is)
		l1_off();
}
#endif
int 
cache_l1_status(void)
{
	return l1_is;
}


/* l1_on()
 *
 *   enable the L1 cache
 *
 *   inputs: nothing
 *   return: nothing
 */
static void 
l1_on(void)
{
	if (l1_is)
		return;


	/* 
	 * turn off bits 30,29 of %cr0:
	 * 30 = Cache Disable (CD), 29 = Not Writethrough (NW)
	 */
	__asm__ __volatile__("invd\n"
			     "movl %%cr0, %%eax \n"
			     "andl $0x9fffffff, %%eax \n"
			     "movl %%eax, %%cr0 \n"
			     "jmp 1f \n"
			     "nop \n"
			     "1: invd \n"
			     :
			     :
			     :"eax", "memory");

	if (systype_is_3k()) {
		unsigned char val;
		/* set bit 0 of reg 0x40 on North Bridge - L1 Cache ON/OFF */
		val = pci_read_config_byte(0x0, PCI_DEVFN(0,0), 0x40);
		pci_write_config_byte(0x0, PCI_DEVFN(0,0), 0x40, val | 0x1);
	}

	/* don't do it again */
	l1_is = 1;


	/* reinit delay stuff */
	delay_init();
	delay_time_trials();

}

#define MTRRphysType_UC		0x0
#define MTRRphysType_WC		0x1
#define MTRRphysType_WT		0x4
#define MTRRphysType_WP		0x5
#define MTRRphysType_WB		0x6
#define MTRRfixType_WP		0x0505050505050505
#define MTRRfixType_WB		0x0606060606060606
#define MTRRphysRange		0x0000000ffffff000	/* mask bits 12-35 */
#define MTRRtopBase		0xfc000000UL
#define MTRRtopSize		0x04000000UL	/* 64 MB */


void 
mtrr_init(void)
{
	unsigned long long msr;
	unsigned int mtrr_vmax, mtrr_num, lo, hi;
	unsigned int ram_size=mem_size, ram_base=0, bit;

	/**
	 * program the MTRRs
	 **/

	if( !systype_is_5k() )
	    return;
	
	/* first look at the MTRR capability lists */
	msr = msr_get(MSR_MTRR_CAP);
	if (!(msr & 0x100)) {
		return;
	}

	/* disable fixed & variable MTRRs */
	msr_set_off(MSR_MTRRdefType, MSR_MTRRfixEnable | MSR_MTRRvarEnable);

	/* number of variable MTRRs supported */
	mtrr_vmax = (int)(msr & 0xff);

	/* zero all MTRR register fields
	 * (see IA32SD, vol 3, section 9.11.5)
	 */
	msr_set(MSR_MTRRfix64K_00000, 0);
	msr_set(MSR_MTRRfix16K_80000, 0);
	msr_set(MSR_MTRRfix16K_A0000, 0);
	msr_set(MSR_MTRRfix4K_C0000,  0);
	msr_set(MSR_MTRRfix4K_C8000,  0);
	msr_set(MSR_MTRRfix4K_D0000,  0);
	msr_set(MSR_MTRRfix4K_D8000,  0);
	msr_set(MSR_MTRRfix4K_E0000,  0);
	msr_set(MSR_MTRRfix4K_E8000,  0);
	msr_set(MSR_MTRRfix4K_F0000,  0);
	msr_set(MSR_MTRRfix4K_F8000,  0);

	/* set all variable MTRRs to zero */
	for (mtrr_num=0; mtrr_num<mtrr_vmax; mtrr_num++) {
		msr_set(MSR_MTRRphysBase(mtrr_num), 0);
		msr_set(MSR_MTRRphysMask(mtrr_num), 0);
	}

	/* reset max MTRR count because we must leave 2 for OS */	
	mtrr_vmax -= 2;

	/* set default type to uncachable */
	msr_set(MSR_MTRRdefType, MTRRphysType_UC);

	/* set Fixed MTRR register values */
	msr_set(MSR_MTRRfix64K_00000, MTRRfixType_WB);	/*   0-512K  -> writeback */
	msr_set(MSR_MTRRfix16K_80000, MTRRfixType_WB);	/* 512-640K  -> writeback */
	msr_set(MSR_MTRRfix4K_F0000,  MTRRfixType_WP);  /* 960-992K  -> write-protected */
	msr_set(MSR_MTRRfix4K_F8000,  MTRRfixType_WP);  /* 992-1024K -> write-protected */

	/* setup Variable MTRR registers
	 *
	 * 1. find largest power of 2 <= ramsize
	 * 2. set mtrr to this value
	 * 3. repeat for remaining ramsize
	 * 4. stop when ramsize <4MB (no sense using MTRRs that small)
	 */
	for (mtrr_num=0, bit=31; 
	    (ram_size>0x3ffff) && (bit>21) && (mtrr_num<mtrr_vmax); bit--) {
		unsigned int size = (1 << bit);

		if (!(ram_size & size))
			continue;

		/* set the base and type (writeback) */
		msr_set(MSR_MTRRphysBase(mtrr_num),
			MTRRphysType_WB | (ram_base & MTRRphysRange));

		/* set the size and enable bit */
		lo = (~(size - 1) & MTRRphysRange) | MSR_MTRRvarEnable;
		hi = (~((size >> 4) - 1) >> 28) & 0xf;
		wrmsr(MSR_MTRRphysMask(mtrr_num), lo, hi);

		/* DPRINTF("MTRR: %dM - %dM=write-back", 
		 *	(ram_base>>20), ((ram_base+size)>>20)); 
		 */

		/* move base pointer */
		ram_base += size;

		/* select next variable mtrr msr */
		mtrr_num++;

		/* subtract size from total */
		ram_size &= ~size;
	}

	/* reserve last 64MB of first 4GB as uncachable */
	lo = MTRRtopBase | MTRRphysType_UC;
	wrmsr(MSR_MTRRphysBase(mtrr_num), lo, 0);
	lo = (~(MTRRtopSize - 1) & 0xfffff000) | MSR_MTRRvarEnable;
	hi = (~((MTRRtopSize >> 4) - 1) >> 28) & 0xf;
	wrmsr(MSR_MTRRphysMask(mtrr_num), lo, hi);

	/* enable Fixed & Variable MTRRs */
	msr_set_on(MSR_MTRRdefType, MSR_MTRRfixEnable | MSR_MTRRvarEnable);
}
#if 0
static void 
l1_off(void)
{
	/* 
	 * turn on bits 30,29 of %cr0:
	 * 30 = Cache Disable (CD), 29 = Not Writethrough (NW)
	 */
	__asm__ __volatile__("jmp 1f \n"
			     "nop \n"
			     "1: invd \n"
			     "movl %%cr0, %%eax \n"
			     "orl $0x60000000, %%eax \n"
			     "movl %%eax, %%cr0 \n"
			     "jmp 1f \n"
			     "nop \n"
			     "1: wbinvd \n"
			     :
			     :
			     :"eax", "memory");


	if (systype_is_3k()) {
		unsigned char val;

		/* unset bit 0 of reg 0x40 on North Bridge - L1 Cache ON/OFF */
		val = pci_read_config_byte(0x0, PCI_DEVFN(0,0), 0x40);
		pci_write_config_byte(0x0, PCI_DEVFN(0,0), 0x40, val & (~0x1));
	}

	l1_is = 0;

	/* reinit delay stuff */
	delay_init();
	delay_time_trials();
}
#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
