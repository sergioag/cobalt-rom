/* $Id: pirq.c,v 1.1.1.1 2003/06/10 22:41:52 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 *
 * handles configuration of PCI Interrupt Routing Table
 * in memory (first 64k), also provides mechanism to
 * find/configure a pci interrupt route from PCI
 * bus & device/function
 *
 * Duncan Laurie
 */

#include <printf.h>
#include <extra_types.h>
#include <io.h>

#include "common/pirq.h"
#include "common/cmos.h"
#include "common/rom.h"
#include "common/pci.h"
#include "common/memory.h"
#include "common/systype.h"

#include "common/ioapic.h"
#include "common/mp.h"
#include "common/mptable.h"

/*
 * IRQ slot entries for constructing the
 * PCI Interrupt Routing Table in memory
 *
 * each entry is 16-bytes long, and describes
 * how a slot's PCI interrupt pins are wire
 * OR'd to the interrupt pins on other slots
 * and to the chipset's router input pins.
 */
struct pirq_entry {
	byte bus;		/* PCI Bus Number */
	byte devfn;		/* PCI_DEVFN(PCI_SLOT(devfn), 0) */
	struct {
		byte link;	/* IRQ pin info, 0=unused  */
		word map;	/* which IRQs are available */
	} __attribute__ ((packed)) irq[4]; /* INTA#, INTB#, INTC#, INTD# */
	byte slot;		/* PCI Slot #, 0=onboard device */
	byte reserved;		/* reserved */
} __attribute__ ((packed));

/*
 * PCI Interrupt Routing Table
 *
 * table mapped in memory (somewhere between F0000h and FFFFFh)
 * gives OS the ability to learn how each PCI function's
 * interrupt pin is connected to inputs on interrupt router
 *
 * size: must be > 32 bytes; 16 byte aligned
 * cksum: modulo 256 sum must be 0
 * version: must be 1.0
 */
struct pirq_table {
	dword signature;	/* ASCII: "$PIR" (0=24h,1=50h,2=49h,3=52h) */
	word version;		/* low byte is minor, high byte is major */
	word size;		/* total size of the table in bytes */
	byte pci_router_bus;	/* PCI bus # that interrupt router is on */
	byte pci_router_devfn;	/* PCI devfn of interrupt router */
	word pci_ex_bitmap;	/* map of IRQs devoted exclusively for PCI */
	word pci_vendor_id;	/* PCI vendor id of interrupt router */
	word pci_device_id;	/* PCI device id of interrupt router */
	dword miniport;		/* for IRQ miniport (not used; set to 0) */
	byte reserved[11];	/* reserved */
	byte checksum;		/* modulo 256 sum of all bytes in table=0 */
	struct pirq_entry slot[0];
} __attribute__ ((packed));

/* pointer to the current pirq map */
static struct pirq_table *pmap = (struct pirq_table *)PIRQ_BASE_ADDRESS;

static struct pirq_entry *find_slot(byte bus, byte devfn);
static byte pirq_route_pci_3k(pci_dev *dev, byte pin);
static byte pirq_route_pci_5k(pci_dev *dev, byte pin);
static int pirq_init_3k(void);
static int pirq_init_5k(void);


/*========================================================================

                 tables for defining interrupt routing
                  ! ! these are chipset specific ! !

          -  IOAPIC tables are not used in UP configuration
          -  both PIRQ and IOAPIC tables are used for SMP

==========================================================================*/

/* PCI Interrupt Routing Table for Monterey
 *
 * Notes: Monterey has the ruler slot which, while it looks like PCI, isn't.
 * Well, it carries three PCI device signals, but it isn't pinned like PCI.
 * Each 'slot' is 32bit, 33MHz capable.
 */
#define SERVERWORKS_PCIIRQ(slot)	((slot)|0x10)
static struct pirq_table pirq_map_5k_monterey = {
	PIRQ_SIGNATURE,			/* signature = $PIR */
	PIRQ_VERSION,			/* version = 0100 */
	PIRQ_TABLE_SIZE(7),		/* total size */
	0,				/* bus number of southbridge */
	PCI_DEVFN(0xf, 0),		/* device/func of southbridge */
	0,				/* exclusive PCI irqs */
	PCI_VENDOR_SERVERWORKS,		/* serverworks pci vendor id */
	PCI_DEVICE_SERVERWORKS_OSB4, 	/* southbridge pci device id */
	0,				/* miniport data (unused) */
	{0,0,0,0,0,0,0,0,0,0,0},	/* 11 reserved bytes */
	0,				/* modulo 256 checksum */
	/* 
	 * the important part - the pirq_entry array 
	 * the 'link' field of each entry is the value written to the OSB4
	 * interrupt routing register
	 */
	{				
		/* 00:0b:00 (eth0) - tied to PCIIRQ9 */
		{ 0, PCI_DEVFN(0xb, 0), {
			{SERVERWORKS_PCIIRQ(9), IRQMAP_PCI},
			{0, 0},
			{0, 0},
			{0, 0}}, 0, 0 },

		/* 00:0d:00 (eth1) - tied to PCIIRQ10 */
		{ 0, PCI_DEVFN(0xd, 0), {
			{SERVERWORKS_PCIIRQ(10), IRQMAP_PCI},
			{0, 0},
			{0, 0},
			{0, 0}}, 0, 0 },

		/* 00:0f:02 (usb) - use the internal redirect, not PCIIRQ */
		{ 0, PCI_DEVFN(0xf, 0), {
			{SERVERWORKS_IRQ_INDEX_USB, 1 << IRQ_USB},
			{0, 0},
			{0, 0},
			{0, 0}}, 0, 0 },

		/* 01:02:00 (slot1, 64/66 PCI slot) */
		{ 1, PCI_DEVFN(0x2, 0), {
			{SERVERWORKS_PCIIRQ(4), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(5), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(4), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(5), IRQMAP_PCI}}, 1, 0 },

		/* 01:03:00 (slot2, 32/66 - ruler device 1) */
		{ 1, PCI_DEVFN(0x3, 0), {
			{SERVERWORKS_PCIIRQ(6), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(7), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(8), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(7), IRQMAP_PCI}}, 2, 0 },

		/* 01:04:00 (slot3, 32/66 - ruler device 2) */
		{ 1, PCI_DEVFN(0x4, 0), {
			{SERVERWORKS_PCIIRQ(7), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(8), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(7), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(6), IRQMAP_PCI}}, 3, 0 },

		/* 01:05:00 (slot4, 32/66 - ruler device 3) */
		{ 1, PCI_DEVFN(0x5, 0), {
			{SERVERWORKS_PCIIRQ(8), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(7), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(6), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(7), IRQMAP_PCI}}, 4, 0 }
	}
};

/* PCI Interrupt Routing Table for RaQ-Alpine */
static struct pirq_table pirq_map_5k_alpine = {
	PIRQ_SIGNATURE,			/* signature = $PIR */
	PIRQ_VERSION,			/* version = 0100 */
	PIRQ_TABLE_SIZE(4),		/* total size */
	0,				/* bus number of southbridge */
	PCI_DEVFN(0xf, 0),		/* device/func of southbridge */
	0,				/* exclusive PCI irqs */
	PCI_VENDOR_SERVERWORKS,		/* serverworks pci vendor id */
	PCI_DEVICE_SERVERWORKS_CSB5, 	/* southbridge pci device id */
	0,				/* miniport data (unused) */
	{0,0,0,0,0,0,0,0,0,0,0},	/* 11 reserved bytes */
	0,				/* modulo 256 checksum */
	/* 
	 * the important part - the pirq_entry array 
	 * the 'link' field of each entry is the value written to the
	 * interrupt routing register
	 */
	{				
		/* 00:0f:02 (usb) - use the internal redirect, not PCIIRQ */
		{ 0, PCI_DEVFN(0xf, 0), {
			{SERVERWORKS_IRQ_INDEX_USB, 1 << IRQ_USB},
			{0, 0},
			{0, 0},
			{0, 0}}, 0, 0 },

		/* 01:06:00 (eth0) - tied to PCIIRQ0 */
		{ 1, PCI_DEVFN(0x6, 0), {
			{SERVERWORKS_PCIIRQ(0), IRQMAP_PCI},
			{0, 0},
			{0, 0},
			{0, 0}}, 0, 0 },

		/* 01:07:00 (eth1) - tied to PCIIRQ1 */
		{ 1, PCI_DEVFN(0x7, 0), {
			{SERVERWORKS_PCIIRQ(1), IRQMAP_PCI},
			{0, 0},
			{0, 0},
			{0, 0}}, 0, 0 },

		/* 01:08:00 (slot0, 64/66 PCI slot) */
		{ 1, PCI_DEVFN(0x8, 0), {
			{SERVERWORKS_PCIIRQ(2), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(3), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(4), IRQMAP_PCI},
			{SERVERWORKS_PCIIRQ(5), IRQMAP_PCI}}, 1, 0 }
	}
};

/*
 * pirq_table_init()
 *
 * perform basic verification on the IRQ routing table in memory
 */
int
pirq_init(void)
{
	switch (systype())
	{
	case SYSTYPE_3K:
	  	return pirq_init_3k();
	case SYSTYPE_5K:
	  	return pirq_init_5k();
	default:
	}

	return -1;
}

static int
pirq_init_3k(void)
{
	/* INTA# -> IRQ 9 INTB# -> IRQ 10 */
	pci_write_config_byte(0x0, PCI_DEVFN(0x7,0), 0x48, 0x31);
	/* INTC# -> IRQ 11 INTD# -> IRQ 12 */
	pci_write_config_byte(0x0, PCI_DEVFN(0x7,0), 0x49, 0xb9);
	/* FIXME: Edge trigger the devices */
	pci_write_config_byte(0x0, PCI_DEVFN(0x7,0), 0x4c, 0x0f);

	return 0;
}

static int
pirq_init_5k(void)
{
	byte *mapptr = (byte *)pmap;
	word sum = 0;
	byte j;

	DPRINTF("PIRQ: Relocate table to 0x%x ", (unsigned int)mapptr);

	/* remap the table to <1M area, so OS can find it */
	switch (boardtype())
	{
	case BOARDTYPE_MONTEREY:
		memcpy(mapptr, &pirq_map_5k_monterey, pirq_map_5k_monterey.size);
		break;
	case BOARDTYPE_ALPINE:
		memcpy(mapptr, &pirq_map_5k_alpine, pirq_map_5k_alpine.size);
		break;
	default:
		return -1;
	}

	/* calculate checksum; modulo 256 */
	for (j=0; j<pmap->size; j++)
		sum += mapptr[j];
	if (sum % 256)
		pmap->checksum = (((sum >> 8) + 1) << 8) - sum;

	DPRINTF("(%d bytes, %d entries, checksum %02x)\n",
		pmap->size, PMAP_NSLOTS(pmap), pmap->checksum);

	/* verify signature */
	if (pmap->signature != PIRQ_SIGNATURE) {
		DPRINTF("PIRQ: Invalid table signature 0x%04x "
			"(should be 0x%04x)\n", 
			pmap->signature, PIRQ_SIGNATURE);
		return -1;
	}

	/* verify version */
	if (pmap->version != PIRQ_VERSION) {
		DPRINTF("PIRQ: Invalid table version %d.%d "
			"(should be %d.%d)\n",
			(pmap->version >> 2), (pmap->version & 0xff),
			(PIRQ_VERSION >> 2), (PIRQ_VERSION & 0xff));
		return -1;
	}

	/* verify table size & alignment */
	if (pmap->size % 16 || pmap->size < sizeof(*pmap)) {
		DPRINTF("PIRQ: Invalid table size 0x%04x\n", pmap->size);
		return -1;
	}

	return 0;
}


/* 
 * pirq_route_pci()
 *
 * Configure IRQ routing for a PCI bus & device/function
 * by looking it up in the interrupt routing table.
 * Program it to the chipset, and return mapped IRQ
 */
byte 
pirq_route_pci(pci_dev *dev, byte pin)
{
	if (!dev || pin<1 || pin>4)
		return 0;

	pin--; /* zero based arrays */

	switch (systype())
	{
	case SYSTYPE_3K:
	  	return pirq_route_pci_3k(dev, pin);
	case SYSTYPE_5K:
	  	return pirq_route_pci_5k(dev, pin);
	default:
	}

	return 0;
}

/*
 * NOTE: 
 * according to PCI-PCI Bridge Spec, INT# lines behind a bridge are mapped 
 * on a sliding window, based on device number and int pin:
 * (dev_num % 4) == 0:
 *      INTA# -> INTA#
 *      INTB# -> INTB#
 *      INTC# -> INTC#
 *      INTD# -> INTD#
 * (dev_num % 4) == 1:
 *      INTA# -> INTB#
 *      INTB# -> INTC#
 *      INTC# -> INTD#
 *      INTD# -> INTA#
 * (dev_num % 4) == 2:
 *      INTA# -> INTC#
 *      INTB# -> INTD#
 *      INTC# -> INTA#
 *      INTD# -> INTB#
 * (dev_num % 4) == 3:
 *      INTA# -> INTD#
 *      INTB# -> INTA#
 *      INTC# -> INTB#
 *      INTD# -> INTC#
 */
static byte
pirq_route_pci_3k(pci_dev *dev, byte pin)
{
	int intlines[] = {9, 10, 11, 12};

	/* FIXME: make this more dynamic, what about multiple bridges? */
	if (dev->bus->busnum == 0) {
	    switch( dev->devfn )
	    {
		    /* USB */
		case PCI_DEVFN( 0x02, 0x00 ):
		    return 9;
		    break;
	    
			/* SCSI */
		case PCI_DEVFN( 0x0e, 0x00 ):
		    return 12;
		    break;
	    
			/* IDE */
		case PCI_DEVFN( 0x0f, 0x00 ):
		    return 14;
		    break;
		    
			/* eth0 */
		case PCI_DEVFN( 0x10, 0x00 ):
		    return 11;
		    break;
	    
			/* eth 1*/
		case PCI_DEVFN( 0x12, 0x00 ):
		    return 10;
		    break;

		default:
		    return intlines[pin];
	    }
	} else {
		/* bridged bus - see note above */
		return intlines[((PCI_SLOT(dev->devfn) % 4) + pin)];
	}
}

static byte
pirq_route_pci_5k(pci_dev *dev, byte pin)
{
	mptable_INT_t mpint;
	struct pirq_entry *pirq_ent;
	byte index, irq;
	word map;

	/* find the pirq_entry for this device */
	pirq_ent = find_slot(dev->bus->busnum, dev->devfn);

	if (pirq_ent) {
		/* the link field holds chipset info for the PCI IRQ */
		index = pirq_ent->irq[pin].link;
		map = pirq_ent->irq[pin].map;
	}
	else {
		/* didn't find it in the table - see if it's across a bridge */
		pci_bus *b = dev->bus;
 		byte realpin;

		DPRINTF("PIRQ: Entry for device %02x:%02x:%02x not found, "
			"assuming PCI->PCI bridge.\n", dev->bus->busnum,
			PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn));

		if (b->root) {
			/* don't mess with host bus */
			printf("PIRQ: unable to map IRQ for host-bus %d\n",
			       b->busnum);
			return 0;
		}
		
		/* walk up the hierarchy until we get to a hostbus */
		while (!b->parent->root) 
			b = b->parent;

		pirq_ent = find_slot(b->parent->busnum, b->self->devfn);
		if (!pirq_ent) {
			/* again, didn't find it */
			printf("PIRQ: no map for device %02x:%02x:%02x or "
			       "top level bridge %02x:%02x:%02x\n",
			       dev->bus->busnum, PCI_SLOT(dev->devfn), 
			       PCI_FUNC(dev->devfn), b->parent->busnum,
			       PCI_SLOT(b->self->devfn),
			       PCI_FUNC(b->self->devfn));
			return 0;
		}

		/* FIXME: Does this shift for every bridge, or just once? */
		realpin = (PCI_SLOT(dev->devfn) % 4) + pin;
		index = pirq_ent->irq[realpin].link;
		map = pirq_ent->irq[realpin].map;
	}

	/* IRQ is not connected if link is 0 */
	if (!index)
		return 0;

	irq = pciirq_5k(index, map);
	if (!irq)
		return 0;
	pciirq_5k_set_level(irq);

	/* build MP config table interrupt entry */
	memset(&mpint, 0, sizeof(mpint));

	mpint.entry		= mp_entry_IOINT;
	mpint.type		= mpINT_type__INT;
	mpint.polarity		= mpINT_polarity__Lo;
	mpint.trigger		= mpINT_trigger__Level;
	mpint.src.bus		= dev->bus->busnum;
	mpint.src.irq.pci.dev	= PCI_SLOT(dev->devfn);
	mpint.src.irq.pci.pin	= pin;
	mpint.dest.ioapic.id	= ioapic_get_id(IOAPIC_PCI0);
	mpint.dest.ioapic.input	= index & 0xf;

	mpINT_add((const mptable_INT_t *)&mpint);

	return irq;
}

static struct pirq_entry *
find_slot(byte bus, byte devfn)
{
	int i;

	devfn = PCI_DEVFN(PCI_SLOT(devfn), 0);

	for (i = 0; i < PMAP_NSLOTS(pmap); i++) {
		if (pmap->slot[i].bus == bus &&
		    pmap->slot[i].devfn == devfn) {
			return &pmap->slot[i];
		}
	}

	return NULL;
}

byte
pciirq_lookup_5k(byte index)
{
	outb(index, SERVERWORKS_IRQ_INDEX);
	return inb(SERVERWORKS_IRQ_REDIRECT);
}

byte
pciirq_5k(byte index, word map)
{
	static byte last = 0;

	/* find the next available irq in bitmap */
	do { last = (last<15) ? last+1 : 0; }
	while (!((1<<last) & map));

	/* tell chipset to route this PCIIRQ to selected INT */
	outb(index, SERVERWORKS_IRQ_INDEX);
	outb(last, SERVERWORKS_IRQ_REDIRECT);

	return last;
}

/* set IRQ level triggered in ELCR */
void
pciirq_5k_set_level(byte irq)
{
	byte reg;

	if (irq > 7) {
		reg = inb(PIRQ_ELCR_REG+1);
		reg |= 1 << (irq-8);
		outb(reg, PIRQ_ELCR_REG+1);
	}
	else {
		reg = inb(PIRQ_ELCR_REG);
		reg |= 1 << irq;
		outb(reg, PIRQ_ELCR_REG);
	}
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
