/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: systype_cpu.c,v 1.1.1.1 2003/06/10 22:41:54 iceblink Exp $
 *
 * allows one to distinguish between the different systems types
 *
 */

#include <string.h>
#include <stdlib.h>
#include <printf.h>
#include <io.h>

#include "common/systype.h"
#include "common/pci.h"
#include "common/msr.h"
#include "common/apic.h"
#include "common/cache.h"
#include "common/x86.h"
#include "common/delay.h"
#include "common/rom.h"
#include "common/cmos.h"

#define SYSINFO(cpu)		(systype_get_info(cpu))
#define SYSINFO_ID(lapic_id)	(systype_get_info_ID(lapic_id))
#define CACHE_DECODE(reg,mask)	((unsigned char)(((reg) >> (mask * 8)) & 0xff))

__u8 global_cpu_map;
static struct cpu_info cpu_list;

static void systype_init_proc_intel(struct cpu_info *);
static void systype_init_proc_amd(struct cpu_info *);
static char *lookup_cache_mask(unsigned char, struct cpu_info *);

struct cpu_info *
systype_get_info(int n)
{
	struct cpu_info *info = &cpu_list;
	int i = 0;

	while (i++<n && info->NEXT)
		info=info->NEXT;

	return info;
}

struct cpu_info *
systype_get_info_ID(int ID)
{
	struct cpu_info *info = &cpu_list;

	while (info->apic_id != ID) {
		if (info->NEXT) 
			info = info->NEXT;
		else
			return NULL;
	}
	return info;
}

const char *
systype_cpu_vendor(int cpu)
{
	return SYSINFO(cpu)->vendor_string;
}

int 
systype_cpu_model(int cpu)
{
	return SYSINFO(cpu)->model;
}

int 
systype_cpu_family(int cpu)
{
	return SYSINFO(cpu)->family;
}

int 
systype_cpu_type(int cpu)
{
	return SYSINFO(cpu)->type;
}

int 
systype_cpu_stepping(int cpu)
{
	return SYSINFO(cpu)->stepping;
}

unsigned int
systype_cpu_clock(int cpu)
{
	return SYSINFO(cpu)->cpu_clock;
}

unsigned int
systype_host_bus_clock(int cpu)
{
	return SYSINFO(cpu)->host_bus_clock;
}

byte
systype_clock_ratio(int cpu)
{
	return SYSINFO(cpu)->clock_ratio;
}

int
systype_l2_cache_size(int cpu)
{
	return SYSINFO(cpu)->l2_cache;
}

void
systype_init_bsp(void)
{
	cpu_list.NEXT = NULL;

	if (systype_is_3k()) {
		systype_init_proc_amd(&cpu_list);
	}
	else if (systype_is_5k()) {
		systype_init_proc_intel(&cpu_list);
	}
	else return;

	global_cpu_map |= (1 << cpu_list.apic_id);
}

void 
systype_init_ap(void)
{
	struct cpu_info *info = &cpu_list;

	while (info->NEXT)
		info = info->NEXT;

	info->NEXT = (struct cpu_info *) malloc(sizeof(struct cpu_info));
	memset(info->NEXT, 0, sizeof(struct cpu_info));
	info = info->NEXT;
	info->NEXT = NULL;

	systype_init_proc_intel(info);
	global_cpu_map |= (1 << info->apic_id);
}

void
systype_init_ap_ID(int ID)
{
	struct cpu_info *info = &cpu_list;

	while (info->NEXT)
		info = info->NEXT;

	info->NEXT = (struct cpu_info *) malloc(sizeof(struct cpu_info));
	memset(info->NEXT, 0, sizeof(struct cpu_info));
	info = info->NEXT;
	info->NEXT = NULL;

	systype_init_proc_intel(info);

	info->bsp = 0;
	info->apic_id = ID;
	global_cpu_map |= (1 << ID);
}

static void
systype_init_proc_amd(struct cpu_info *info)
{
	unsigned int top, a, b, c, d;

	if (!systype_is_3k())
		return;

	/* save CPU speed */
	info->cpu_clock = ticks_per_us;

	/* front-side-bus runs at 100MHz */
	info->host_bus_clock = 100;
	info->clock_ratio = (dword)((info->cpu_clock*2) /info->host_bus_clock);
	if ((int)(info->cpu_clock * 2 % info->host_bus_clock) >
	   (int)(info->host_bus_clock / 3)) {
		info->clock_ratio++;
	}

	/* fetch vendor string */
	cpuid(0, &a, &b, &c, &d);
	memcpy(info->vendor_string,     &b, 4);
	memcpy(info->vendor_string + 4, &d, 4);
	memcpy(info->vendor_string + 8, &c, 4);
	info->vendor_string[13] = '\0';

	/* fetch feature mask */
	cpuid(1, &a, &b, &c, &d);
	info->feature_mask = d;

	/* AMD extended function 8000_0000h
	 * - EAX has highest extended function supported */
	cpuid(0x80000000, &top, &b, &c, &d);

	/* AMD extended function 8000_0001h
	 * - EAX has processor signature
	 * - EDX has extended features flags */
	if (top >= 1) {
		cpuid(0x80000001, &a, &b, &c, &d);
		info->stepping = a & 0xf;
		info->model    = (a >> 4) & 0xf;
		info->family   = (a >> 8) & 0xf;
	}

	/* AMD extended function 8000_0006h
	 * - ECX has L2 cache size in bits 16-31 */
	if (top >= 6) {
		cpuid(0x80000006, &a, &b, &c, &d);
		info->l2_cache = (c >> 16) & 0xffff;
	}

	/* does not have local apic */
	info->apic_id = info->apic_version = 0;

	/* does not support SMP (i.e. always BSP) */
	info->bsp = 1;
}

static byte intel_cpu_clock_ratio[16] = {
	10,	/* 0000  x5	*/
	6,	/* 0001  x3	*/
	8,	/* 0010  x4	*/
	4,	/* 0011  x2	*/
	11,	/* 0100  x5.5	*/
	7,	/* 0101  x3.5	*/
	17,	/* 0110  x4.5/8.5x with coppermine PIII */
	5,	/* 0111  x2.5	*/
	7,	/*   RESERVED   */
	14,	/* 1001  x7	*/
	16,	/* 1010  x8	*/
	20,	/* 1011  x6/10x with coppermine PIII */
	4,	/* 1100  x2	*/
	15,	/* 1101  x7.5	*/
	3,	/* 1110  x1.5	*/
	13	/* 1111  x6.5	*/
};

static void 
systype_init_proc_intel(struct cpu_info *info)
{
	unsigned int top, eax, ebx, ecx, edx;
	dword msrval;

	if (!systype_is_5k())
		return;

	/* save CPU speed */
	info->cpu_clock = ticks_per_us;

	/* check for invalid system bus frequency */
	msrval = (dword) msr_get(MSR_EBL_CR_POWERON);
	if ((msrval & (1<<11)) == (1<<11)) {
		printf("Warning! CPU System Bus Frequency mismatch!\n");
	}
	    
	/* determine host bus clock speed */
	if (boardtype() == BOARDTYPE_MONTEREY) {
		info->clock_ratio = intel_cpu_clock_ratio[(msrval>>22) & 0xf];
		info->host_bus_clock = (dword)(info->cpu_clock * 2 / info->clock_ratio);
	} else if (boardtype() == BOARDTYPE_ALPINE) {
		info->clock_ratio = inb(0x603) & (1<<1); // read GPIO11
		info->host_bus_clock = info->clock_ratio ? 133 : 100;
		info->clock_ratio = (dword)(info->cpu_clock * 2 / info->host_bus_clock);
		if ((int)(info->cpu_clock * 2 % info->host_bus_clock) >
		    (int)(info->host_bus_clock / 3)) {
			info->clock_ratio++;
		}
	}

	cpuid(0, &eax, &ebx, &ecx, &edx);

	top = eax;
	memcpy(info->vendor_string,     &ebx, 4);
	memcpy(info->vendor_string + 4, &edx, 4);
	memcpy(info->vendor_string + 8, &ecx, 4);

	info->vendor_string[13] = '\0';

	if (top >= 1) {
		cpuid(1, &eax, &ebx, &ecx, &edx);

		info->feature_mask = edx;
		info->stepping     = eax & 0xf;
		info->model        = (eax >> 4) & 0xf;
		info->family       = (eax >> 8) & 0xf;
		info->type         = (eax >> 12) & 0x3;
	}

	if (top >= 2) {
		int i=0, max=0, j;

		/* initialize to zero */
		info->l2_cache = 0;

		/* need to enable L2 cache in MSR */
		if( cmos_read_flag(CMOS_DELAY_CACHE_FLAG) )
			msr_set_on(MSR_BBL_CR_CTL3, (1<<8));

		do {
			char *p;
			cpuid(2, &eax, &ebx, &ecx, &edx);
			if (!max)
				max = eax & 0xff;

			/* if bit 31=0 then remainder of register is valid */
			if ((eax >> 31) != 0x1) {
				/* only care about highest 3 bytes of eax */
				for (j=3; j>0; j--) {
					p = lookup_cache_mask(CACHE_DECODE(eax,j), info);
					if (p) DPRINTF("L2 Cache: %s\n", p);
				}
			}
			/* if bit 31=0 then remainder of register is valid */
			if ((edx >> 31) != 0x1) {
				/* examine all 4 bytes of edx */
				for (j=3; j>=0; j--) {
					p = lookup_cache_mask(CACHE_DECODE(edx,j), info);
					if (p) DPRINTF("L2 Cache: %s\n", p);
				}
			}
		} while(++i < max);

		/* disable L2 cache setting */
		if( cmos_read_flag(CMOS_DELAY_CACHE_FLAG) )
			msr_set_off(MSR_BBL_CR_CTL3, (1<<8));
	}

	info->apic_id = lapic_get_id();
	info->apic_version = lapic_get_version();
	info->bsp = lapic_get_bsp();
}

struct intel_cache_entry {
	unsigned char key;
	unsigned int size;
	char *desc;
};

static struct intel_cache_entry intel_cache_table[] = {
	{ 0x40,	0,	"None"		},
	{ 0x41, 128,	"128K, 4-way"	},
	{ 0x42, 256,	"256K, 4-way"	},
	{ 0x43, 512,	"512K, 4-way"	},
	{ 0x44, 1024,	"1M, 4-way"	},
	{ 0x45, 2048,	"2M, 4-way"	},
	{ 0x82, 256,	"256K, 8-way"	},
	{ 0x84, 1024,	"1M, 8-way"	},
	{ 0x85, 2048,	"2M, 8-way"	},
	{ 0x00, 0,	NULL		}
};
	
static char *
lookup_cache_mask(unsigned char mask, struct cpu_info *info)
{
	int i = 0;

	while (intel_cache_table[i].key && intel_cache_table[i].key != mask)
		i++;

	info->l2_cache = intel_cache_table[i].size;
	return intel_cache_table[i].desc;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
