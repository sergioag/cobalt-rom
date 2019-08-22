/* $Id: mptable.h,v 1.1.1.1 2003/06/10 22:42:02 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 * Coypright (c) 2001 Sun Microsystems, Inc.
 *
 * Duncan Laurie <smp@sun.com>
 *
 * definitions for MP tables
 */

#ifndef COMMON_MPTABLE_H
#define COMMON_MPTABLE_H

#include <extra_types.h>
#include "common/systype.h"
#include "common/pci.h"

/* tables conform to Intel MP spec v1.4 */
#define MP_SPEC_REV		0x04

/* OEM identifier (8 bytes) */
#define MP_OEM_ID		"COBALT  "

/* memory location for tables */
#define MP_BASE_ADDRESS		0xF8200

/* table entry types */
enum mptable_entry_type {
	mp_entry_CPU		= 0,	/* Processor */
	mp_entry_BUS		= 1,	/* Bus */
	mp_entry_IOAPIC		= 2,	/* IO-APIC */
	mp_entry_IOINT		= 3,	/* I/O Interrupt */
	mp_entry_LocalINT	= 4,	/* Local Interrupt */
	mp_entry_SysAddrMap	= 128,	/* (ext) system address mapping */
	mp_entry_BusTree	= 129,	/* (ext) bus hierarchy */
	mp_entry_BusCompat	= 130	/* (ext) bus compatibility */
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   MP Floating Table
 *
 * contains a physical address pointer to the MP configuration table
 * and other MP feature information bytes.  its presence indicates to
 * the operating system that this system conforms to the MP specification
 * (byte 9 defines compliance with version 1.1 or 1.4)
 *
 * variable length data structure in word multiples.  only one 16-byte
 * struct is defined and it must be stored in at least one of the 
 * following locations:
 *   - first 1K of Extended BIOS Data Area (EBDA)
 *   - last 1K of base memory (639K-640K) (if EBDA segment undefined)
 *   - BIOS ROM address space between 000F0000h and 000FFFFFh
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#define MPTABLE_FLPOINTER_SIG		(('_'<<0)+('M'<<8)+('P'<<16)+('_'<<24))
typedef struct mptable_flpointer {
	__u32	signature;	/* ASCII: "_MP_" (5F 4D 50 5F) */
	__u32	address;	/* address of the MP config table */
	__u8	length;		/* length of table in words (0x01) */
	__u8	version;	/* version of MP spec (0x04=1.4) */
	__u8	checksum;	/* all bytes add to zero */
	struct {
		__u8	config;	/* zero means config table present,
				 * non-zero indicates default config */
		__u8	__reserved_1	: 6,
			mclocks		: 1, /* Multiple Clock Sources
					      * 0 = all CPUs share clock
					      * 1 = CPUs have different clocks */
			imcr		: 1; /* IMCR present
					      * 0: Virtual Wire mode
					      * 1: PIC mode */
		__u8	__reserved_2;
		__u8	__reserved_3;
		__u8	__reserved_4;
	} feature;
} __attribute__ ((packed)) mptable_flpointer_t;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   MP Configuration Table
 *
 * composed of a base section and an extended section.  the base
 * section contains entries that are completely backwards compatible
 * with MP spec 1.1, extended section contains entries newly defined
 * with the MP spec 1.4.
 *
 * contains explicit configuration information about APICs, processors,
 * buses, and interrupts.  consists of a header, followed by a number
 * of entries of various types.  the format and length of each entry 
 * depends on its type.
 *
 * this table must be stored either in non-repored system RAM or within
 * the BIOS read-only memory space.  suggested locations:
 *   - first 1K of EBDA
 *   - last 1K of system base memory (if EBDA segment undefined)
 *   - at the top of system physical memory
 *   - in the BIOS ROM space between 000E0000h and 000FFFFFh
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#define MPTABLE_CFHEADER_SIG	(('P'<<0)+('C'<<8)+('M'<<16)+('P'<<24))
typedef struct mptable_cfheader {
	__u32	signature;	/* ASCII: "PCMP" (50 43 4D 50) */
	__u16	length;		/* length of base config table in bytes */
	__u8	version;	/* version of MP spec (0x04=1.4) */
	__u8	checksum;	/* checksum of table, bytes add to zero */
	struct {
		__u8	oem[8];
		__u8	product[12];
	} __attribute__ ((packed)) id;
	struct {
		__u32	addr;	/* optional OEM-defined config table (0) */
		__u16	size;	/* size of base OEM table in bytes (0) */
	} __attribute__ ((packed)) oem;
	__u16	count;		/* entries in variable portion of table */
	__u32	apic;		/* base address of local apic */
	struct {
		__u16	length;
		__u8	checksum;
	} __attribute__ ((packed)) ext;
	__u8	__reserved_1;
} __attribute__ ((packed)) mptable_cfheader_t;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   CPU Entry
 *   [ 20 bytes ]
 *
 * filled in by the BIOS after it executes CPUID on each processor in
 * the system whenever possible the complete 32-bit CPU signature should
 * be filled in with values returned by the CPUID instruction.
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
typedef struct mptable_CPU {
	__u8	entry;
	struct {
		__u8	id;		/* local APIC ID of this processor */
		__u8	version;	/* bits 7:0 of local APIC version */
	} __attribute__ ((packed)) apic;
	__u8	enable		: 1,	/* enable flag */
		bsp		: 1,	/* bootstrap processor */
		__reserved_1	: 6;
	__u32	stepping	: 4,	/* CPU sig: stepping */
		model		: 4,	/* CPU sig: model */
		family		: 4,	/* CPU sig: family */
		__reserved_2	: 20;
	__u32	features;		/* features from CPUID */
	__u32	__reserved_3;
	__u32	__reserved_4;
} __attribute__ ((packed)) mptable_CPU_t;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   Bus Entry
 *   [ 8 bytes ]
 *
 * bus entries identify the kinds of buses in the system.  since there
 * can be more than 1 bus per system, each bus is assigned a unique
 * bus ID number by the bios.  the ID is used by the operation system
 * to associate interrupt lines with specific buses.
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#define MPTABLE_BUS_EISA	"EISA  "
#define MPTABLE_BUS_ISA		"ISA   "
#define MPTABLE_BUS_PCI		"PCI   "
typedef struct mptable_BUS {
	__u8	entry;
	__u8	id;		/* bus ID; unique from zero */
	__u8	type[6];	/* type of bus */
} __attribute__ ((packed)) mptable_BUS_t;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   I/O APIC Entry
 *   [ 8 bytes ]
 *
 * defines each I/O APIC present in the system
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#define IOAPIC_ISA		0
#define IOAPIC_PCI0		1
#define IOAPIC_PCI1		2
typedef struct mptable_IOAPIC {
	__u8	entry;
	__u8	id;
	__u8	version;
	__u8	enable		: 1,
		__reserved_1	: 7;
	__u32	address;
} __attribute__ ((packed)) mptable_IOAPIC_t;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   Local or I/O Interrupt Assignment Entry
 *   [ 8 bytes ]
 *
 * this entry defines interrupt routing for both the shared I/O APICs and
 * the processor specific Local APICs.  the entry structure is the same,
 * but some fields have different meanings when referring to I/O or Local
 * APICs.  see the comments below for each field.
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
enum mpINT_type {
	mpINT_type__INT		= 0x0, /* vectored interrupt from APIC table */
	mpINT_type__NMI		= 0x1, /* nonmaskable interrupt */
	mpINT_type__SMI		= 0x2, /* system management interrupt */
	mpINT_type__EXT		= 0x3  /* vectored interrupt from external PIC */
};
enum mpINT_polarity {
	mpINT_polarity__Def	= 0x0, /* conforms to bus spec */
	mpINT_polarity__Hi	= 0x1, /* active hi input polarity */
	mpINT_polarity__Lo	= 0x3  /* active lo input polarity */
};
enum mpINT_trigger {
	mpINT_trigger__Def	= 0x0, /* conforms to bus spec */
	mpINT_trigger__Edge	= 0x1, /* edge triggered */
	mpINT_trigger__Level	= 0x3  /* level triggered */
};

typedef struct mpINT_node {
	struct mptable_INT *entry;
	struct mpINT_node *next;
} mpINT_node_t;

typedef struct mptable_INT {
	__u8	entry;
	__u8	type;			/* enum <mpINT_type> */
	__u16	polarity	: 2,	/* enum <mpINT_polarity> */
		trigger		: 2,	/* enum <mpINT_trigger> */
		__reserved_1	: 12;
	struct {
		__u8 bus;
		union {
			__u8	isa;
			struct {
				__u8	pin		: 2,
					dev		: 5,
					__reserved_2	: 1;
			} __attribute__ ((packed)) pci;
		} irq;
	} __attribute__ ((packed)) src;
	union {
		struct {
			__u8	id;
			__u8	input;
		} __attribute__ ((packed)) ioapic;
		struct {
			__u8	id;
			__u8	input;
		} __attribute__ ((packed)) lapic;
	} dest;
} __attribute__ ((packed)) mptable_INT_t;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   System Address Space Mapping Entry
 *   [ 20 bytes ]
 *
 * part of Extended MP Configuration Table
 * define system addresses that are visiable on a a particular bus.
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
typedef struct mpextable_SysAddrMap {
	__u8	entry;
	__u8	length;	/* 20 bytes */
	__u8	bus;	/* bus id for mapped address */
	struct {
		__u8	type;	/* 0:IO 1:MMIO 2:PMMIO */
		__u64	base;	/* base address of region */
		__u64	length;	/* length of address space */
	} __attribute__ ((packed)) addr;
} __attribute__ ((packed)) mpextable_SysAddrMap_t;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   Bus Hierarchy Descriptor Entry
 *   [ 8 bytes ]
 *
 * part of Extended MP Configuration Table
 * define how I/O buses are connected relative to each other
 * in a system with more than one I/O bus.
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
typedef struct mpextable_BusTree {
	__u8	entry;
	__u8	length;			/* 8 bytes */
	__u8	bus;			/* bus ID for mapped address */
	__u8	decode		: 1,	/* 1 = uses all unclaimed addr */
		__reserved_1	: 7;
	__u8	parent;			/* bus id of parent */
	__u8	__reserved_2;
	__u8	__reserved_3;
	__u8	__reserved_4;
} __attribute__ ((packed)) mpextable_BusTree_t;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   Compatibility Bus Address Space Modififer Entry
 *   [ 8 bytes ]
 *
 * part of Extended MP Configuration Table
 * defines a set of predefined address ranges that should either be
 * added or removed from the supported address map ranges for a given bus
 * (used for ISA device compatibility)
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
enum mpextable_range_list {
	/* x100-x3FF x500-x7FF x900-xBFF xD00-xFFF */
	mpextable_range__ISA	= 0,
	/* x3B0-x3BB x3C0-x3DF x7B0-x7BB x7C0-x7DF
	 * xBB0-xBBB xBC0-xBDF xFB0-xFBB xFC0-xFDF */
	mpextable_range__VGA	= 1
};
typedef struct mpextable_BusCompat {
	__u8	entry;
	__u8	length;		/* 8 bytes */
	__u8	bus;		/* bus ID for mapped address */
	struct {
		__u8	modifier	: 1, /* 0=add, 1=subtract */
			__reserved_1	: 7;
		__u32	range;	/* <mpextable_range_list> */
	} __attribute__ ((packed)) addr;
} __attribute__ ((packed)) mpextable_BusCompat_t;

/*------------------------------------------------------------------------*/

void mptable_init(void);
void mptable_checksum(void);
void mptable_add_CPU(const struct cpu_info *);
void mptable_add_BUS(uint8, const char *);
void mptable_add_IOAPIC(int);
void mptable_add_INT(const mptable_INT_t *);

void mpextable_add_SysAddrMap(uint8, uint8, uint32, uint32);
void mpextable_add_PCIAddrMap(pci_bus *);
void mpextable_add_BusTree(uint8, uint8, uint8);
void mpextable_add_BusCompat(uint8, uint8, uint32);

mptable_INT_t * mpINT_find(uint8, uint8, uint8);
void mpINT_add(const mptable_INT_t *);

void __mptable_build(int);
#define mptable_dump()  __mptable_build(0)
#define mptable_build() __mptable_build(1)

/*------------------------------------------------------------------------*/

#endif /* COMMON_MPTABLE_H */
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
