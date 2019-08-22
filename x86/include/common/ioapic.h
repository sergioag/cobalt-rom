/* $Id: ioapic.h,v 1.1.1.1 2003/06/10 22:42:02 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 */

#ifndef COMMMON_IOAPIC_H
#define COMMMON_IOAPIC_H

#include <extra_types.h>

#define IOAPIC_BASE		0xFEC00000
#define IOAPIC_ID		0x00
#define IOAPIC_VERSION		0x01
#define IOAPIC_ARB		0x02
#define IOAPIC_INITIAL_ID	4
#define IOAPIC_NUM		2

enum IO_APIC_entry_type {
	IO_APIC_type__Fixed 	= 0,	/* 000 */
	IO_APIC_type__LowPri	= 1,	/* 001 */
	IO_APIC_type__SMI 	= 2,	/* 010 */
	IO_APIC_type__reserved_1= 3,	/* 011 */
	IO_APIC_type__NMI 	= 4,	/* 100 */
	IO_APIC_type__INIT 	= 5,	/* 101 */
	IO_APIC_type__reserved_2= 6,	/* 110 */
	IO_APIC_type__ExtINT 	= 7	/* 111 */
};

enum IO_APIC_entry_mode {
	IO_APIC_mode__Physical	= 0,
	IO_APIC_mode__Logical	= 1
};

enum IO_APIC_entry_trigger {
	IO_APIC_trigger__Edge	= 0,
	IO_APIC_trigger__Level	= 1
};

enum IO_APIC_entry_polarity {
	IO_APIC_polarity__Hi	= 0,
	IO_APIC_polarity__Lo	= 1
};

enum IO_APIC_entry_mask {
	IO_APIC_mask__On	= 0,
	IO_APIC_mask__Off	= 1
};

typedef struct IO_APIC_table_entry {
	__u32	vector		: 8,	/* interrupt vector */
		type		: 3,	/* delivery mode	enum <IO_APIC_entry_type> */
		mode		: 1,	/* destination mode	enum <IO_APIC_entry_mode> */
		status		: 1,	/* delivery status	0:idle, 1:pending */
		polarity	: 1,	/* interrupt polarity	enum <IO_APIC_entry_polarity> */
		irr		: 1,	/* remote irr		0:EOI_received, 1:accepted */
		trigger		: 1,	/* trigger mode		enum <IO_APIC_entry_trigger> */
		mask		: 1,	/* interrupt mask	enum <IO_APIC_entry_mask> */
		__reserved_2	: 15;	/* reserved		(set to zero) */
	union {
		struct {
			__u32	__reserved_1	: 24,
				apic_id		: 4,
				__reserved_2	: 4;
		} physical;

		struct {
			__u32	__reserved_1	: 24,
				cpu_map		: 8;
		} logical;
	} dest;
} __attribute__ ((packed)) IO_APIC_entry_t;

void ioapic_init(void);
void ioapic_setup(int);
__u32 ioapic_get_reg(int, __u32);
void ioapic_set_reg(int, __u32, __u32);
void ioapic_set_id(int, __u8);
__u8 ioapic_get_id(int);
__u8 ioapic_get_version(int);
__u8 ioapic_get_entries(int);
__u32 ioapic_get_base(int);
IO_APIC_entry_t * ioapic_get_redir(int, __u8);
void ioapic_set_redir(int, __u8, const IO_APIC_entry_t *);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
