/*
 * $Id: cache.c,v 1.1.1.1 2003/06/10 22:41:40 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 *
 * this controls L1/L2 cache
 *
 * Erik Gilling
 * Duncan Laurie
 */

#include <printf.h>
#include <string.h>
#include <stdlib.h>

#include "common/cache.h"
#include "common/pci.h"
#include "common/cmos.h"
#include "common/systype.h"
#include "common/memory.h"
#include "common/delay.h"
#include "common/msr.h"
#include "common/rom.h"

/* set from set_cache_func */
static int l1_should_be = 1;
static int l2_should_be = 1;
extern int l2_is;
static void l2_on(void);

void 
cache_set_caches(int l1, int l2)
{
	l1_should_be = l1;
	l2_should_be = l2;
}

void 
cache_l2_on(void)
{
	if (!l2_is)
		l2_on();
}

int
cache_l2_status(void)
{
	return l2_is;
}

int
cache_l1_should_be(void)
{
	return l1_should_be;
}

int 
cache_l2_should_be(void)
{
	return l2_should_be;
}

/* enable_caches()
 *
 *   set caches to default states
 *
 *   inputs: nothing
 *   return: nothing
 */
void 
enable_caches(void)
{
	if (l1_should_be)
	    cache_l1_on();

	if (l2_should_be)
	    cache_l2_on();
}

/* l2_on()
 *
 *   enable the L2 cache
 *
 *   inputs: nothing
 *   return: nothing
 */
static void 
l2_on(void)
{
	if (l2_is)
		return;
	cache_l1_on();

	DPRINTF("Enabling L2 cache: ");

	switch (systype())
	{
	case SYSTYPE_3K: {
		pci_dev *north = NULL;
		unsigned char cache_sz, val;

		/* The K6-2+ has internal L2 cache, therefore we do not
		 * add any external L2 cache (we could--it would be L3)
		 *
		 * So we need to stop here if on-chip L2 was found because
		 * attempting to enable non-existant external L2 will hang */
		cache_sz = systype_l2_cache_size(0);
		if (cache_sz > 0) {
			DPRINTF("on-chip L2 is %dK -", cache_sz);
			break;
		}
		
		/* l2 should be off right now */
		__asm__ __volatile__("wbinvd":::"memory");

		/* set bit 0 of reg 0x42 on North Bridge - L2 Cache ON/OFF */
		north = pci_lookup_dev(PCI_VENDOR_AL, 
			PCI_DEVICE_AL_M1541, NULL);
		val = pci_dev_read_config_byte(north, 0x42);
		cache_sz = (pci_dev_read_config_byte(north, 0x41) & 0xc) >> 2;
		if (!cache_sz)
			break;	/* failsafe if cache is missing */

		pci_dev_write_config_byte(north, 0x42, val | 0x15);

		/* prime L2 */
		__asm__ __volatile__(
			"cld \n" 
			"movl $0x100000, %%esi \n"
			"movl $0x100000, %%edi \n" 
			"movl %0, %%ecx \n" 
			"wbinvd \n" 
			"es \n"
			/* "addr32 \n" */
			"rep \n" "lodsl"
			:: "r"(0x40000 << cache_sz)
			: "eax", "ecx", "esi", "edi", "memory"
		);

		/* turn off initialization bits */
		val = pci_dev_read_config_byte(north, 0x42);
		/* 1110 1011 */
		pci_dev_write_config_byte(north, 0x42, val & 0xeb);
		break;
	}
	case SYSTYPE_5K:
		/* enable L2 */	
		msr_set_on(MSR_BBL_CR_CTL3, (1 << 8));
		break;

	default:
		DPRINTF("unknown system type -");
	}

	/* don't do it again */
	l2_is = 1;

	/* reinit delay stuff */
	delay_init();
	delay_time_trials();

	DPRINTF("done\n");
}

#if 0

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
#endif
#if 0
/*
 * Control Register Read
 *
 * cache_p6_cmd_cr   
 *   reg - register to read
 *
 * reads the reg from the TAGRam configuration registers
 */
unsigned char 
cache_p6_cmd_cr(int reg)
{
	unsigned long long msr;

	msr_set(MSR_BBL_CR_CTL, 0x2 | ((reg & 0x3f) << 0x5));
	msr_set(MSR_BBL_CR_TRIG, 0);

	while (msr_get(MSR_BBL_CR_BUSY) & 0x1) ;

	msr = msr_get(MSR_BBL_CR_CTL);
	return (unsigned char) (((unsigned long long) (msr >> 21)) & 0xff);
}

/*
 * Control Register Write
 *
 * cache_p6_cmd_cw
 *   reg - register to write to
 *   data - data to write to register
 *
 * writes data into reg on the TAGRam configuration registers
 */
void 
cache_p6_cmd_cw(int reg, unsigned char data)
{
	msr_set(MSR_BBL_CR_CTL, 0x3 | ((reg & 0x3f) << 0x5) | (data << 21));
	msr_set(MSR_BBL_CR_TRIG, 0);

	while (msr_get(MSR_BBL_CR_BUSY) & 0x1) ;
}

/*
 * Tag Inquite
 *
 * cache_p6_cmd_ti
 *   addr - address to which the tag corresponds
 *   hit - to be filled with tag hit/miss data
 *   state - to be filled with tag state data
 *   way - to be filled with tag way data
 *
 * does a tag inquire on addr and fills hit, state and way 
 */
void 
cache_p6_cmd_ti(unsigned int addr, int *hit, int *state, int *way);

/*
 * Data Read with LRU Update
 *
 * cache_p6_cmd_rlu
 *   addr - address of data to read
 *   hit - to be filled with tag hit/miss data
 *   state - to be filled with tag state data
 *   way - to be filled with tag way data
 *   line - to be filled with he cache line
 *   ecc - to be filled with ecc data
 *
 * does a data read with lru update and fills hit, state, way
 * line, and ecc.
 */
void 
cache_p6_cmd_rlu(unsigned int addr, int *hit, int *state, int *way,
		      unsigned char *line, unsigned char *ecc);

/*
 * Tag Read with Data Read
 *
 * cache_p6_cmd_trr
 *   tag_addr - address to which the tag corresponds
 *   way - way of the tag
 *   data_addr - to be filled with the victum address
 *   line - to be filled with he cache line
 *   ecc - to be filled with ecc data
 *
 * finds the tag at tag_addr and way and fills data_addr, line, and ecc.
 */
void 
cache_p6_cmd_trr(unsigned int tag_addr, int way, unsigned int *data_addr,
		      unsigned char *line, unsigned int *ecc);

/*
 * Tag Write with Data Read
 *
 * cache_p6_cmd_twr
 *   addr - address of tag
 *   hit - tag hit/miss data
 *   state - tag state data
 *   way - tag way data
 *   line - to be filled with he cache line
 *   ecc - to be filled with ecc data
 *
 * sets the tag at addr with hit, state, and way and reads the corresponding 
 * cache line into line and ecc.
 */
void 
cache_p6_cmd_twr(unsigned int addr, int way, int state,
		      unsigned char *line, unsigned int *ecc);

/*
 * Tag Write
 *
 * cache_p6_cmd_tw
 *   addr - address of tag
 *   hit - tag hit/miss data
 *   state - tag state data
 *   way - tag way data
 *
 * sets the tag at addr with hit, state, and way.
 */
void 
cache_p6_cmd_tw(unsigned int addr, int way, int state);

/*
 * Tag Write with Data Read
 *
 * cache_p6_cmd_twr
 *   addr - address of tag
 *   hit - tag hit/miss data
 *   state - tag state data
 *   way - tag way data
 *   line - cache line
 *   ecc - ecc data
 *
 * sets the tag at addr with hit, state, and way and the corresponding 
 * cache line into line and ecc.
 */
void 
cache_p6_cmd_tww(unsigned int addr, int way, int state,
		      unsigned char *line, unsigned int *ecc);


#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
