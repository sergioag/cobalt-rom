/* $Id: mptable.c,v 1.1.1.1 2003/06/10 22:41:50 iceblink Exp $
 * Copyright (c) 1999-2000 Cobalt Networks, Inc.
 * Copyright (c) 2001 Sun Microsystems, Inc.
 *
 *   Duncan Laurie <smp@sun.com>
 */


#include <printf.h>
#include <stdlib.h>

#include "common/mptable.h"
#include "common/mp.h"
#include "common/ioapic.h"
#include "common/apic.h"
#include "common/pirq.h"
#include "common/pci.h"
#include "common/cmos.h"
#include "common/rom.h"

static struct mptable_flpointer *mpptr;
static struct mptable_cfheader *mptable;
static uint8 *mpe;
static int ISA_busid;
static int mptable_save_flag;

/*##################################################################*/

static mpINT_node_t *mpINT_list;
static mpINT_node_t **mpINT_list_head = &mpINT_list;

/**
 *	mpINT_add()
 *
 *	@intr:	mptable INT entry struct
 *
 *	Create new `MP INT' entry for internal INT list.
 */
void
mpINT_add (const mptable_INT_t *intr)
{
	mpINT_node_t *node;

	/* create list member */
	node = malloc(sizeof(*node));
	if (!node) return;
	memset(node, 0, sizeof(*node));

	/* create list member interrupt entry */
	node->entry = malloc(sizeof(*(node->entry)));
	if (node->entry == NULL) {
		free(node);
		return;
	}
	memcpy(node->entry, intr, sizeof(*(node->entry)));

	/* insert the entry into the list */
	*mpINT_list_head = node;
	mpINT_list_head = &node->next;
}

/**
 *	mpINT_find()
 *
 *	@bus:	pci bus number
 *	@dev:	pci device id
 *	@pin:	pci interrupt pin
 *
 *	Find a node in the internal INT list.
 *	Return node entry if found, or NULL.
 */
mptable_INT_t *
mpINT_find (uint8 bus, uint8 dev, uint8 pin)
{
	mpINT_node_t *cur;

	for (cur=mpINT_list; cur; cur=cur->next)
		if (cur->entry->src.bus == bus &&
		    cur->entry->src.irq.pci.dev == dev &&
		    cur->entry->src.irq.pci.pin == pin)
			return cur->entry;
	return NULL;
}

/*##################################################################*/

/*
 * initialize MP tables
 *
 *   construct floating pointer table,
 *   compute floating pointer checksum,
 *   construct MP config table,
 *   setup pointer to first MP entry
 */
void
mptable_init (void)
{
	static const char oem_id[8] = MP_OEM_ID;
	char *prod_id = (char *)systype_productid_str();

	mpptr = (void *)(MP_BASE_ADDRESS);

	/*
	 * construct MP floating table
	 */
	memset(mpptr, 0, sizeof(*mpptr));

	mpptr->signature	= MPTABLE_FLPOINTER_SIG;
	mpptr->address		= MP_BASE_ADDRESS + sizeof(*mpptr);
	mpptr->length		= 1;
	mpptr->version		= MP_SPEC_REV;
	mpptr->feature.config	= 0;	/* mp config table defined */
	mpptr->feature.imcr	= 0;	/* virtual wire mode */	
	mpptr->feature.mclocks	= 0;	/* single CPU clock source */

	/* construct MP config table */
	mptable = (void *)(mpptr->address);

	memset(mptable, 0, sizeof(*mptable));
	mptable->signature	= MPTABLE_CFHEADER_SIG;
	mptable->length		= sizeof(*mptable);
	mptable->version	= MP_SPEC_REV;
	mptable->apic		= lapic_get_base();

	memcpy(mptable->id.oem, oem_id, sizeof(mptable->id.oem));
	memcpy(mptable->id.product, prod_id, sizeof(mptable->id.product));

	/* clear interrupt list */
	mpINT_list = NULL;

	/* initialize pointer to first mp entry */
	mpe = (uint8 *)(mpptr->address + mptable->length);
}

static void
mptable_print_INT (const mptable_INT_t *e)
{
	printf("MPTABLE:  ");

	switch (e->type) {
	case mpINT_type__EXT: printf(" EXT"); break;
	case mpINT_type__INT: printf(" INT"); break;
	case mpINT_type__NMI: printf(" NMI"); break;
	case mpINT_type__SMI: printf(" SMI"); break;
	}

	switch (e->polarity) {
	case mpINT_polarity__Lo:  printf("   active-lo"); break;
	case mpINT_polarity__Hi:  printf("   active-hi"); break;
	case mpINT_polarity__Def: printf("        spec"); break;
	}

	switch (e->trigger) {
	case mpINT_trigger__Level: printf("     level"); break;
	case mpINT_trigger__Edge:  printf("      edge"); break;
	case mpINT_trigger__Def:   printf("      spec"); break;
	}

	if (e->src.bus == ISA_busid) {
		printf("    ISA   %4d", e->src.irq.isa);
	}
	else {
		printf("   PCI%1d   %2d:%c",
		       e->src.bus, e->src.irq.pci.dev,
		       'A' + e->src.irq.pci.pin);
	}

	printf("   %4d", e->dest.ioapic.id);
	printf("   %3d", e->dest.ioapic.input);
	printf("\n");
}

/* add mptable entries for the pci buses */
static int
mptable_pci_walk(pci_bus *bus)
{
	int id = 0;

	while (bus) {
		mptable_add_BUS(bus->busnum, MPTABLE_BUS_PCI);
		if (bus->kids)
			id += mptable_pci_walk(bus->kids);
		++id;
		bus = bus->peer;
	}

	return id;
}

/*
 * construct MP tables, compute checksums
 */
void
__mptable_build (int save)
{
	mpINT_node_t *list;
	struct cpu_info *info;
	mptable_INT_t mpent;
	pci_bus *bus;
	uint8 j=0;

	if (systype_is_3k()) /* 3k dont tread on me */
		return;

	mptable_save_flag = save;

	/* MP entry : Processor */
	for (info=systype_get_info(0); info; info=info->NEXT)
		mptable_add_CPU(info);

	/* MP entry : Bus */
	ISA_busid = mptable_pci_walk(pci_lookup_bus(0));

	/* ISA bus is last */
	mptable_add_BUS(ISA_busid, MPTABLE_BUS_ISA);

	/* MP entry : IO-APIC */
	for (j=0; j<IOAPIC_NUM; j++) 
		mptable_add_IOAPIC(j);

	/* I/O INTINn : PCI Interrupts */
	DPRINTF("MPTABLE:                 I/O Interrupt Entries\n");
	DPRINTF("MPTABLE:  TYPE    POLARITY   TRIGGER    BUS    IRQ   APIC   INT \n");
	DPRINTF("MPTABLE: -------------------------------------------------------\n");
	for (j=0; j<ISA_busid; j++) {
		for (list=mpINT_list; list; list=list->next) {
			if (list->entry->src.bus != j)
				continue;
			mptable_add_INT((const mptable_INT_t *)list->entry);
		}
	}

	/* I/O INTIN0 : Timer Interrupt */

	/* in `Virtual Wire Mode' the Timer is routed as Ext IRQ 0 */
	memset(&mpent, 0, sizeof(mpent));
	mpent.entry		= mp_entry_IOINT;
	mpent.type		= mpINT_type__EXT;
	mpent.polarity		= mpINT_polarity__Hi;
	mpent.trigger		= mpINT_trigger__Edge;
	mpent.src.bus		= ISA_busid;
	mpent.dest.ioapic.id	= ioapic_get_id(IOAPIC_ISA);
	mpent.src.irq.isa	= 0;
	mpent.dest.ioapic.input	= 0;
	mptable_add_INT((const mptable_INT_t *)&mpent);

	/* in `Symmetric Mode' the Timer is routed to IO-APIC INT 0 */
	memset(&mpent, 0, sizeof(mpent));
	mpent.entry		= mp_entry_IOINT;
	mpent.type		= mpINT_type__INT;
	mpent.polarity		= mpINT_polarity__Hi;
	mpent.trigger		= mpINT_trigger__Edge;
	mpent.src.bus		= ISA_busid;
	mpent.dest.ioapic.id	= ioapic_get_id(IOAPIC_ISA);
	mpent.src.irq.isa	= 0;
	mpent.dest.ioapic.input	= 0;
	mptable_add_INT((const mptable_INT_t *)&mpent);

	/* I/O INTINn : ISA Interrupts */
	mpent.entry		= mp_entry_IOINT;
	mpent.type 		= mpINT_type__INT;
	mpent.polarity		= mpINT_polarity__Hi;
	mpent.trigger		= mpINT_trigger__Edge;
	mpent.src.bus		= ISA_busid;
	mpent.dest.ioapic.id	= ioapic_get_id(IOAPIC_ISA);
	for (j=0; j<ioapic_get_entries(IOAPIC_ISA); j++) {
		if (!(IRQMAP_ISA & (1<<j)))
			continue;
		mpent.src.irq.isa	= j;
		mpent.dest.ioapic.input	= j;
		mptable_add_INT((const mptable_INT_t *)&mpent);
	}

	/* now process extra ISA bus entries */
	for (list=mpINT_list; list; list=list->next) {
		if (list->entry->src.bus != 0xff)
			continue;
		list->entry->src.bus = ISA_busid; /* set ISA bus # */
		mptable_add_INT((const mptable_INT_t *)list->entry);
	}

	DPRINTF("MPTABLE:                Local Interrupt Entries\n");
	DPRINTF("MPTABLE:  TYPE    POLARITY   TRIGGER    BUS    IRQ   APIC   INT \n");
	DPRINTF("MPTABLE: -------------------------------------------------------\n");

	/* Local INTIN0 : ExtINT (Virtual Wire) */
	memset(&mpent, 0, sizeof(mpent));
	mpent.entry		= mp_entry_LocalINT;
	mpent.type		= mpINT_type__EXT;
	mpent.polarity		= mpINT_polarity__Hi;
	mpent.trigger		= mpINT_trigger__Edge;
	mpent.src.bus		= ISA_busid;
	mpent.src.irq.isa	= 0;
	mpent.dest.lapic.id	= 0xff;
	mpent.dest.lapic.input	= 0;
	mptable_add_INT((const mptable_INT_t *)&mpent);

	/* Local INTIN1 : NMI */
	memset(&mpent, 0, sizeof(mpent));
	mpent.entry		= mp_entry_LocalINT;
	mpent.type		= mpINT_type__NMI;
	mpent.polarity		= mpINT_polarity__Hi;
	mpent.trigger		= mpINT_trigger__Edge;
	mpent.src.bus		= 0;
	mpent.src.irq.isa	= 0;
	mpent.dest.lapic.id	= 0xff;
	mpent.dest.lapic.input	= 1;
	mptable_add_INT((const mptable_INT_t *)&mpent);

	/* MP Entry : PCI bus hierarchy
	 * ~ System Address Map
	 * ~ Bus Hierarchy
	 * ~ Bus Compatibility
	 */
	for (bus=pci_lookup_bus(0); bus; bus=bus->peer)	{
		pci_bus *sub;

		/* add system address mappings for {{P,}MM,}IO regions */
		mpextable_add_PCIAddrMap(bus);

		/* add bus compatibility entry:
		 * ~ predefined ISA range,
		 * ~ add(0) if bus 0, otherwise subtract(1)
		 */
		mpextable_add_BusCompat(bus->busnum,
					(bus->busnum==0) ? 0 : 1,
					mpextable_range__ISA);

		/* traverse bridges: subordinate first, then peer */
		for (sub=bus->kids; sub;
		     sub=(sub->kids) ? sub->kids : sub->peer) {

			/* add mptable entry for this bus in hierarchy:
			 * ~ bus does not use unclaimed addr */
			mpextable_add_BusTree(sub->busnum, 0,
					      sub->parent->busnum);

			/* add compatibility entry: subtract ISA range */
			mpextable_add_BusCompat(sub->busnum, 1,
						mpextable_range__ISA);
		}
	}

	/* ISA Bus hierarchy: subtractive decode, parent bus 0 */
	mpextable_add_BusTree(ISA_busid, 1, 0);

	/* calculate table checksums */
	mptable_checksum();
}

/*
 * compute checksums:
 *   MP Extended Entries
 *   MP Base Config Table
 */
void
mptable_checksum (void)
{
	const uint8 *memptr;
	uint8 sum;
	uint16 j;

	if (!mptable_save_flag)
		return;

	/* compute floating table checksum */
	memptr = (uint8 *) mpptr;
	for (j=sum=0; j<sizeof(*mpptr); j++) {
		sum -= memptr[j];
	}
	mpptr->checksum = sum;

	/* compute extended entries checksum */
	mptable->checksum = mptable->ext.checksum = 0;
	memptr = (uint8 *) mpe;
	for (j=sum=0; j<(mptable->ext.length); j++) {
		sum -= memptr[j];
	}
	mptable->ext.checksum = sum;

	/* compute MP config table checksum */
	memptr = (uint8 *) mptable;
	for (j=sum=0; j<(mptable->length); j++) {
		sum -= memptr[j];
	}
	mptable->checksum = sum;

	DPRINTF("SMP: floating table base 0x%08x (%d bytes, checksum %02x)\n",
		(uint32)mpptr, mpptr->length * 16, mpptr->checksum);
	DPRINTF("SMP: config mptable base 0x%08x (%d bytes, checksum %02x)\n",
		(uint32)memptr, mptable->length, mptable->checksum);
	DPRINTF("SMP: extended entry base 0x%08x (%d bytes, checksum %02x)\n",
		(uint32)(memptr + mptable->length), mptable->ext.length,
		mptable->ext.checksum);
}

/**
 *	mptable_add_CPU()
 *
 *	@cpu:	structure containing cpu data
 *
 *	Add `CPU' entry to MP Config Table.
 */
void
mptable_add_CPU (const struct cpu_info *info)
{
	mptable_CPU_t *cpu = (void *) mpe;

	if (!mptable_save_flag)
		return;

	/* move existing extended entries out of the way */
	if (mptable->ext.length > 0) {
		memmove(mpe + sizeof(*cpu), mpe, mptable->ext.length);
	}

	memset(cpu, 0, sizeof(*cpu));

	cpu->entry	= mp_entry_CPU;
	cpu->apic.id	= info->apic_id;
	cpu->apic.version = info->apic_version;
	cpu->enable	= 1;
	cpu->bsp	= info->bsp;
	cpu->stepping	= info->stepping & 0xf;
	cpu->model	= info->model & 0xf;
	cpu->family	= info->family & 0xf;
	cpu->features	= info->feature_mask;

	/* adjust MP table length and entry pointer */
	mptable->length += sizeof(*cpu);
	(mptable->count)++;
	mpe += sizeof(*cpu);
}

/**
 *	mptable_add_BUS()
 *	@id:	bus identifier
 *	@type:	type of bus (6 byte string)
 *
 *	Add `Bus' entry to MP Config Table.
 */
void
mptable_add_BUS (__u8 id, const char *type)
{
	mptable_BUS_t *mpbus = (void *) mpe;

	if (!mptable_save_flag)
		return;

	memset(mpbus, 0, sizeof(*mpbus));

	mpbus->entry = mp_entry_BUS;
	mpbus->id = id;
	memcpy(mpbus->type, type, 6);

	/* adjust MP table length and entry pointer */
	mptable->length += sizeof(*mpbus);
	(mptable->count)++;
	mpe += sizeof(*mpbus);

	DPRINTF("MPTABLE: Bus #%d is %s\n", id, type);
}

/**
 *	mptable_add_IOAPIC()
 *
 *	@num:	logical IO-APIC number
 *
 *	Add `IO-APIC' entry to MP Config Table.
 */
void
mptable_add_IOAPIC (int num)
{
	mptable_IOAPIC_t *mpapic = (void *) mpe;

	if (!mptable_save_flag)
		return;

	memset(mpapic, 0, sizeof(*mpapic));

	mpapic->entry	= mp_entry_IOAPIC;
	mpapic->id	= ioapic_get_id(num);
	mpapic->version	= ioapic_get_version(num);
	mpapic->address	= ioapic_get_base(num);
	mpapic->enable	= 1;

	/* adjust MP table length and entry pointer */
	mptable->length += sizeof(*mpapic);
	(mptable->count)++;
	mpe += sizeof(*mpapic);
}

/**
 *	mptable_add_INT()
 *
 *	@intr:	mptable INT entry struct
 *
 *	Add `IO-INT' or `Local-INT' entry to MP Config Table.
 */
void
mptable_add_INT (const mptable_INT_t *intr)
{
	mptable_INT_t *mpint = (void *) mpe;
	size_t len = sizeof(*intr);

	if (debug)
		mptable_print_INT(intr);

	if (!mptable_save_flag)
		return;

	/* copy mptable INT entry supplied as first arg */
	memset(mpint, 0, len);
	memcpy(mpint, intr, len);

	/* adjust MP table length and entry pointer */
	(mptable->count)++;
	mptable->length += len;
	mpe += len;
}

/**
 *	mpextable_add_SysAddrMap()
 *
 *	@bus:	bus id for mapped address
 *	@type:	IO | MMIO | PMMIO
 *	@base:	base address of region
 *	@len:	region length
 *
 *	Add `System Address Map' entry to MP Extended Config Table.
 */
void
mpextable_add_SysAddrMap (__u8 bus, __u8 type, __u32 base, __u32 len)
{
	mpextable_SysAddrMap_t *map;

	if (!mptable_save_flag)
		return;

	map = (void *)(mpe + mptable->ext.length);
	memset(map, 0, sizeof(*map));

	map->entry	= mp_entry_SysAddrMap;
	map->length	= 20; /* 20 bytes */
	map->bus	= bus;
	map->addr.type	= type;
	map->addr.base	= (__u64) base;
	map->addr.length = (__u64) len;

	mptable->ext.length += map->length;
}

/**
 *	mpextable_add_BusTree()
 *
 *	@bus:		bus id for this node
 *	@decode:	subtractive decode flag
 *	@parent:	bus id of parent node
 *
 *	Add `Bus Hierarchy' entry to MP Extended Config Table.
 */
void
mpextable_add_BusTree (__u8 bus, __u8 decode, __u8 parent)
{
	mpextable_BusTree_t *tree;

	if (!mptable_save_flag)
		return;

	tree = (void *)(mpe + mptable->ext.length);
	memset(tree, 0, sizeof(*tree));

	tree->entry	= mp_entry_BusTree;
	tree->length	= 8; /* 8 bytes */
	tree->bus	= bus;
	tree->decode	= decode;
	tree->parent	= parent;

	mptable->ext.length += tree->length;
}

/**
 *	mpextable_add_BusCompat()
 *
 *	@bus:	bus id for mapped address
 *	@mod:	address modifier (0=add, 1=subtract)
 *	@range:	predefined range list entry
 *
 *	Add `Bus Compatibility' entry to MP Extended Config Table.
 */
void
mpextable_add_BusCompat (__u8 bus, __u8 mod, __u32 range)
{
	mpextable_BusCompat_t *compat;

	if (!mptable_save_flag)
		return;

	compat = (void *)(mpe + mptable->ext.length);
	memset(compat, 0, sizeof(*compat));

	compat->entry		= mp_entry_BusCompat;
	compat->length		= 8; /* 8 bytes */
	compat->bus		= bus;
	compat->addr.modifier	= mod;
	compat->addr.range	= range;

	mptable->ext.length += compat->length;
}

/**
 *	mpextable_add_PCIAddrMap()
 *
 *	@bus:	PCI bus to add
 *
 *	Add `PCI Address Map' entry to MP Extended Config Table.
 */
void
mpextable_add_PCIAddrMap (pci_bus *bus)
{
	pci_bar_t type;

	if (!mptable_save_flag)
		return;

	for (type = PCI_BAR_IO; type <= PCI_BAR_PMMIO; type++) {
		mpextable_add_SysAddrMap(bus->busnum, type - 1,
					 bus->resource[type].base,
					 bus->resource[type].size);
	}
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
