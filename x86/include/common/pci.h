/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
#ifndef COMMON_PCI_H
#define COMMON_PCI_H

#include <extra_types.h>

/*
 * The PCI interface treats multi-function devices as independent
 * devices.  The slot/function address of each device is encoded
 * in a single byte as follows:
 *
 *	7:3 = slot (dev)
 *	2:0 = function
 */
#define PCI_DEVFN(dev,fn)	((byte)((((dev) & 0x1f) << 3) | ((fn) & 0x07)))
#define PCI_SLOT(devfn)		((byte)(((devfn) >> 3) & 0x1f))
#define PCI_FUNC(devfn)		((byte)((devfn) & 0x07))
#define PCI_DEVICE(vid,did)	PCI_VENDOR_##vid, PCI_DEVICE_##vid##_##did

#if DEBUG_PCI
extern int dbgindent;
#define	PCIDBG(fmt, args...)	do { int n = dbgindent; \
					printf("PCI: "); \
					while (n-- > 0) { \
						printf(" "); \
					} \
					printf(fmt, ##args); \
				} while (0)
#define	DBGINDENT(n)		(dbgindent += n)
#else
#define	PCIDBG(fmt, args...)
#define DBGINDENT(n)
#endif

/* Acer Labs (ALi) */
#define PCI_VENDOR_AL				0x10b9
#define PCI_DEVICE_AL_M1541			0x1541
#define PCI_DEVICE_AL_M1533			0x1533
#define PCI_DEVICE_AL_M5229			0x5229
#define PCI_DEVICE_AL_M5237			0x5237
#define PCI_DEVICE_AL_M5243			0x5243
#define PCI_DEVICE_AL_M7101			0x7101

/* ServerWorks */
#define PCI_VENDOR_SERVERWORKS			0x1166
#define PCI_DEVICE_SERVERWORKS_LE		0x0009
#define PCI_DEVICE_SERVERWORKS_OSB4		0x0200
#define PCI_DEVICE_SERVERWORKS_CSB5             0x0201
#define PCI_DEVICE_SERVERWORKS_OSB4IDE		0x0211
#define PCI_DEVICE_SERVERWORKS_CSB5IDE          0x0212
#define PCI_DEVICE_SERVERWORKS_USB		0x0220
#define PCI_DEVICE_SERVERWORKS_CSB5LPC          0x0230
#define PCI_SERVERWORKS_SCR2			0x4f

#define PCI_SERVERWORKS_MULR			0x70
#define PCI_SERVERWORKS_MHLR0			0x74
#define PCI_SERVERWORKS_MHLR1			0x75
#define PCI_SERVERWORKS_MHUR0			0x76
#define PCI_SERVERWORKS_MHUR1			0x77

#define PCI_SERVERWORKS_MRDRL0			0x80
#define PCI_SERVERWORKS_MRDRU0			0x81
#define PCI_SERVERWORKS_MRDRL1			0x82
#define PCI_SERVERWORKS_MRDRU1			0x83
#define PCI_SERVERWORKS_MRDRL2			0x84
#define PCI_SERVERWORKS_MRDRU2			0x85
#define PCI_SERVERWORKS_MRDRL3			0x86
#define PCI_SERVERWORKS_MRDRU3			0x87
#define PCI_SERVERWORKS_MRDRL4			0x88
#define PCI_SERVERWORKS_MRDRU4			0x89
#define PCI_SERVERWORKS_MRDRL5			0x8a
#define PCI_SERVERWORKS_MRDRU5			0x8b
#define PCI_SERVERWORKS_MRDRL6			0x8c
#define PCI_SERVERWORKS_MRDRU6			0x8d
#define PCI_SERVERWORKS_MRDRL7			0x8e
#define PCI_SERVERWORKS_MRDRU7			0x8f

#define PCI_SERVERWORKS_IO_BASE			0xd0
#define PCI_SERVERWORKS_IO_LIMIT		0xd2
#define PCI_SERVERWORKS_IO_EN			0x01
#define PCI_SERVERWORKS_IORV			0xdc

#define PCI_SERVERWORKS_MMIO_BASE		0xc0
#define PCI_SERVERWORKS_MMIO_LIMIT		0xc2
#define PCI_SERVERWORKS_MMIO_EN			0x01
#define PCI_SERVERWORKS_MMIO_WC			0x01

#define PCI_SERVERWORKS_PMMIO_BASE		0xc4
#define PCI_SERVERWORKS_PMMIO_LIMIT		0xc6
#define PCI_SERVERWORKS_PMMIO_EN       		0x0c
#define PCI_SERVERWORKS_PMMIO_WC		0x02

/* memory region valid */
#define PCI_SERVERWORKS_PMRV			0xcc
/* write combining command */
#define PCI_SERVERWORKS_WCCR			0xcd

/* Intel */
#define PCI_VENDOR_INTEL			0x8086
#define PCI_DEVICE_INTEL_82557			0x1229
#define PCI_DEVICE_INTEL_82559ER		0x1209

/* National Semiconductor */
#define PCI_VENDOR_NS				0x100b
#define PCI_DEVICE_NS_83815			0x0020

/* Highpoint */
#define PCI_VENDOR_HIGHPOINT			0x1103
#define PCI_DEVICE_HIGHPOINT_HPT370		0x0004

/* Symbios Logic (LSI) */
#define PCI_VENDOR_SYMBIOS			0x1000
#define PCI_DEVICE_SYMBIOS_53C875		0x000f
#define PCI_DEVICE_SYMBIOS_53C1010		0x0020
#define PCI_DEVICE_SYMBIOS_53C1010_66		0x0021

/* Chrysalis */
#define PCI_VENDOR_CHRYSALIS			0xcafe
#define PCI_DEVICE_CHRYSALIS_LUNA340		0xc0de

/* DEC */
#define PCI_VENDOR_DEC				0x1011
#define PCI_DEVICE_DEC_21152			0x0024

/* TigerJet Networks */
#define PCI_VENDOR_TIGERJET			0xe159
#define PCI_DEVICE_TIGERJET_TIGER600		0x0600

/* Sun */
#define PCI_VENDOR_SUN				0x108e

/* Base Address Register types */
typedef enum {
	PCI_BAR_EMPTY=0,
	PCI_BAR_IO,
	PCI_BAR_MMIO,
	PCI_BAR_PMMIO,
} pci_bar_t;

/* forward declarations to make compiler happy */
struct _pci_bus;
struct _pci_dev;

/*
 *  a PCI Base Address Register
 */
typedef struct _pci_bar {
	pci_bar_t type; /* defined above */
	struct _pci_dev *dev; /* points back to device */
	dword base;
	dword limit;
	dword size, size2;
	int addr64;
} pci_bar;

/*
 *  a PCI Bus representation
 */
typedef struct _pci_bus {
	byte busnum; /* self bus number */
	byte pri; /* primary bus number */
	byte sub; /* subordinate bus number */
	byte last_kid; /* last child bus number allocated */
	byte fbbe; /* can all devices do fast back-to-back transactions */
	byte freq; /* bus frequency 0=33 MHz, 1=66 MHz */
	byte root; /* is this a root bus? */

	struct _pci_dev *self; /* PCI dev of the bridge to this bus */
	struct _pci_dev *devices; /* devices on this bus */
	struct _pci_bus *parent; /* parent bus (bus num == this.pri) */
	struct _pci_bus *kids; /* child bus list */
	struct _pci_bus *peer; /* next bus in peer list */

	struct _pci_bar resource[4]; /* resource maps */
	uint32 (*set_resource_range)(struct _pci_bus *bus, pci_bar_t type);
} pci_bus;

/*
 *  a PCI Device representation
 */
typedef struct _pci_dev {
	word vendor; /* vendor id */
	word device; /* device id */
	byte htype; /* header type (dev/bridge etc) */
	dword class; /* device class */
	byte revision; /* revision number */
	byte devfn; /* slot/function */
	byte irq; /* assigned irq */
	byte master; /* can the device busmaster */
	byte multifunc; /* is this device the primary of a multifunction */

	struct _pci_bus *bus; /* bus on which this dev lives */
	struct _pci_dev *allnext; /* next dev on global list */
	struct _pci_dev *busnext; /* next dev on this bus */ 
	struct _pci_bar bar[6]; /* base address registers */

	const char *name; /* string representation of device name */
} pci_dev;

extern pci_dev *pci_devices;
void pci_init(void);
void pci_list(void);

void pci_dev_blank(pci_dev *dev); 
void pci_bus_blank(pci_bus *bus); 
void pci_bar_scan(pci_dev *dev, int nbars);
void pci_bus_scan(pci_bus *bus);
void pci_set_hole( void );

pci_dev *pci_lookup(word vendor, word device);
pci_dev *pci_lookup_dev(word vendor, word device, pci_dev *start);
pci_dev *pci_lookup_dev_by_bus(byte bus, byte devfn);
pci_bus *pci_lookup_bus(byte num);

dword pci_dev_read_config(pci_dev * dev, byte reg, int size);
#define pci_dev_read_config_byte(dev,reg)\
	(byte)(pci_dev_read_config(dev, reg, 1) & 0xff)
#define pci_dev_read_config_word(dev,reg)\
	(word)(pci_dev_read_config(dev, reg, 2) & 0xffff)
#define pci_dev_read_config_dword(dev,reg)\
	(dword)(pci_dev_read_config(dev, reg, 4) & 0xffffffff)
#define pci_dev_readb(d,r)\
	pci_dev_read_config_byte(d,r)
#define pci_dev_readw(d,r)\
	pci_dev_read_config_word(d,r)
#define pci_dev_readl(d,r)\
	pci_dev_read_config_dword(d,r)
#define pci_dev_read8(d,r)\
	pci_dev_read_config_byte(d,r)
#define pci_dev_read16(d,r)\
	pci_dev_read_config_word(d,r)
#define pci_dev_read32(d,r)\
	pci_dev_read_config_dword(d,r)

dword pci_read_config(byte bus, byte devfn, byte reg, int size);
#define pci_read_config_byte(bus,devfn,reg)\
	(byte)(pci_read_config(bus, devfn, reg, 1) & 0xff)
#define pci_read_config_word(bus,devfn,reg)\
	(word)(pci_read_config(bus, devfn, reg, 2) & 0xffff)
#define pci_read_config_dword(bus,devfn,reg)\
	(dword)(pci_read_config(bus, devfn, reg, 4) & 0xffffffff)
#define pci_readb(b,df,r)\
	pci_read_config_byte(b,df,r)
#define pci_readw(b,df,r)\
	pci_read_config_word(b,df,r)
#define pci_readl(b,df,r)\
	pci_read_config_dword(b,df,r)
#define pci_read8(b,df,r)\
	pci_read_config_byte(b,df,r)
#define pci_read16(b,df,r)\
	pci_read_config_word(b,df,r)
#define pci_read32(b,df,r)\
	pci_read_config_dword(b,df,r)

void pci_dev_write_config(pci_dev * dev, byte reg, dword value, int size);
#define pci_dev_write_config_byte(dev,reg,val)\
	pci_dev_write_config(dev, reg, val, 1)
#define pci_dev_write_config_word(dev,reg,val)\
	pci_dev_write_config(dev, reg, val, 2)
#define pci_dev_write_config_dword(dev,reg,val)\
	pci_dev_write_config(dev, reg, val, 4)
#define pci_dev_writeb(d,r,v)\
	pci_dev_write_config_byte(d,r,v)
#define pci_dev_writew(d,r,v)\
	pci_dev_write_config_word(d,r,v)
#define pci_dev_writel(d,r,v)\
	pci_dev_write_config_dword(d,r,v)
#define pci_dev_write8(d,r,v)\
	pci_dev_write_config_byte(d,r,v)
#define pci_dev_write16(d,r,v)\
	pci_dev_write_config_word(d,r,v)
#define pci_dev_write32(d,r,v)\
	pci_dev_write_config_dword(d,r,v)

void pci_write_config(byte bus, byte devfn, byte reg, dword value, int size);
#define pci_write_config_byte(bus,devfn,reg,val)\
	pci_write_config(bus, devfn, reg, val, 1)
#define pci_write_config_word(bus,devfn,reg,val)\
	pci_write_config(bus, devfn, reg, val, 2)
#define pci_write_config_dword(bus,devfn,reg,val)\
	pci_write_config(bus, devfn, reg, val, 4)
#define pci_writeb(b,df,r,v)\
	pci_write_config_byte(b,df,r,v)
#define pci_writew(b,df,r,v)\
	pci_write_config_word(b,df,r,v)
#define pci_writel(b,df,r,v)\
	pci_write_config_dword(b,df,r,v)
#define pci_write8(b,df,r,v)\
	pci_write_config_byte(b,df,r,v)
#define pci_write16(b,df,r,v)\
	pci_write_config_word(b,df,r,v)
#define pci_write32(b,df,r,v)\
	pci_write_config_dword(b,df,r,v)

#endif

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
