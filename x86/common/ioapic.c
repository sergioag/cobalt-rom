/*
 * $Id: ioapic.c,v 1.1.1.1 2003/06/10 22:41:48 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 *
 * support for I/O APICs on MP systems
 *
 * Duncan Laurie
 */

#include <io.h>
#include <printf.h>

#include "common/ioapic.h"
#include "common/memory.h"
#include "common/alloc.h"
#include "common/rom.h"
#include "common/cmos.h"
#include "common/systype.h"

/********************************************************************/
#define ioapic_index(n,index)\
	((*((volatile __u32 *)(IOAPIC_BASE|((n)<<12))))=(index))

#define ioapic_set(n,data)\
	((*((volatile __u32 *)(IOAPIC_BASE|0x10|((n)<<12))))=(data))

#define ioapic_get(n)\
	((*((volatile __u32 *)(IOAPIC_BASE|0x10|((n)<<12)))))
/********************************************************************/

int ioapic_next_id = IOAPIC_INITIAL_ID;

void
ioapic_init (void)
{
	int j;

	if (!systype_is_5k())
		return;

	ioapic_next_id = IOAPIC_INITIAL_ID;

	for (j=0; j<IOAPIC_NUM; j++) {
		ioapic_setup(j);
	}
}

static void
ioapic_mask (int num, int intr)
{
	IO_APIC_entry_t *entry = ioapic_get_redir(num, intr);
	entry->mask = 1;
	ioapic_set_redir(num, intr, (const IO_APIC_entry_t *)entry);

	/* follow write with read to flush posted write buffers
	 * this forces INT line deasserted and stops spurious interrupts
	 */
	ioapic_get(num);
}

/*
 * setup I/O APIC corresponding to <num>
 *   set its APIC ID to <id>,
 *   zero all redirection registers
 */
void
ioapic_setup (int num)
{
	__u8 entries = ioapic_get_entries(num);
	IO_APIC_entry_t entry;
	int x;

	/* mask all entries */
	for (x=0; x<entries; x++) {
		ioapic_mask(num, x);
	}

	memset(&entry, 0, sizeof(IO_APIC_entry_t));
	entry.mask = 1;

	/* clear redirection registers */
	for (x=0; x<entries; x++) {
		ioapic_set_redir(num, x, (const IO_APIC_entry_t *)&entry);
	}

	/* set [unique] id */
	ioapic_set_id(num, ioapic_next_id++);
}

/*
 * get an I/O APIC register
 */
__u32
ioapic_get_reg (int num, __u32 index)
{
	ioapic_index(num, index);
	return ioapic_get(num);
}

/*
 * set an I/O APIC register
 */
void
ioapic_set_reg (int num, __u32 index, __u32 value)
{
	ioapic_index(num, index);
	ioapic_set(num, value);
}

/*
 * set the ID of I/O APIC
 */
void
ioapic_set_id (int num, __u8 id)
{
	__u32 reg;

	ioapic_index(num, IOAPIC_ID);

	reg = ioapic_get(num);
	reg &= ~(0xf << 24);
	reg |= ((id & 0xf) << 24);

	ioapic_set(num, reg);
}

/*
 * return ID of I/O APIC
 */
__u8
ioapic_get_id (int num)
{
	ioapic_index(num, IOAPIC_ID);
	return (__u8) ((ioapic_get(num) >> 24) & 0xf);
}

/*
 * return version of I/O APIC
 * (lower 16 bits of version register)
 */
__u8
ioapic_get_version (int num)
{
	ioapic_index(num, IOAPIC_VERSION);
	return (__u8) (ioapic_get(num) & 0xff);
}

/*
 * return number of entries in I/O APIC
 * (upper 16 bits of version register)
 */
__u8
ioapic_get_entries (int num)
{
	ioapic_index(num, IOAPIC_VERSION);
	return (__u8) (1 + ((ioapic_get(num)>>16) & 0xff));
}

/*
 * return base of I/O APIC
 */
__u32
ioapic_get_base (int num)
{
	return (__u32) (IOAPIC_BASE | (num << 12));
}

/*
 * lookup an interrupt input signal
 *
 *    num: I/O APIC number
 *   intr: interrupt input signal
 */
IO_APIC_entry_t *
ioapic_get_redir (int num, __u8 intr)
{
	static IO_APIC_entry_t entry;
	memset(&entry, 0, sizeof(IO_APIC_entry_t));
	
	ioapic_index(num, (intr<<1) + 0x10);
	*(((int *)&entry)+0) = ioapic_get(num);

	ioapic_index(num, (intr<<1) + 0x11);
	*(((int *)&entry)+1) = ioapic_get(num);

	return &entry;
}

/*
 * redirect an interrupt input signal
 *
 *     num: I/O APIC number
 *    intr: interrupt input signal
 *   redir: redirection table
 */
void
ioapic_set_redir (int num, __u8 intr, const IO_APIC_entry_t *entry)
{
	ioapic_index(num, (intr<<1) + 0x11);
	ioapic_set(num, *(((int *)entry)+1));
	ioapic_get(num);

	ioapic_index(num, (intr<<1) + 0x10);
	ioapic_set(num, *(((int *)entry)+0));
	ioapic_get(num);
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
