/* $Id: pci_resource.c,v 1.1.1.1 2003/06/10 22:41:52 iceblink Exp $
 *
 * Copyright 2000 Cobalt Networks, Inc.
 * Copyright 2001 Sun Microsystems, Inc.
 */

#include <printf.h>

#include "common/pci.h"
#include "common/pci_regs.h"
#include "common/pci_fixup.h"
#include "common/pci_resource.h"
#include "common/region.h"
#include "common/alloc.h"
#include "common/rom.h"
#include "common/cmos.h"
#include "common/rammap.h"

static uint32 __pci_resource_align_dev[4] = {
	0x0,
	0x3,	/* IO    =   4 bytes */
	0xf,	/* MMIO  =  16 bytes */
	0xf,	/* PMMIO =  16 bytes */
};

static uint32 __pci_resource_align_bus[4] = {
	0x0,
	0x3,	/* IO    =   4 bytes */
	0xffff, /* MMIO  = 64K bytes */
	0xffff, /* PMMIO = 64K bytes */
};

static uint32 __pci_resource_align_bridge[4] = {
	0x0,
	0xfff,	 /* IO    = 4K bytes */
	0xfffff, /* MMIO  = 1M bytes */
	0xfffff, /* PMMIO = 1M bytes */
};

static uint32 __pci_resource_align_cardbus[4] = {
	0x0,
	0x3ff,	 /* IO    =  4 bytes (but linux wants 1K) */
	0xfff,   /* MMIO  = 4K bytes */
	0xfff,   /* PMMIO = 4K bytes */
};

static byte bridge;

static uint32 pci_resource_allocate(pci_bus *, pci_bar_t);

/*
 * initialize/allocate PCI resource regions
 */
void
pci_resource_init(pci_bus *root)
{
	uint32 base;

	/* IO resource */
	PCIDBG("Init Resource: I/O Ports\n");
	region_init(PCI_BAR_IO,
		    __pci_resource_align_dev[PCI_BAR_IO],
		    (RAM_PCI_REGION_IO), "I/O");
	pci_resource_allocate(root, PCI_BAR_IO);

	/* MMIO resource */
	PCIDBG("Init Resource: Non-prefetchable Memory\n");
	region_init(PCI_BAR_MMIO, 
		    __pci_resource_align_dev[PCI_BAR_MMIO],
		    (RAM_PCI_REGION_MMIO), "MMIO");
	base = pci_resource_allocate(root, PCI_BAR_MMIO);

	/* PMMIO resource */
	PCIDBG("Init Resource: Prefetchable Memory\n");

	/* if there's a PCI-PCI bridge, we need to re-align the base
         * to avoid a potential overlap */
	if (bridge) {
		base &= ~__pci_resource_align_bridge[PCI_BAR_PMMIO];
		--base;
	}
	region_init(PCI_BAR_PMMIO,
		    __pci_resource_align_dev[PCI_BAR_PMMIO],
		    base, /* use base from MMIO region as limit */
		    "PMMIO");
	pci_resource_allocate(root, PCI_BAR_PMMIO);
}

/*
 * allocate resources, working from last bus to first bus
 */
static uint32
pci_resource_allocate(pci_bus *bus, pci_bar_t type)
{
	pci_bar *map = &bus->resource[type];
	pci_dev *dev;

	/* peer bus resources must be mutually exclusive
	 * start with last bus in peer list
	 */
	if (bus->peer) {
		pci_resource_allocate(bus->peer, type);
	}

	/* bus address regions have stricter alignments than devices */
	map->limit = region_align(type, __pci_resource_align_bus[type]) - 1;

	/* now try subordinate buses */
	if (bus->kids) {
		pci_resource_allocate(bus->kids, type);
	}

	DBGINDENT(3);

	/* iterate through all devices on this bus */
	for (dev=bus->devices; dev; dev=dev->busnext) {
		pci_bar *b;
		int j,p;

		/* run PRE fixup handlers--skip BAR init if return is >0 */
		if (0 < pci_resource_fixup_pre(dev, type))
			continue;

		/* check all 6 BARs for each device */
		for (b=&dev->bar[j=p=0]; j<6; b=&dev->bar[++j]) {
			/* only do one resource type at a time
			 * to minimize wasted space */
			if (b->type != type)
				continue;

			/* allocate (type) resource of (b->size) bytes */
			b->base = region_allocate(type, b->size);
			if (!b->base)
				continue;

			b->limit = b->base + b->size - 1;

			/* save new address in PCI config */
			pci_dev_writel(dev, PCI_BAR_ADDR(j), b->base);

			if (!p) {
				p++;
				PCIDBG("bus %02x device %02x function %d\n",
				       bus->busnum, PCI_SLOT(dev->devfn),
				       PCI_FUNC(dev->devfn));
			}
			PCIDBG("   bar %d: %x-%x [size=%s]\n", j,
			       b->base, b->limit, print_bytes(b->size));
		}
		
		/* run POST fixup handlers */
		pci_resource_fixup_post(dev, type);
	}

	DBGINDENT(-3);

	map->base = region_get_base(type);
	map->size = bus->set_resource_range(bus, type);

	return map->base;
}

uint32
pci_resource_cnb30le(pci_bus *bus, pci_bar_t type)
{
	pci_bar *map;
	pci_dev *dev;
	uint8 reg;
	uint32 sz;

	if (!bus || type == PCI_BAR_EMPTY)
		return 0;

	dev = bus->self;
	map = &bus->resource[type];

	if (!map || map->limit <= map->base)
		return 0;

	map->base = region_align(type, __pci_resource_align_bus[type]);
	sz = map->limit - map->base + 1;

	switch(type) {
	case PCI_BAR_PMMIO:
		/* set base and limit address */
		pci_dev_writew(dev, PCI_SERVERWORKS_PMMIO_BASE,
			       map->base >> 16);
		pci_dev_writew(dev, PCI_SERVERWORKS_PMMIO_LIMIT,
			       map->limit >> 16);

		/* enable resource and set prefetchable */
		reg = pci_dev_readb(dev, PCI_SERVERWORKS_PMRV);
		reg |= PCI_SERVERWORKS_PMMIO_EN;
		pci_dev_writeb(dev, PCI_SERVERWORKS_PMRV, reg);

		/* enable write-combine */
		reg = pci_dev_readb(dev, PCI_SERVERWORKS_WCCR);
		reg |= PCI_SERVERWORKS_PMMIO_WC;
		pci_dev_writeb(dev, PCI_SERVERWORKS_WCCR, reg);

		    /* chew out of ram */
		    /* 64M align to make things behave */
		if( mem_size > (map->base) )
		{
		    mem_size = map->base & ( ~ ( (64 << 20) -1 ) ); 
			/* adjust MULR */
		    pci_dev_writeb(dev, PCI_SERVERWORKS_MULR, (mem_size >> 24) & 0xff );
			/* shave off of the end of the last DIMM as well */
		    pci_dev_writeb(dev, PCI_SERVERWORKS_MRDRU7, (mem_size >> 24) & 0xff );
		}

		break;

	case PCI_BAR_MMIO:
		/* set base and limit address */
		pci_dev_writew(dev, PCI_SERVERWORKS_MMIO_BASE,
			       map->base >> 16);
		pci_dev_writew(dev, PCI_SERVERWORKS_MMIO_LIMIT,
			       map->limit >> 16);

		/* enable resource */
		reg = pci_dev_readb(dev, PCI_SERVERWORKS_PMRV);
		reg |= PCI_SERVERWORKS_MMIO_EN;
		pci_dev_writeb(dev, PCI_SERVERWORKS_PMRV, reg);

		/* enable write-combine */
		reg = pci_dev_readb(dev, PCI_SERVERWORKS_WCCR);
		reg |= PCI_SERVERWORKS_MMIO_WC;
		pci_dev_writeb(dev, PCI_SERVERWORKS_WCCR, reg);

		    /* chew out of ram */
		    /* 64M align to make things behave */
		if( mem_size > (map->base) )
		{
		    mem_size = map->base & ( ~ ( (64 << 20) -1 ) ); 
			/* adjust MULR */
		    pci_dev_writeb(dev, PCI_SERVERWORKS_MULR, (mem_size >> 24) & 0xff );
			/* shave off of the end of the last DIMM as well */
		    pci_dev_writeb(dev, PCI_SERVERWORKS_MRDRU7, (mem_size >> 24) & 0xff );
		    
		}

		break;

	case PCI_BAR_IO:
		/* set base and limit address */
		pci_dev_writew(dev, PCI_SERVERWORKS_IO_BASE,
			       map->base);
		pci_dev_writew(dev, PCI_SERVERWORKS_IO_LIMIT,
			       map->limit);

		/* set valid config */
		reg = pci_dev_readb(dev, PCI_SERVERWORKS_IORV);
		reg |= PCI_SERVERWORKS_IO_EN;
		pci_dev_writeb(dev, PCI_SERVERWORKS_IORV, reg);
		break;

	default:
		return 0;
	}

	PCIDBG("Host-bridge [%02x/%02x] %s range: %x-%x [size=%s]\n",
	       PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn),
	       region_get_name(type), map->base, map->limit,
	       print_bytes(sz));

	return sz;
}

uint32
pci_resource_bridge(pci_bus *bus, pci_bar_t type)
{
	pci_dev *dev;
	pci_bar *map;
	uint16 pcicmd;
	uint32 size;
	uint8 highmem;

	if (!bus || type == PCI_BAR_EMPTY)
		return 0;

	bridge = 1;
	dev = bus->self;
	map = &bus->resource[type];

	if (!map || map->limit <= map->base)
		size = 0;
	else {
		/* bridges impose alignment restrictions. */
		map->base = region_align(type,
					 __pci_resource_align_bridge[type]);
		size = (map->limit > map->base) ? map->limit - map->base : 0;
	}

	/* new value for COMMAND register */
	pcicmd = pci_dev_readw(dev, PCI_COMMAND);

	/* PCI-PCI bridge spec says to set
	 * map->base > map->limit if no passthru
	 */
	switch(type) {
	case PCI_BAR_IO:
		/* I/O ports */
		pci_dev_writeb(dev, PCI2PCI_IO_BASE,
			       size ? (map->base >> 8) & 0xf0 : 0xf0);
		pci_dev_writeb(dev, PCI2PCI_IO_LIMIT,
			       size ? (map->limit >> 8) & 0xf0 : 0);

		/* 32-bit I/O */
		highmem = pci_dev_readb(dev, PCI2PCI_IO_BASE);
		if (highmem & 0x01)
			pci_dev_writeb(dev, PCI2PCI_IO_BASE_UPPER, 0);
		highmem = pci_dev_readb(dev, PCI2PCI_IO_LIMIT);
		if (highmem & 0x01) 
			pci_dev_writeb(dev, PCI2PCI_IO_LIMIT_UPPER, 0);
		pcicmd |= PCI_CMD_IO;
		break;

	case PCI_BAR_MMIO:
		/* Non-prefetchable Memory */
		pci_dev_writew(dev, PCI2PCI_MMIO_BASE,
			       size ? (map->base >> 16) & 0xfff0 : 0xfff0);
		pci_dev_writew(dev, PCI2PCI_MMIO_LIMIT,
			       size ? (map->limit >> 16) & 0xfff0 : 0);
		/* no 64-bit MMIO */
		pcicmd |= PCI_CMD_MEMORY;
		break;

	case PCI_BAR_PMMIO:
		/* Prefetchable Memory */
		pci_dev_writew(dev, PCI2PCI_PMMIO_BASE,
			       size ? (map->base >> 16) & 0xfff0 : 0xfff0);
		pci_dev_writew(dev, PCI2PCI_PMMIO_LIMIT,
			       size ? (map->limit >> 16) & 0xfff0 : 0);

		/* 64-bit PMMIO */
		highmem = pci_dev_readb(dev, PCI2PCI_PMMIO_BASE);
		if (highmem & 0x01) 
			pci_dev_writeb(dev, PCI2PCI_PMMIO_BASE_UPPER, 0);
		highmem = pci_dev_readb(dev, PCI2PCI_PMMIO_LIMIT);
		if (highmem & 0x01) 
			pci_dev_writeb(dev, PCI2PCI_PMMIO_LIMIT_UPPER, 0);
		pcicmd |= PCI_CMD_MEMORY;
		break;

	default:
		return 0;
	}

	if (!size)
		return 0;

	pci_dev_writew(dev, PCI_COMMAND, pcicmd);
	PCIDBG("PCI-bridge [%02x/%02x/%02x] %s range: %x-%x"
	       " [size=%s]\n", bus->busnum, PCI_SLOT(dev->devfn),
	       PCI_FUNC(dev->devfn), region_get_name(type),
	       map->base, map->limit, print_bytes(size+1));

	return size+1;
}


static int cardbus_allocate(pci_bus *bus, pci_bar_t type, dword len,
			    int iobase, int iolimit,
			    int membase, int memlimit)
{
	pci_dev *dev;
	pci_bar *map;
	uint16 pcicmd;
	uint32 size;
	
	dev = bus->self;
	map = &bus->resource[type];

	map->base = region_allocate(type, len);
	map->base = region_align(type, __pci_resource_align_cardbus[type]);
	map->limit = map->base + len - 1;
	size = (map->limit > map->base) ? map->limit - map->base : 0;
	if (!size)
		return 0;

	/* new value for COMMAND register */
	pcicmd = pci_dev_readw(dev, PCI_COMMAND);

	switch (type) {
	case PCI_BAR_IO:
		/* I/O:
		 * 31 - 16: location of 64k page in 32-bit I/O space
		 * 15 - 0:  location of window within 64k page.
		 * because we only allocate within the first 64K,
		 * the top 16 bits are always zero even if a card
		 * supports 32-bit I/O space. */
		pci_dev_writel(dev, iobase, map->base  & 0xffff);
		pci_dev_writel(dev, iolimit, map->limit & 0xffff);
		pcicmd |= PCI_CMD_IO;
		break;

	case PCI_BAR_MMIO:
		/* MMIO:
		 * 31 - 12: memory window (4k aligned) in 
		 *          32-bit PCI space
		 * 11 - 0:  always 0s. */
		pci_dev_writel(dev, membase, map->base & 0xfffff000);
		pci_dev_writel(dev, memlimit, map->limit & 0xfffff000);
		pcicmd |= PCI_CMD_MEMORY;
		break;

	case PCI_BAR_PMMIO:
		/* MMIO:
		 * 31 - 12: memory window (4k aligned) in 
		 *          32-bit PCI space
		 * 11 - 0:  always 0s. */
		pci_dev_writel(dev, membase, map->base & 0xfffff000);
		pci_dev_writel(dev, memlimit, map->limit & 0xfffff000);
		pcicmd |= PCI_CMD_MEMORY;
		break;

	default:
		break;
	}

	pci_dev_writew(dev, PCI_COMMAND, pcicmd);
	PCIDBG("Cardbus-bridge [%02x/%02x/%02x] %s range: %x-%x"
	       " [size=%s]\n", bus->busnum, PCI_SLOT(dev->devfn),
	       PCI_FUNC(dev->devfn), region_get_name(type),
	       map->base, map->limit, print_bytes(size+1));
	return size + 1;
}


/* we don't look beyond the cardbus bridge. instead,
 * we statically allocate based upon the size given to use. */
uint32
pci_resource_cardbus(pci_bus *bus, pci_bar_t type)
{
	pci_bar *map;
	uint32 size = 0;

	if (!bus || type == PCI_BAR_EMPTY)
		return 0;

	map = &bus->resource[type];

	/* region 0 */
	if (map->size) 
		size = cardbus_allocate(bus, type, map->size,
					PCI2CB_IO_BASE0, PCI2CB_IO_LIMIT0,
					PCI2CB_MMIO_BASE0, PCI2CB_MMIO_LIMIT0);

	/* region 1 */
	if (map->size2) 
		size += cardbus_allocate(bus, type, map->size2,
					PCI2CB_IO_BASE1, PCI2CB_IO_LIMIT1,
					PCI2CB_MMIO_BASE1, PCI2CB_MMIO_LIMIT1);
	return size;
}

uint32
pci_resource_m1541(pci_bus *bus, pci_bar_t type )
{
	pci_dev *dev;
	pci_bar *map;
	uint16 pcicmd;
	uint32 size;

	if (!bus || type == PCI_BAR_EMPTY)
		return 0;

	dev = bus->self;
	map = &bus->resource[type];

	if (!map || map->limit <= map->base)
		size = 0;
	else 
		size = (map->limit > map->base) ? map->limit - map->base : 0;

	/* new value for COMMAND register */
	pcicmd = pci_dev_readw(dev, PCI_COMMAND);

	/* PCI-PCI bridge spec says to set
	 * map->base > map->limit if no passthru
	 */
	switch(type) {
	case PCI_BAR_IO:
		/* I/O ports */
		pcicmd |= PCI_CMD_IO;
		break;

	case PCI_BAR_MMIO:
		/* Non-prefetchable Memory */
		pcicmd |= PCI_CMD_MEMORY;
		break;

	case PCI_BAR_PMMIO:
		/* Prefetchable Memory */
		pcicmd |= PCI_CMD_MEMORY;
		break;

	default:
		return 0;
	}

	if (!size)
		return 0;

	pci_dev_writew(dev, PCI_COMMAND, pcicmd);
	PCIDBG("PCI-bridge [%02x/%02x/%02x] %s range: %x-%x"
	       " [size=%d]\n\r", bus->busnum, PCI_SLOT(dev->devfn),
	       PCI_FUNC(dev->devfn), region_get_name(type),
	       map->base, map->limit, size+1);

	return size+1;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
