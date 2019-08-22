/* $Id: pci_fixup.c,v 1.2 2003/10/06 15:15:03 iceblink Exp $
 *
 * Copyright 2000 Cobalt Networks, Inc.
 * Copyright 2001 Sun Microsystems, Inc.
 */

#include <printf.h>

#include "common/pci.h"
#include "common/pci_regs.h"
#include "common/pci_fixup.h"
#include "common/pci_resource.h"
#include "common/pirq.h"
#include "common/region.h"
#include "common/alloc.h"
#include "common/rom.h"
#include "common/cmos.h"
#include "common/mptable.h"
#include "common/ioapic.h"
#include "common/acpi.h"

static struct pci_fixup_t *find_fixup(pci_dev *dev);

static int cnb30le_fixup_pre(pci_dev *dev);
static int cnb30le_fixup_post(pci_dev *dev);
static int svwks_usb_irq_fixup(pci_dev *dev);
static int svwks_ide_irq_fixup(pci_dev *dev);
static int osb4_acpi_fixup(pci_dev *dev);
static int osb4_acpi_io_fixup(pci_dev *dev);
static int osb4_ide_fixup(pci_dev *dev);
static int csb5_ide_fixup(pci_dev *dev);
static int csb5_lpc_irq_fixup(pci_dev *dev);
static int sym53c875_fixup(pci_dev *dev);
static int chrysalis_luna340_fixup(pci_dev *dev);
static int m1541_fixup(pci_dev *dev);
static int m7101_fixup(pci_dev *dev);
static int m5237_fixup(pci_dev *dev);
static int m5229_fixup(pci_dev *dev);

/*
 * This provides a mechanism to catch 'broken' devices and fix them up.
 * There are two hooks - one for before the device has been configured, and
 * one for afterwards.  If a pre-config fixup returns non-zero, the
 * configuration for that device WILL NOT be done.  This is not a place to put
 * lengthy device-specific initializations, this is a place to fix things
 * general probing may have gotten wrong.
 */
struct pci_fixup_func {
	int (*dev_fixup)(pci_dev *);
	int (*irq_fixup)(pci_dev *);
	int (*io_fixup)(pci_dev *);
	int (*mmio_fixup)(pci_dev *);
	int (*pmmio_fixup)(pci_dev *);
};
struct pci_fixup_t {
	word vendor;
	word device;
	struct pci_fixup_func pre;
	struct pci_fixup_func post;
};

/* the table of device fixups */
static struct pci_fixup_t fixup_table[] = {
	{ PCI_DEVICE(SERVERWORKS, LE),
	  { cnb30le_fixup_pre, NULL, NULL, NULL, NULL },
	  { cnb30le_fixup_post, NULL, NULL, NULL, NULL },
	},
	{ PCI_DEVICE(SERVERWORKS, OSB4),
	  { NULL, NULL, NULL, NULL, NULL },
	  { osb4_acpi_fixup, NULL, osb4_acpi_io_fixup, NULL, NULL },
	},
	{ PCI_DEVICE(SERVERWORKS, OSB4IDE),
	  { osb4_ide_fixup, NULL, NULL, NULL, NULL },
	  { NULL, svwks_ide_irq_fixup, NULL, NULL, NULL },
	},
	{ PCI_DEVICE(SERVERWORKS, USB),
	  { NULL, NULL, NULL, NULL, NULL },
	  { NULL, svwks_usb_irq_fixup, NULL, NULL, NULL },
	},
	{ PCI_DEVICE(SERVERWORKS, CSB5),
	  { NULL, NULL, NULL, NULL, NULL },
	  { osb4_acpi_fixup, NULL, osb4_acpi_io_fixup, NULL, NULL },
	},
	{ PCI_DEVICE(SERVERWORKS, CSB5IDE),
	  { csb5_ide_fixup, NULL, NULL, NULL, NULL },
	  { NULL, svwks_ide_irq_fixup, NULL, NULL, NULL },
	},
	{ PCI_DEVICE(SERVERWORKS, CSB5LPC),
	  { csb5_lpc_irq_fixup, NULL, NULL, NULL, NULL },
	  { NULL, NULL, NULL, NULL, NULL },
	},
	{ PCI_DEVICE(AL, M1541),
	  { NULL, NULL, NULL, NULL, NULL },
	  { m1541_fixup, NULL, NULL, NULL, NULL },
	},
	{ PCI_DEVICE(AL, M7101),
	  { NULL, NULL, m7101_fixup, NULL, NULL },
	  { NULL, NULL, NULL, NULL, NULL },
	},
	{ PCI_DEVICE(AL, M5229),
	  { NULL, NULL, NULL, NULL, NULL },
	  { NULL, m5229_fixup, NULL, NULL, NULL },
	},
	{ PCI_DEVICE(AL, M5237),
	  { NULL, m5237_fixup, NULL, NULL, NULL },
	  { NULL, NULL, NULL, NULL, NULL },
	},
	{ PCI_DEVICE(SYMBIOS, 53C875),
	  { sym53c875_fixup, NULL, NULL, NULL, NULL },
	  { NULL, NULL, NULL, NULL, NULL },
	},
	{ PCI_DEVICE(CHRYSALIS, LUNA340),
	  { NULL, NULL, NULL, NULL, NULL },
	  { chrysalis_luna340_fixup, NULL, NULL, NULL, NULL },
	},
	{ 0, 0,
	  { NULL, NULL, NULL, NULL, NULL },
	  { NULL, NULL, NULL, NULL, NULL },
	},
};

int
pci_fixup_pre(pci_dev *dev)
{
	struct pci_fixup_t *f;
	int ret = 0;
	
	f = find_fixup(dev);
	if (f && f->pre.dev_fixup) {
		PCIDBG("running pre-cfg device fixup <%p>\n",
		       f->pre.dev_fixup);
		DBGINDENT(3);
		ret = f->pre.dev_fixup(dev);
		DBGINDENT(-3);
		return ret;
	}
	return ret;
}

int
pci_fixup_post(pci_dev *dev)
{
	struct pci_fixup_t *f;
	int ret = 0;

	f = find_fixup(dev);
	if (f && f->post.dev_fixup) {
		PCIDBG("running post-cfg device fixup %p\n",
		       f->post.dev_fixup);
		DBGINDENT(3);
		ret = f->post.dev_fixup(dev);
		DBGINDENT(-3);
	}
	return ret;
}

int
pci_irq_fixup_pre(pci_dev *dev)
{
	struct pci_fixup_t *f;
	int ret = 0;

	f = find_fixup(dev);
	if (f && f->pre.irq_fixup) {
		PCIDBG("running pre-cfg device irq fixup %p\n",
		       f->pre.irq_fixup);
		DBGINDENT(3);
		ret = f->pre.irq_fixup(dev);
		DBGINDENT(-3);
	}
	return ret;
}

int
pci_irq_fixup_post(pci_dev *dev)
{
	struct pci_fixup_t *f;
	int ret = 0;

	f = find_fixup(dev);
	if (f && f->post.irq_fixup) {
		PCIDBG("running post-cfg device irq fixup %p\n",
		       f->post.irq_fixup);
		DBGINDENT(3);
		ret = f->post.irq_fixup(dev);
		DBGINDENT(-3);
	}
	return ret;
}

int
pci_resource_fixup_pre (pci_dev *dev, pci_bar_t type)
{
	struct pci_fixup_t *f;
	int (*pre_fix)(pci_dev *);
	int ret = 0;

	f = find_fixup(dev);
	if (!f) return 0;

	switch (type)
	{
	case PCI_BAR_IO:    pre_fix = f->pre.io_fixup;    break;
	case PCI_BAR_MMIO:  pre_fix = f->pre.mmio_fixup;  break;
	case PCI_BAR_PMMIO: pre_fix = f->pre.pmmio_fixup; break;
	default: return 0;
	}

	if (pre_fix) {
		PCIDBG("running pre-cfg %s fixup %p\n",
		       region_get_name(type), pre_fix);
		DBGINDENT(3);
		ret = pre_fix(dev);
		DBGINDENT(-3);
	}
	return ret;
}

int
pci_resource_fixup_post (pci_dev *dev, pci_bar_t type)
{
	struct pci_fixup_t *f;
	int (*post_fix)(pci_dev *);
	int ret = 0;

	f = find_fixup(dev);
	if (!f) return 0;

	switch (type)
	{
	case PCI_BAR_IO:    post_fix = f->post.io_fixup;    break;
	case PCI_BAR_MMIO:  post_fix = f->post.mmio_fixup;  break;
	case PCI_BAR_PMMIO: post_fix = f->post.pmmio_fixup; break;
	default: return 0;
	}

	if (post_fix) {
		PCIDBG("running post-cfg %s fixup %p\n",
		       region_get_name(type), post_fix);
		DBGINDENT(3);
		ret = post_fix(dev);
		DBGINDENT(-3);
	}
	return ret;
}

static struct pci_fixup_t *
find_fixup(pci_dev *dev)
{
	struct pci_fixup_t *f;

	for (f=&fixup_table[0]; f->vendor; f++) {
		if (f->vendor == dev->vendor &&
		    f->device == dev->device)
			return f;
	}
	return NULL;
}

static int
osb4_acpi_fixup(pci_dev *dev)
{
	mptable_INT_t mpirq;

	/* route SCI interrupt */
	dev->irq = pciirq_5k(SERVERWORKS_IRQ_INDEX_ACPI, 1 << IRQ_ACPI);
	pciirq_5k_set_level(dev->irq);
	PCIDBG("ServerWorks ACPI: SCI interrupt is %d\n", dev->irq);

	/* SCI interrupt is Level Triggered, Active-Lo Polarity
	 * routed through the ISA IO-APIC */
	mpirq.entry		= mp_entry_IOINT;
	mpirq.type		= mpINT_type__INT;
	mpirq.polarity		= mpINT_polarity__Lo;
	mpirq.trigger		= mpINT_trigger__Level;
	mpirq.dest.ioapic.id	= ioapic_get_id(IOAPIC_ISA);
	mpirq.dest.ioapic.input	= dev->irq;
	mpirq.src.irq.isa	= dev->irq;
	mpirq.src.bus		= 0xff; /* set later */

	mpINT_add(&mpirq);

	return 0;
}

static int
osb4_acpi_io_fixup(pci_dev* dev)
{
	acpi_osb4_init(dev);  /* init the ACPI io regions */
	return 0;
}

static int
csb5_lpc_irq_fixup(pci_dev *dev)
{
	/* PCI-LPC device indicates that it wants an IRQ
	 * when it does not need, use, or even want one */
	dev->irq = 0;
	pci_dev_writeb(dev, PCI_INTERRUPT_LINE, 0xff);

	PCIDBG("Ignoring PCI-LPC device request for interrupt.\n");

	return 0xff;
}

static int
svwks_usb_irq_fixup(pci_dev *dev)
{
	mptable_INT_t *mpirq;

	/* find the mptable entry for USB device */
	mpirq = mpINT_find(dev->bus->busnum, PCI_SLOT(dev->devfn), 0);
	if (!mpirq) {
		printf("PCI fixup: ServerWorks USB MPtable entry not found!\n");
		return 0;
	}

	/* it _must_ route to ISA IO-APIC and PIN */
	mpirq->dest.ioapic.id = ioapic_get_id(IOAPIC_ISA);
	mpirq->dest.ioapic.input = dev->irq;

	PCIDBG("ServerWorks USB: interrupt re-routed to IO-APIC %d INT %d\n",
	       mpirq->dest.ioapic.id, mpirq->dest.ioapic.input);

	return dev->irq;
}

static int
osb4_ide_fixup(pci_dev *dev)
{
	/* set prog-if primary/secondary to compatible mode */
	pci_dev_writeb(dev, 0x09, 0x8F);

	/* set DMA timings for mode 2 */
	pci_dev_writel(dev, 0x44, 0x20202020);

	/* set Ultra DMA Mode 2 */
	pci_dev_writew(dev, 0x56, 0x2222);

	/* enable Ultra DMA */
	pci_dev_writeb(dev, 0x54, 0xf);

	PCIDBG("OSB4 IDE: Native PCI enabled and set for Ultra DMA mode 2\n");

	return 0;
}

static int
csb5_ide_fixup(pci_dev *dev)
{
	pci_dev *sbdev;
	uint8 reg8;

	sbdev = pci_lookup(PCI_VENDOR_SERVERWORKS,
			   PCI_DEVICE_SERVERWORKS_CSB5);
	if (sbdev) {
		uint32 reg32;

		/*
		 * CSB5 main fn has IMB register with IDE bits
		 *
 		 *  bit 14 [1] Enable IDE function
		 *  bit 18 [0] Enable IDE clock select
		 *  bit 19 [0] Reset IDE controller
		 *  bit 27 [0] Enable IRQ14 bypass IDE logic
		 *  bit 28 [0] Enable IRQ15 bypass IDE logic
		 *  bit 29 [0] Enable IDE REQ/GNT on req0/req1 pin
		 *             for external arbiter
		 */
		reg32 = pci_dev_read32(sbdev, 0x64);
		reg32 &= ~0x380c0000;
		reg32 |=  0x00004000;
		pci_dev_write32(sbdev, 0x64, reg32);
		
		/* the IMB BTR2 register [0xAC] must be
		 * set to 1 if DMA of IDE function is used */
		reg8 = pci_dev_read8(sbdev, 0xAC);
		reg8 |= 0x01;
		pci_dev_write8(sbdev, 0xAC, reg8);
	}

	/* set prog-if primary/secondary to native PCI mode */
	pci_dev_writeb(dev, 0x09, 0x8f);

	/*
	 * CSB5-IDE UDMA Control Register:
	 *
	 *  bit 6   : Clear this to enable DMA
	 *  bit 1-0 : Select supported DMA Mode
	 *             00 = Legacy DMA
	 *             01 = udma2
	 *             10 = udma2, udma4
	 *             11 = udma2, udma4, udma5
	 */
	reg8 = pci_dev_read8(dev, 0x5A);
	reg8 &= ~0x40;
	reg8 |=  0x03;
	pci_dev_write8(dev, 0x5A, reg8);

	/* set up drive timings */
	pci_dev_write16(dev, 0x48, 0xf0f0); /* PIO pre-fetch and post-write */
	pci_dev_write32(dev, 0x40, 0x20202020); /* PIO 4 timings */
	pci_dev_write16(dev, 0x4a, 0x4444);  /* PIO 4 */   
	pci_dev_write32(dev, 0x44, 0x77777777); /* dma mode 0 */
		
	/* 
	 * program the subsystem vendor/device fields 
	 * set the vendor to Sun
	 * bits [14:15] of device reflect primary/secondary 80-pin cables
	 * -- precedent for this was set by DELL
	 * -- these registers are write-once
	 * -- apparently 2 16 bit config cycles don't work, use 1 32 bit
	 */
        pci_dev_write32(dev, PCI_SUBSYS_VENDOR, PCI_VENDOR_SUN|(0xc000<<16));

	PCIDBG("ServerWorks CSB5 IDE: Ultra DMA enabled in native PCI mode\n");

	return 0;
}

/* Fix IDE interrupts: use 14 and 15
 * - Level Triggered,
 * - Active-Hi Polarity,
 * - routed through the ISA bus & ISA IO-APIC */
static int
svwks_ide_irq_fixup (pci_dev *dev)
{
	mptable_INT_t mpirq;
	uint8 ide0irq=IRQ_IDE0, ide1irq=IRQ_IDE1;

	if (dev->device == PCI_DEVICE_SERVERWORKS_CSB5IDE) {
		ide0irq = pciirq_5k(SERVERWORKS_IRQ_INDEX_IDE0, 1 << IRQ_IDE0);
		ide1irq = pciirq_5k(SERVERWORKS_IRQ_INDEX_IDE1, 1 << IRQ_IDE1);
	}

	dev->irq = ide0irq;

	mpirq.entry		= mp_entry_IOINT;
	mpirq.type		= mpINT_type__INT;
	mpirq.polarity		= mpINT_polarity__Hi;
	mpirq.trigger		= mpINT_trigger__Level;
	mpirq.dest.ioapic.id	= ioapic_get_id(IOAPIC_ISA);
	mpirq.src.bus		= 0xff; /* set later */

	mpirq.src.irq.isa	= ide0irq;
	mpirq.dest.ioapic.input	= ide0irq;
	mpINT_add(&mpirq);

	mpirq.src.irq.isa	= ide1irq;
	mpirq.dest.ioapic.input	= ide1irq;
	mpINT_add(&mpirq);

	return dev->irq;
}

static int
sym53c875_fixup(pci_dev *dev)
{
	/* this chip has silly defaults in the BARs */
	pci_bar *b;

	b = &dev->bar[0];
	b->base = b->size = b->limit = 0;
	pci_dev_writel(dev, PCI_BAR_ADDR(0), 0);

	b = &dev->bar[1];
	b->base = b->size = b->limit = 0;
	pci_dev_writel(dev, PCI_BAR_ADDR(1), 0);

	b = &dev->bar[2];
	b->base = b->size = b->limit = 0;
	pci_dev_writel(dev, PCI_BAR_ADDR(2), 0);

	return 0;
}

static int
cnb30le_fixup_pre(pci_dev *dev)
{
	/* set SBUSNUM high to allow all bus numbers */
	pci_dev_writeb(dev, PCI_SBUSNUM, 0xff);
	return 0;
}

static int
cnb30le_fixup_post(pci_dev *dev)
{
	pci_bus *bus;
	byte reg;

	/* enable Memory and I/O access */
	reg = pci_dev_readb(dev, PCI_COMMAND);
	reg |= PCI_CMD_IO | PCI_CMD_MEMORY;
	pci_dev_writeb(dev, PCI_COMMAND, reg);
	PCIDBG("I/O & Memory address space enabled\n");

	switch (PCI_FUNC(dev->devfn)) {
	case 0:
		
		/*
		 * Primary function of NorthBridge
		 */

		bus = dev->bus;
		bus->self = dev;
		bus->set_resource_range = &pci_resource_cnb30le;
		bus->fbbe = 1;

		/* write primary PCI Subordinate Bus Number */
		pci_dev_writeb(dev, PCI_SBUSNUM, bus->sub);
		break;

	case 1:

		/*
		 * Secondary function of NorthBridge
		 */

		/* get memory for pci bus structure */
		bus = malloc(sizeof(*bus));
		if (!bus) {
			printf("malloc failed at %s:%d\n",
				__FILE__, __LINE__);
			return -1;
		}
		pci_bus_blank(bus);
		bus->root = 1;
		bus->self = dev;
		bus->set_resource_range = &pci_resource_cnb30le;
		bus->fbbe = 1;

		/* determine secondary PCI bus speed
		 * from SCR2 register (0x4F) of primary host-bridge
		 * bit0: 0=33MHz, 1=66MHz
		 */
		reg = pci_dev_readb(dev->bus->self, PCI_SERVERWORKS_SCR2);
		bus->freq = (reg & 1);

		/* set the Bus Number of the peer host-bridge
		   to 1 + Subordinate Bus Number of primary */
		bus->busnum = bus->last_kid = dev->bus->last_kid + 1;
		pci_dev_writeb(dev, PCI_BUSNUM, bus->busnum);

		/* scan for devices */
		pci_bus_scan(bus);
		bus->sub = bus->last_kid;

		/* save the Subordinate Bus Number found while scanning
		 * the peer-bus into the SBUSNUM register on peer host-bridge */
		pci_dev_writeb(dev, PCI_SBUSNUM, bus->sub);

		/* append peer bus to primary */
		while (dev->bus->peer)
			dev->bus = dev->bus->peer;
		dev->bus->peer = bus;

		PCIDBG("Enabled peer PCI bus.\n");
		break;

	default:
		return -1;
	}

	return 0;
}

static int
chrysalis_luna340_fixup(pci_dev *dev)
{
	if (dev->revision == 0) {
		/* 
		 * Revision 0 (Luna340 A) has a bug in it's memory decoder.
		 * This is by request of Roger Hebb <RHebb@chrysalis-its.com>
		 */
		pci_bar *b = &dev->bar[0];
		b->size = 16 << 20;
		b->base = region_allocate(PCI_BAR_MMIO, b->size);
		b->limit = b->base + b->size - 1;
		pci_dev_writel(dev, PCI_BAR_ADDR(0), b->base);
		PCIDBG("Chrysalis Luna340: re-assigned BAR0=0x%08x (%s)\n", 
			b->base, print_bytes(b->size));
	}
	return 0;
}

static int m1541_fixup(pci_dev *dev)
{
	dev->bus->set_resource_range = &pci_resource_m1541;
	pci_dev_write_config( dev, PCI_COMMAND, 0x0006, 2);
	pci_dev_write_config( dev, PCI_STATUS+1, 0xf4, 1);
	pci_dev_write_config( dev, 0x0d, 0x20, 1);
	return 0;
}

static int m7101_fixup(pci_dev *dev)
{
	pci_bar *b;

	b = &dev->bar[0];
	b->type = PCI_BAR_IO;
	b->base = 0x00006001;
	b->size = 4;
	b->limit = b->base + b->size - 1;
	pci_dev_write_config(dev, PCI_BAR_ADDR(0), b->base, b->size);

	b = &dev->bar[1];
	b->type = PCI_BAR_IO;
	b->base = 0x00003a81;
	b->size = 4;
	b->limit = b->base + b->size - 1;
	pci_dev_write_config(dev, PCI_BAR_ADDR(1), b->base, b->size);

	return 1;
}

static int m5237_fixup(pci_dev *dev)
{
	dev->irq = 6;
	pci_dev_write_config( dev, PCI_INTERRUPT_LINE, dev->irq, 1);
	return 1;
}

static int
m5229_fixup(pci_dev *dev)
{
	uint8 reg;

	/* first enable the progintf byte - see the spec for details */
	reg = pci_dev_readb(dev, 0x50);
	pci_dev_writeb(dev, 0x50, reg | 0x03);

	/* turn off this bit to make progintf writeable - specs says 0 is OK */
	reg = pci_dev_readb(dev, 0x4d);
	pci_dev_writeb(dev, 0x4d, reg & ~0x80);

	/* turn off these bits to make progintf writeable */
	pci_dev_writeb(dev, 0x43, 0x00);

	/* turn on full native mode, fully enabled disk controllers */
	pci_dev_writeb(dev, 0x09, 0xff);

	/* set FIFO threshholds - DMA, 8 WORDs */
	pci_dev_writeb(dev, 0x54, 0x99);
	pci_dev_writeb(dev, 0x55, 0x99);

	/* set UDMA - driver can change this  - UDMA mode 0 - conservative */
	pci_dev_writeb(dev, 0x56, 0xcc);
	pci_dev_writeb(dev, 0x57, 0xcc);

	PCIDBG("ALI M5229: Native PCI enabled and set for Ultra DMA mode 0\n");

	return 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
