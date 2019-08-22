/* $Id: pci.c,v 1.2 2003/12/11 00:57:09 thockin Exp $
 * Copyright 2001 Sun Microsystems, Inc.
 * All rights reserved.
 *
 *   Erik Gilling (gilling@sun.com)
 *   Tim Hockin (thockin@sun.com)
 *   Duncan Laurie (void@sun.com)
 *
 * FIXME: we need to support expansion ROMS
 * FIXME: we need to support BIST
 * FIXME: do we need to support capabilities?
 * FIXME: we need to handle MIN_GNT/MAX_LAT -> latency timers better
 */

#include <printf.h>

#include "common/pci.h"
#include "common/pci_regs.h"
#include "common/pci_fixup.h"
#include "common/pci_resource.h"
#include "common/alloc.h"
#include "common/memory.h"
#include "common/systype.h"
#include "common/cmos.h"
#include "common/rom.h"
#include "common/pirq.h"

static void pci_dev_init(pci_dev *);
static void pci_bridge_init(pci_bus *, byte);
static void pci_cardbus_init(pci_bus *);
static void dump_bus(pci_bus *);
static pci_bus *find_bus(byte, pci_bus *);
static const char *pci_lookup_dev_name(pci_dev *);

/* global list of all pci devices */
pci_dev *pci_devices;
static pci_dev *pci_devices_tail;

/* file global root of pci bus tree */
static pci_bus _pci_root_struct;
static pci_bus *pci_root = &_pci_root_struct;

/* cardbus devices. we only use this for legacy addresses 
 * we need to allocate addresses based on a per device basis
 * rather than on a per logical function basis. 
 * start address: 0x3e0 (even aligned) */
static uint16 cardbus_legacy_bar = 0x3e0;
/*static byte vga = 0;*/

#if DEBUG_PCI
int dbgindent;
#endif

/* 
 * map pci vendor/device id to name 
 */
#define DEVNAME(vid,did,name)	{ PCI_DEVICE(vid,did), (name) }
static struct pci_dev_name {
	word vendor;
	word device;
	const char *name;
} pci_dev_names[] = {
	DEVNAME(AL, M1541, "Acer Labs M1541 Aladdin V Host Bridge"),
	DEVNAME(AL, M1533, "Acer Labs M1543 Aladdin V PCI-ISA Bridge"),
	DEVNAME(AL, M5229, "Acer Labs M5229 TXpro IDE Controller"),
	DEVNAME(AL, M5243, "Acer Labs M5243 AGP Controller"),
	DEVNAME(AL, M5237, "Acer Labs M5237 USB Controller"),
	DEVNAME(AL, M7101, "Acer Labs M7101 PMU"),
	DEVNAME(INTEL, 82557, "Intel 82557 EEPro100 Fast Ethernet"),
	DEVNAME(INTEL, 82559ER, "Intel 82559ER EEPro100 Fast Ethernet"),
	DEVNAME(NS, 83815, "National DP83815 MacPhyter Ethernet"),
	DEVNAME(SERVERWORKS, LE, "ServerWorks CNB30LE Host Bridge"),
	DEVNAME(SERVERWORKS, OSB4, "ServerWorks OSB4 PCI-ISA Bridge"),
	DEVNAME(SERVERWORKS, OSB4IDE, "ServerWorks OSB4 IDE Controller"),
	DEVNAME(SERVERWORKS, USB, "ServerWorks OpenHCI USB Controller"),
	DEVNAME(SERVERWORKS, CSB5, "ServerWorks CSB5 South Bridge"),
	DEVNAME(SERVERWORKS, CSB5IDE, "ServerWorks CSB5 IDE Controller"),
	DEVNAME(SERVERWORKS, CSB5LPC, "ServerWorks CSB5 PCI-LPC Bridge"),
	DEVNAME(HIGHPOINT, HPT370, "HighPoint HPT370 ATA-100 Controller"),
	DEVNAME(SYMBIOS, 53C875, "Symbios Logic SYM53C875 SCSI Controller"),
	DEVNAME(SYMBIOS, 53C1010, "LSI Logic LSI53C1010 SCSI Controller"),
	DEVNAME(SYMBIOS, 53C1010_66, "LSI Logic LSI53C1010-66 SCSI Controller"),
	DEVNAME(DEC, 21152, "DEC 21152 PCI-PCI bridge"),
	DEVNAME(CHRYSALIS, LUNA340, "Chrysalis Luna340"),
	DEVNAME(TIGERJET, TIGER600, "TigerJet Tiger600 PCI-PCI bridge"),
	{ 0, 0, NULL }
};

/* 
 *   initialize the pci device and bus trees
 */
void 
pci_init(void)
{
#if DEBUG_PCI
	dbgindent=0;
	printf("\n");
#endif
	pirq_init();

	pci_devices = NULL;

	pci_bus_blank(pci_root);
	pci_root->root = 1;
	pci_bus_scan(pci_root);

	pci_root->sub = pci_root->last_kid;

	pci_resource_init(pci_root);

#if DEBUG_PCI
	printf("PCI init: ");
#endif
}

static pci_bus *pci_bridge_bus_alloc(pci_dev *newdev, uint32 
				     (*fcn)(pci_bus *, pci_bar_t))
{
	pci_bus *newbus;

	/* get memory for pci bus structure */
	newbus = malloc(sizeof(*newbus));
	if (!newbus) {
		printf("malloc failed at %s:%d\n", 
		       __FILE__, __LINE__);
		return NULL;
	}
	pci_bus_blank(newbus);
	
	/* fill in the bus structure */
	newbus->self = newdev;
	newbus->busnum = ++newdev->bus->last_kid;
	newbus->last_kid = newbus->busnum;
	newbus->pri = newdev->bus->busnum;
	newbus->sub = 0xff;
	newbus->fbbe = (pci_dev_readb(newdev, PCI2PCI_STATUS) & 
			PCI_STATUS_FBBE);
	newbus->set_resource_range = fcn;
	return newbus;
}

/*
 * scan pci bus "bus" for devices
 * and recursively initialize any found devices/bridges
 *
 * inputs: pci bus
 */
void
pci_bus_scan(pci_bus *bus)
{
	word devfn; /* word so that we can get all the way to 0xff */
	byte multifunc = 0;
	pci_bus *bus_kid_tail = NULL;
	pci_dev *bus_dev_tail = NULL;

	PCIDBG("Scanning PCI bus %d\n", bus->busnum);
	DBGINDENT(3);

	/* for every device/function on this bus */
	for (devfn = 0; devfn <= 0xff; devfn++) {
		pci_dev *newdev;
		pci_bus *newbus;
		word x;

		/* skip functions if we are not a multifunction device */
		if (!multifunc && PCI_FUNC(devfn)) {
			continue;
		}

		/* skip invalid devices (bad vendor ids) */
		x = pci_read_config_word(bus->busnum, devfn, PCI_VENDOR_ID);
		if (x == 0xffff || x == 0x0000) {
			continue;
		}

		/* create a new pci device */
		newdev = malloc(sizeof(*newdev));
		if (!newdev) { 
			printf("malloc failed at %s:%d\n", 
				__FILE__, __LINE__);
			continue;
		}
		pci_dev_blank(newdev);

		/* fill in the new structure */
		newdev->devfn = devfn;
		newdev->vendor = x;
		newdev->device = pci_readw(bus->busnum, devfn, PCI_DEVICE_ID);
		newdev->htype = pci_readb(bus->busnum, devfn, PCI_HTYPE);

		/* MSB of primary devfns is set for multifunc devs (pci2.1) */
		newdev->multifunc = newdev->htype & 0x80;
		newdev->htype &= ~0x80;

		/* LSB of class is program interface - we don't care */
		newdev->class = pci_readl(bus->busnum, devfn, 
			PCI_CLASS_ID) >> 16;
		newdev->revision = pci_readb(bus->busnum, devfn, 
			PCI_REVISION_ID);
		bus->fbbe = (pci_readb(bus->busnum, devfn, PCI_STATUS) 
			& PCI_STATUS_FBBE) && bus->fbbe;

		newdev->name = pci_lookup_dev_name(newdev);
		newdev->bus = bus;

		PCIDBG("Found device %02x:%02x %04x:%04x %s\n",
			PCI_SLOT(devfn), PCI_FUNC(devfn),
			newdev->vendor, newdev->device, newdev->name);

		DBGINDENT(3);

		/* 
		 * add the new device to the proper lists 
		 */

		/* if it is the first device, there is no tail yet */
		if (!pci_devices) {
			pci_devices = newdev;
		} else {
			pci_devices_tail->allnext = newdev;
		}
		pci_devices_tail = newdev;

		/* if it is the first device on the bus, no tail yet */
		if (!bus->devices) {
			bus->devices = newdev;
		} else {
			bus_dev_tail->busnext = newdev;
		}
		bus_dev_tail = newdev;
		
		/* set the multifunction flag */
		if (PCI_FUNC(devfn) == 0) {
			multifunc = newdev->multifunc;
			if (multifunc) {
				PCIDBG("Multi-function\n");
			}
		}

		/* do any pre-config fixups */
		if (pci_fixup_pre(newdev) != 0) {
			/* we're done, says the fixup routine */
			goto scan_continue;
		}

		/* initialize known bridge types */
		if ((newdev->htype == PCI_HTYPE_PCI_BRIDGE) ||
		    (newdev->htype == PCI_HTYPE_CARDBUS_BRIDGE)) {
			if (newdev->htype == PCI_HTYPE_PCI_BRIDGE) {
				PCIDBG("PCI-PCI bridge\n"); 
				newbus = pci_bridge_bus_alloc(newdev,
							      pci_resource_bridge);
			 } else {
				 PCIDBG("PCI-Cardbus bridge\n"); 
				 newbus = pci_bridge_bus_alloc(newdev,
							       pci_resource_cardbus);
			 }
			
			if (!newbus)
				goto scan_continue;

			/* append onto bus tree */
			newbus->parent = bus;
			if (!bus->kids) {
				bus->kids = newbus;
			} else {
				bus_kid_tail->peer = newbus;
			}
			bus_kid_tail = newbus;
			
			/* probe behind the bus */
			pci_bridge_init(newbus, newdev->htype);
			bus->last_kid = newbus->last_kid;
		}

		/* configure this device - even bridges */
		pci_dev_init(newdev);

		/* do any post-config fixups */
		pci_fixup_post(newdev);

scan_continue:
		DBGINDENT(-3);
	}

	/* if all devices on this bus are Fast Back-to-Back capable... */
	if (bus->fbbe) {
		pci_dev *p = bus->devices;
		word data;

		PCIDBG("Enabling Fast Back-to-Back transactions\n");
		while (p) {
			data = pci_dev_readw(p, PCI_COMMAND);
			data |= PCI_CMD_FBBE;
			pci_dev_writew(p, PCI_COMMAND, data);
			p = p->busnext;
		}

		/* host bridges won't have bus->parent */
		if (bus->parent) {
			p = bus->self;
			data = pci_dev_readw(p, PCI2PCI_BRIDGE_CTRL);
			data |= PCI2PCI_BCTL_FBBE;
			pci_dev_writew(p, PCI2PCI_BRIDGE_CTRL, data);
		}				
	}
	DBGINDENT(-3);
}

static void
pci_bridge_init(pci_bus *bridge, byte type)
{
	word reg;

	if (!bridge)
		return;

	DBGINDENT(3);

	/* disable IO and memory pass-through on bridge */
	reg = pci_dev_readw(bridge->self, PCI_COMMAND);
	reg &= ~(PCI_CMD_IO | PCI_CMD_MEMORY);
	pci_dev_writew(bridge->self, PCI_COMMAND, reg);

	/* reset status register */
	reg = pci_dev_readw(bridge->self, PCI_STATUS);
	pci_dev_writew(bridge->self, PCI_STATUS, reg);

	/* write current bus config to enable probing */
	pci_dev_writeb(bridge->self, PCI2PCI_BUS_PRI, bridge->pri);
	pci_dev_writeb(bridge->self, PCI2PCI_BUS_SEC, bridge->busnum);
	pci_dev_writeb(bridge->self, PCI2PCI_BUS_SUB, 0xff);
	
	switch (type) {
	case PCI_HTYPE_PCI_BRIDGE: 
		/* recursivley scan the new bus */
		pci_bus_scan(bridge);
		break;

	case PCI_HTYPE_CARDBUS_BRIDGE:
		pci_cardbus_init(bridge);
		break;

	default:
	}
	
	/* set proper subordinate number */
	bridge->sub = bridge->last_kid;
	pci_dev_writeb(bridge->self, PCI2PCI_BUS_SUB, bridge->sub);

	PCIDBG("Subordinate bus: %d\n", bridge->sub);

	DBGINDENT(-3);
}


/* hardcode cardbus bars based upon what windows xp does 
 * 4M (XP uses a 4K one) memory window/64MB memory window
 * 256-byte I/O window/ 256-byte I/O window
 */
static void
pci_cardbus_init(pci_bus *bus)
{
	/* initialize io/mem regions to zero. this tells the
	 * Cardbus bridge not to pass through any transactions. */
	pci_dev_writel(bus->self, PCI2CB_MMIO_BASE0, 0);
        pci_dev_writel(bus->self, PCI2CB_MMIO_LIMIT0, 0);
        pci_dev_writel(bus->self, PCI2CB_MMIO_BASE1, 0);
        pci_dev_writel(bus->self, PCI2CB_MMIO_LIMIT1, 0);
        pci_dev_writel(bus->self, PCI2CB_IO_BASE0, 0);
        pci_dev_writel(bus->self, PCI2CB_IO_LIMIT0, 0);
        pci_dev_writel(bus->self, PCI2CB_IO_BASE1, 0);
        pci_dev_writel(bus->self, PCI2CB_IO_LIMIT1, 0);

	bus->resource[PCI_BAR_IO].type = PCI_BAR_IO;
	bus->resource[PCI_BAR_IO].size = 0x100;
	bus->resource[PCI_BAR_IO].size2 = 0x100;

	/* first window is prefetchable. second isn't. */
	bus->resource[PCI_BAR_PMMIO].type = PCI_BAR_PMMIO;
	bus->resource[PCI_BAR_PMMIO].size = 0x400000;
	bus->resource[PCI_BAR_MMIO].type = PCI_BAR_MMIO;
	bus->resource[PCI_BAR_MMIO].size2 = 0x4000000;
}

/*
 * setup/initialize pci device
 * IRQ, cacheline size, latency, etc.
 */
static void
pci_dev_init(pci_dev *dev)
{
	uint32 size;
	byte pin;
	word cmd;

	/* PCI 2.1 says intpin = 0 if dev uses no IRQ */
	if (0 >= pci_irq_fixup_pre(dev)) {

		pin = pci_dev_readb(dev, PCI_INTERRUPT_PIN);
		dev->irq = pirq_route_pci(dev, pin);

		if (dev->irq) {
			PCIDBG("PCI->ISA pin%d IRQ mapping -> %d\n",
			       pin, dev->irq);

			/* write interrupt vector */
			pci_dev_writeb(dev, PCI_INTERRUPT_LINE, dev->irq);

			/* check to see if IRQ assignment was successful */
			pin = pci_dev_readb(dev, PCI_INTERRUPT_LINE);
			if (pin != dev->irq) {
#if DEBUG_PCI
				PCIDBG("set interrupt %d failed, device read %d\n",
				       dev->irq, pin);
#else
				printf("PCI: %02x:%02x:%02x set interrupt %d failed, "
				       "device read %d\n",
				       dev->bus->busnum, PCI_SLOT(dev->devfn),
				       PCI_FUNC(dev->devfn), dev->irq, pin);
#endif
				dev->irq = pin;
			}
		}
		else {
			/* PCI-PCI bridge spec says write 0xff */
			pci_dev_writeb(dev, PCI_INTERRUPT_LINE, 0xff);
 		}
	}

	/* run post-cfg irq_fixup() if it exists */
	pci_irq_fixup_post(dev);

	/* set the cache line size - both 3k and 5k have 32 byte lines */
	pci_dev_writeb(dev, PCI_CACHE_LINE_SIZE, 8);

	/* enable everything in COMMAND , except IO, MEM, VGA and FBBE */
	pci_dev_writew(dev, PCI_COMMAND, 
		       ~(PCI_CMD_IO | PCI_CMD_VGA_PALETTE |
			 PCI_CMD_MEMORY | PCI_CMD_FBBE));

	/* test for busmastering */
	cmd = pci_dev_readw(dev, PCI_COMMAND);
	dev->master = ((cmd & PCI_CMD_MASTER) == PCI_CMD_MASTER) ? 1 : 0;

	/* set max latency if bus master enabled */
	if (dev->master) {
		PCIDBG("Master capable\n");
		if (pci_dev_readb(dev, PCI_LATENCY_TIMER) < 32) {
			pci_dev_writeb(dev, PCI_LATENCY_TIMER, 64);
		}
	}

	/* set resources */
	switch (dev->htype) {
	case PCI_HTYPE_STANDARD:
		/* calc size and then zero out expansion rom register */
		pci_dev_write32(dev, PCI_EXPANSION_ROM, ~0);
		size = pci_dev_read32(dev, PCI_EXPANSION_ROM);
		if (size) {
			size = ~(size & 0xfffff800) + 1;
			PCIDBG("Expansion ROM [size=%s] (disabled)\n",
			       print_bytes(size)); 
		}
		pci_dev_write32(dev, PCI_EXPANSION_ROM, 0);
	
		/* standard devices have 6 BARs */
		pci_bar_scan(dev, 6);
		break;

	case PCI_HTYPE_PCI_BRIDGE:
		/* calc size and then zero out expansion rom register */
		pci_dev_write32(dev, PCI2PCI_EXPANSION_ROM, ~0);
		size = pci_dev_read32(dev, PCI2PCI_EXPANSION_ROM);
		if (size) {
			size = ~(size & 0xfffff800) + 1;
			PCIDBG("Expansion ROM [size=%s] (disabled)\n",
			       print_bytes(size)); 
		}
		pci_dev_write32(dev, PCI2PCI_EXPANSION_ROM, 0);

		/* PCI-PCI bridges have some special extra registers */
		pci_dev_writew(dev, PCI2PCI_BRIDGE_CTRL, 
			       ~(PCI2PCI_BCTL_ISA
				 | PCI2PCI_BCTL_VGA
				 | PCI2PCI_BCTL_RESET
				 | PCI2PCI_BCTL_FBBE));
		if (pci_dev_readb(dev, PCI2PCI_LATENCY) < 32) {
			pci_dev_writeb(dev, PCI2PCI_LATENCY, 64);
		}

		/* PCI-PCI bridges only have 2 BARs */
		pci_bar_scan(dev, 2);
		break;

	case PCI_HTYPE_CARDBUS_BRIDGE:
		/* PCI-Cardbus bridges have some special extra registers */
		pci_dev_writew(dev, PCI2CB_BRIDGE_CTRL, 
			       ~(PCI2CB_BCTL_ISA 
				 | PCI2CB_BCTL_VGA
				 | PCI2CB_BCTL_RESET 
				 | PCI2CB_BCTL_PREFETCH1
				 | PCI2CB_BCTL_INTR)	
			       | PCI2CB_BCTL_PREFETCH0);

		/* legacy address is allocated on a per-chip and not
		 * on a per-function basis */
		pci_dev_writel(dev, PCI2CB_16BIT_LEGACY_BAR, 
			       cardbus_legacy_bar);
		if (!dev->multifunc) 
			cardbus_legacy_bar += 2;

		if (pci_dev_readb(dev, PCI2PCI_LATENCY) < 32) {
			pci_dev_writeb(dev, PCI2PCI_LATENCY, 64);
		}

		/* PCI-Cardbus bridges have 1 BAR */
		pci_bar_scan(dev, 1);
		break;

	default:
		/* unknown header type */
		PCIDBG("Unsupported header type: 0x%02x\n", dev->htype);
	}
}

/*
 * calculate size of required pci device resources
 */
void
pci_bar_scan(pci_dev *dev, int nbars)
{
	int has_mem=0, has_io=0, cur;
	
	/* max of 6 BARs */
	nbars = (nbars > 6) ? 6 : nbars;

	/* start by zeroing all the BARs */
	for (cur=0; cur<nbars; cur++)
		pci_dev_write32(dev, PCI_BAR_ADDR(cur), 0);

	for (cur=0; cur<nbars; cur++)	{
		pci_bar *bar = &dev->bar[cur];
		uint32 val;

		/* first write all 1s to the BAR,
		 * then read it back to find capabilities */
		pci_dev_write32(dev, PCI_BAR_ADDR(cur), ~0);
		val = pci_dev_read32(dev, PCI_BAR_ADDR(cur));

		if (val & PCI_BAR_IS_IO) {
			/* IO resource (32-bit) */
			has_io = PCI_CMD_IO;
			bar->type = PCI_BAR_IO;

			/* determine requested region size */
			bar->size = (~(val & 0xfffffffc) & 0xff) + 1;

			/* PCI spec says 256 bytes is max */
			if (bar->size > 256) bar->size = 256;
		}
		else if (val & PCI_BAR_IS_LOWMEM) {
			/* memory mapped to low 1M is unsupported (legacy) */
			bar->type = PCI_BAR_EMPTY;
		}
		else if (val) {
			/* Memory resource */
			has_mem = PCI_CMD_MEMORY;

			/* Prefetchable [bit 3 is set]
		         *  ~ no side effects on reads,
			 *  ~ returns all on reads regardless of byte enables,
			 *  ~ host bridge can merge processor writes in region
			 */
			bar->type = (val & PCI_BAR_IS_PREFETCH) ?
				PCI_BAR_PMMIO : PCI_BAR_MMIO;

			/* determine 32-bit region size */
			bar->size = ~(val & 0xfffffff0) + 1;

			/* 64-bit address [bit 2 is set]
			 *  ~ x86 does not support 64-bit addressing,
			 *  ~ clear high 32 bits and skip next BAR
			 */
			if (val & PCI_BAR_IS_64BIT) {
				PCIDBG("BAR %d: requested 64-bit\n", cur);
				cur++;
				bar->addr64 = 1;
				pci_dev_write32(dev, PCI_BAR_ADDR(cur), 0);
			}
		}
		else {
			bar->type = PCI_BAR_EMPTY;
		}
	}
    
	/* enable IO and/or MMIO in command register */
	if (has_io || has_mem) {
		uint16 data;
		data = pci_dev_read16(dev, PCI_COMMAND);
		pci_dev_write16(dev, PCI_COMMAND, data | has_mem | has_io);
		PCIDBG("%s%s address space enabled\n",
		       has_io ? has_mem ? "I/O & " : "I/O" : "",
		       has_mem ? "Memory" : "");
	}
}

/*
 *   find a pci device "devfn" on bus "bus"
 *
 *   inputs: bus number,
 *           device/func number
 *   return: pci device,
 *           or null if not found
 */
pci_dev *
pci_lookup_dev_by_bus(byte bus, byte devfn)
{
        pci_dev *dev;

        for (dev=pci_devices; dev; dev=dev->allnext) {
                if (dev->bus->busnum == bus && dev->devfn == devfn)
                        break;
	}

        return dev;
}

/* pci_lookup_dev()
 *
 *   find a pci device by vendor/device ID
 *   begin search in tree from pci device "start"
 *
 *   inputs: vendor id,
 *           device id,
 *           starting pci device
 *   return: pci device,
 *           or null if not found
 */
pci_dev *
pci_lookup_dev(word vendor, word device, pci_dev *start)
{
	start = (start) ? start->allnext : pci_devices;
        while (start &&
	       (start->vendor != vendor ||
		start->device != device)) {
                start = start->allnext;
	}
        return start;
}

/* pci_lookup()
 *
 *   find a pci device by vendor/device ID
 *
 *   inputs: vendor id, device id
 *   return: pci device,
 *           or null if not found
 */
pci_dev *
pci_lookup(word vendor, word device)
{
	return pci_lookup_dev(vendor, device, NULL);
}

/* pci_lookup_bus()
 *
 *   find a pci bus by number
 *
 *   inputs: bus number
 *   return: pci bus,
 *           or null if not found
 */
pci_bus *
pci_lookup_bus(byte num)
{
	return find_bus(num, pci_root);
}

/* recursive tree walker for searching for a bus */
static pci_bus *
find_bus(byte num, pci_bus *root)
{
	pci_bus *bus = root;
	pci_bus *result = NULL;

	/* for each bus on this level */
	while (bus) {
		/* short circuit, if we can */
		if (bus->busnum == num)
			return bus;

		/* depth-first */
		if (bus->kids) {
			result = find_bus(num, bus->kids);
			if (result)
				return result;
		}

		/* next on this level */
		bus = bus->peer;
	}

	return NULL;
}

/* pci_lookup_dev_name()
 *
 *   find a pci device name if it exists
 *
 *   inputs: pci device
 *   return: string containing pci device name,
 *           or "unknown device" if not found
 */
static const char *
pci_lookup_dev_name(pci_dev *dev)
{
	struct pci_dev_name *idlist = pci_dev_names;

	while (idlist->name != NULL) {
		if (dev->vendor == idlist->vendor 
		 && dev->device == idlist->device) {
			return idlist->name;
		}
		idlist++;
	}

	return "unknown device";
}

void pci_list(void)
{
	pci_bus *b = pci_root;

	while (b) {
		dump_bus(b);
		b = b->peer;
	}
}

static void
dump_bus(pci_bus *bus)
{
	pci_dev *d = bus->devices;
	pci_bus *b = bus->kids;
	
	DBGINDENT(3);

	if (!bus->parent) {
		DPRINTF("  Host Bus: %d (device %02x:%02x) [%dMHz]\n", 
			bus->busnum, PCI_SLOT(bus->self->devfn),
			PCI_FUNC(bus->self->devfn), bus->freq ? 66 : 33);
	}
	else {
		DPRINTF("  Bridged Bus: %d (bridge: %02x:%02x:%02x)\n", 
			bus->busnum, bus->parent->busnum,
			PCI_SLOT(bus->self->devfn), 
			PCI_FUNC(bus->self->devfn));
	}

	DBGINDENT(3);

	while (d) {
		DPRINTF("    Device: %02x:%02x %04x:%04x %s",
			PCI_SLOT(d->devfn), PCI_FUNC(d->devfn), 
			d->vendor, d->device, d->name);

		if (d->irq && d->irq < 0xff)
			DPRINTF(" (IRQ %d)", d->irq);

		DPRINTF("\n");
		d = d->busnext;
	}

	DBGINDENT(-3);
	
	while (b) {
		dump_bus(b);
		b = b->peer;
	}

	DBGINDENT(-3);
}

void
pci_dev_blank(pci_dev *dev) 
{
	int i;

	dev->vendor = dev->device = 0;
	dev->class = dev->revision = 0;
	dev->htype = dev->devfn = dev->irq = 0;
	dev->master = dev->multifunc = 0;

	dev->name = NULL;
	dev->bus = NULL;
	dev->allnext = dev->busnext = NULL;

	/* clear BARs */
	for (i=0; i<6; i++) {
		dev->bar[i].type   = PCI_BAR_EMPTY;
		dev->bar[i].dev    = dev;
		dev->bar[i].base   = 0;
		dev->bar[i].limit  = 0;
		dev->bar[i].size   = 0;
		dev->bar[i].addr64 = 0;
	}		
}

void
pci_bus_blank(pci_bus *bus) 
{
	int i;

	memset(bus, 0, sizeof(pci_bus));
	for (i=0; i<4; i++)
		bus->resource[i].type   = i;
}

void pci_set_hole( void )
{
    if( systype_is_5k() )
    {
	pci_write_config_byte( 0, PCI_DEVFN(0,0), PCI_SERVERWORKS_MHUR1, 
			       0x00 ); /* reset the valid bit */


	    /* make a hole from the current mem_size to the top of memory */
	pci_write_config_byte( 0, PCI_DEVFN(0,0), PCI_SERVERWORKS_MHLR0, 
			       (mem_size>>20) & 0xff );
	pci_write_config_byte( 0, PCI_DEVFN(0,0), PCI_SERVERWORKS_MHLR1, 
			       (mem_size >> 28) & 0x0f );

	pci_write_config_byte( 0, PCI_DEVFN(0,0), PCI_SERVERWORKS_MHUR0, 
			        0xff );
	pci_write_config_byte( 0, PCI_DEVFN(0,0), PCI_SERVERWORKS_MHUR1, 
			       0x1f ); /* set the valid bit */

    }
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
