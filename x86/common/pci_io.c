/* $Id: pci_io.c,v 1.1.1.1 2003/06/10 22:41:51 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 *
 * functions for setup and query of PCI bus
 *
 * Erik Gilling
 * Tim Hockin
 * Duncan Laurie
 */

#include <io.h>
#include <printf.h>

#include "common/pci.h"
#include "common/pci_regs.h"

/* generate a 4-byte value to read/write from pci config space */
#define PCI_CONFIG(bus,devfn,reg)	((dword)((0x80000000) 	 	| \
						((reg) & 0xfc)  	| \
						(((bus) & 0xff) << 16) 	| \
						(((devfn) & 0xff) << 8)))

/* pci_read_config()
 *
 *   read "size" bytes from pci config space
 *   at location "reg" of device "devfn" on bus "bus"
 *
 *   inputs: bus number,
 *           device/func number,
 *           register to read from,
 *           number of bytes to read
 *   return: value from pci config space
 */
dword pci_read_config(byte bus, byte devfn, byte reg, int size)
{
	word in_port;

	outl(PCI_CONFIG(bus, devfn, reg), 0xcf8);
	in_port = 0xcfc + (reg & 0x3); /* offset the read */

	switch (size)
	{
	    case 1:	return (dword) inb(in_port);   /*  BYTE   */
 	    case 2:	return (dword) inw(in_port);   /*  WORD   */
	    case 4:	return (dword) inl(in_port);   /*  DWORD  */
	    default:	printf("PCI: invalid config size %d\n", size);
	}

	return 0;
}

/* pci_write_config()
 *
 *   write "size" bytes of "value" to pci config space
 *   at location "reg" of device "devfn" on bus "bus"
 *
 *   inputs: bus number,
 *           device/func number,
 *           register to write,
 *           value to write,
 *           number of bytes to write
 *   return: nothing
 */
void pci_write_config(byte bus, byte devfn, byte reg, dword value, int size)
{
	word out_port;
    
	outl(PCI_CONFIG(bus, devfn, reg), 0xcf8);

	/* offset the read */
	out_port = 0xcfc + (reg & 0x3);

	switch(size)
	{
	case 1: /*  BYTE   */
		outb((byte)value, out_port);
		break;

	case 2: /*  WORD   */
		outw((word)value, out_port);
		break;

	case 4: /*  DWORD  */
		outl((dword)value, out_port);
		break;

	default:
		printf("PCI: invalid config size %d\n", size);
	}
}

/* pci_dev_read_config()
 *
 *   read "size" bytes from pci device "dev" config space
 *   starting at location "reg"
 *
 *   inputs: pci device,
 *           register to read,
 *           number of bytes to read
 *   return: value read from pci config space
 */
dword pci_dev_read_config(pci_dev *dev, byte reg, int size)
{
	return pci_read_config(dev->bus->busnum, dev->devfn, reg, size);
}

/* pci_dev_write_config()
 *
 *   write "size" bytes of "value" to pci device "dev" config space
 *   starting at location "reg"
 *
 *   inputs: pci device,
 *           register to write,
 *           value to write,
 *           number of bytes to write
 *   return: nothing
 */
void pci_dev_write_config(pci_dev *dev, byte reg, dword value, int size)
{
	pci_write_config(dev->bus->busnum, dev->devfn, reg, value, size);
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
