/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: pci.c,v 1.1.1.1 2003/06/10 22:42:21 iceblink Exp $
 */
#include <string.h>
#include <printf.h>

#include "common/rom.h"
#include "common/pci.h"

#include "pci.h"

static void do_dump_pci(int bus, int dev, int fun);

/*
 * menu functions
 */

int 
lspci_func(int argc, char *argv[] __attribute__ ((unused)))
{
	pci_dev *dev;
    
	if (argc != 1) {
		return EBADNUMARGS;
	}

	printf("\nBus Dev Func  Vid   Did Class Rev  Name\n");
	printf("-------------------------------------------------\n");

	for (dev = pci_devices; dev; dev = dev->allnext) {
		printf(" %02x  %02x  %02x  %04x  %04x  %04x  %02x  %s",
			dev->bus->busnum, PCI_SLOT(dev->devfn),
			PCI_FUNC(dev->devfn), dev->vendor, dev->device,
			dev->class, dev->revision, dev->name);

		if (dev->irq > 0) {
			printf(" (IRQ%d)", dev->irq);
		}

		printf("\n");
	}
    
	return 0;
}

int read_pci_cfg_func( int argc, char *argv[] )
{
    unsigned long bus, dev, fun, reg, size, value;

    if( argc != 6 )
	return EBADNUMARGS;
    
    if ( ( stoli( argv[1], &bus ) != 0) || 
	 ( stoli( argv[2], &dev ) != 0) || 
	 ( stoli( argv[3], &fun ) != 0) || 
	 ( stoli( argv[4], &reg ) != 0) || 
	 ( stoli( argv[5], &size ) != 0) ) 
	return EBADNUM;
    
    value = pci_read_config( bus, PCI_DEVFN(dev, fun), reg, size );
    switch (size) {
	case 1:
    		printf( "0x%02x\n", (unsigned int) value );
		break;
	case 2:
    		printf( "0x%04x\n", (unsigned int) value );
		break;
	case 4:
	default:
    		printf( "0x%08x\n", (unsigned int) value );
    }

    return 0;
}

int write_pci_cfg_func( int argc, char *argv[] )
{
    unsigned long int bus, dev, fun, reg, size, value;

    if( argc != 7 )
	return EBADNUMARGS;
    
    if ( ( stoli( argv[1], &bus ) != 0) || 
	 ( stoli( argv[2], &dev ) != 0) || 
	 ( stoli( argv[3], &fun ) != 0) || 
	 ( stoli( argv[4], &reg ) != 0) || 
	 ( stoli( argv[5], &size ) != 0) || 
	 ( stoli( argv[6], &value ) != 0) ) 
	return EBADNUM;
    
    pci_write_config( bus, PCI_DEVFN(dev, fun), reg, size, value );

    return 0;
}

int print_pci_header_func(int argc, char *argv[])
{
	unsigned long int bus, dev, fun;
	unsigned char htype;

	if (argc != 4)
		return EBADNUMARGS;
    
	if ((stoli(argv[1], &bus) != 0) || 
	    (stoli(argv[2], &dev) != 0) || 
	    (stoli(argv[3], &fun) != 0))
		return EBADNUM;

	printf("\nPCI Header for %02lx:%02lx:%02lx\n\n", bus, dev, fun);
	printf("0x00 [2] Vendor ID ............... %04x\n", 
		pci_read_config(bus, PCI_DEVFN(dev, fun), 0x00, 2));
	printf("0x02 [2] Device ID ............... %04x\n", 
		pci_read_config(bus, PCI_DEVFN(dev, fun), 0x02, 2));
	printf("0x04 [2] Command Register ........ %04x\n", 
		pci_read_config(bus, PCI_DEVFN(dev, fun), 0x04, 2));
	printf("0x06 [2] Status Register ......... %04x\n", 
		pci_read_config(bus, PCI_DEVFN(dev, fun), 0x06, 2));
	printf("0x08 [1] Revision ID ............. %02x\n", 
		pci_read_config(bus, PCI_DEVFN(dev, fun), 0x08, 1));
	printf("0x09 [3] Class Code .............. %06x\n", 
		pci_read_config(bus, PCI_DEVFN(dev, fun), 0x08, 4)>>8);
	printf("0x0C [1] Cache Line Size ......... %02x\n", 
		pci_read_config(bus, PCI_DEVFN(dev, fun), 0x0c, 1));
	printf("0x0D [1] Latency Timer ........... %02x\n", 
		pci_read_config(bus, PCI_DEVFN(dev, fun), 0x0d, 1));
	htype = pci_read_config(bus, PCI_DEVFN(dev, fun), 0x0e, 1);
	printf("0x0E [1] Header Type ............. %02x\n", htype);
	printf("0x0F [1] BIST .................... %02x\n", 
		pci_read_config(bus, PCI_DEVFN(dev, fun), 0x0f, 1));
	printf("0x10 [4] Base Address 0 .......... %08x\n", 
		pci_read_config(bus, PCI_DEVFN(dev, fun), 0x10, 4));
	printf("0x14 [4] Base Address 1 .......... %08x\n", 
		pci_read_config(bus, PCI_DEVFN(dev, fun), 0x14, 4));
	if ((htype & ~0x80) == 0) {
		/* standard device */
		printf("0x18 [4] Base Address 2 .......... %08x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x18, 4));
		printf("0x1C [4] Base Address 3 .......... %08x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x1c, 4));
		printf("0x20 [4] Base Address 4 .......... %08x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x20, 4));
		printf("0x24 [4] Base Address 5 .......... %08x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x24, 4));
		printf("0x28 [4] CardBus CIS Ptr ......... %08x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x28, 4));
		printf("0x2C [2] Subsystem Vendor ID ..... %04x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x2c, 2));
		printf("0x2E [2] Subsystem ID ............ %04x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x2e, 2));
		printf("0x30 [4] Expansion ROM ........... %08x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x30, 4));
		printf("0x34 [1] Capabilities ............ %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x34, 1));
		printf("0x3C [1] Interrupt Line .......... %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x3c, 1));
		printf("0x3D [1] Interrupt Pin ........... %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x3d, 1));
		printf("0x3E [1] Min_Gnt ................. %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x3e, 1));
		printf("0x3F [1] Max_Lat ................. %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x3f, 1));
	} else if ((htype & ~0x80) == 1) {
		/* PCI-PCI bridge */
		printf("0x18 [1] Primary Bus Number ...... %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x18, 1));
		printf("0x19 [1] Secondary Bus Number .... %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x19, 1));
		printf("0x1A [1] Subordinate Bus Number .. %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x1a, 1));
		printf("0x1B [1] Secondary Latency Timer . %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x1b, 1));
		printf("0x1C [1] I/O Base ................ %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x1c, 1));
		printf("0x1D [1] I/O Limit ............... %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x1d, 1));
		printf("0x1E [2] Secondary Status ........ %04x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x1e, 2));
		printf("0x20 [2] Memory Base ............. %04x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x20, 2));
		printf("0x22 [2] Memory Limit ............ %04x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x22, 2));
		printf("0x24 [2] Prefetch Memory Base .... %04x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x24, 2));
		printf("0x26 [2] Prefetch Memory Limit ... %04x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x26, 2));
		printf("0x28 [4] Prefetch Base (upper 32)  %08x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x28, 4));
		printf("0x2C [4] Prefetch Limit (upper 32) %08x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x2c, 4));
		printf("0x30 [2] I/O Base (upper 16) ..... %04x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x30, 2));
		printf("0x32 [2] I/O Limit (upper 16) .... %04x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x32, 2));
		printf("0x34 [1] Capabilities ............ %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x34, 1));
		printf("0x38 [4] Expansion ROM ........... %08x\n",
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x38, 4));
		printf("0x3C [1] Interrupt Line .......... %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x3c, 1));
		printf("0x3D [1] Interrupt Pin ........... %02x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x3d, 1));
		printf("0x3E [2] Bridge Control .......... %04x\n", 
			pci_read_config(bus, PCI_DEVFN(dev, fun), 0x3e, 2));
	} else {
		printf("Unknown header type\n");
	}
	printf("\n");

	return 0;
}

int dump_pci_func(int argc, char *argv[])
{
    unsigned long int bus, dev, fun;

    if( argc != 4 )
	return EBADNUMARGS;
    
    if ( ( stoli( argv[1], &bus ) != 0) || 
	 ( stoli( argv[2], &dev ) != 0) || 
	 ( stoli( argv[3], &fun ) != 0) )
	return EBADNUM;

    do_dump_pci(bus, dev, fun);

    return 0;
}

static void 
do_dump_pci(int bus, int dev, int fun)
{
    int index;
    int value;

    printf("%02x:%02x:%02x\n", (unsigned int)bus, (unsigned int)dev,
        (unsigned int)fun);
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");

    for (index = 0; index <= 0xff; index++) {
	if (!(index % 0x10)) {
	    printf("\n%02x: ", index);
	}

	value = pci_read_config(bus, PCI_DEVFN(dev, fun), index, 1);
	
	printf(" %02x", value);
    }
    printf("\n");
}

int 
dump_all_pci_func(int argc, char * argv[] __attribute__((unused)))
{
        pci_dev *dev;

	if (argc != 1) {
		return EBADNUMARGS;
	}

        for (dev = pci_devices; dev; dev=dev->allnext) {
		do_dump_pci(dev->bus->busnum, PCI_SLOT(dev->devfn), 
			PCI_FUNC(dev->devfn));
		printf("\n");
	}
    
	return 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
