/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: eth.c,v 1.2 2003/10/06 15:15:03 iceblink Exp $
 */

#include <io.h>

#include "common/rom.h"
#include "common/cmos.h"
#include "common/eth.h"
#include "common/eepro100_eeprom.h"
#include "common/macphyter_eeprom.h"
#include "common/dallas.h"
#include "common/pci.h"
#include "common/systype.h"
#include "common/delay.h"

static void config_eepro100(void)
{
	pci_dev *enet, *poll=pci_devices;
	unsigned short port;
	unsigned char *ssn;
	unsigned char mac[6];

	/* find and configure Intel EEPRO 100 boards on bus 0, and not in any PCI slots */
	while ((enet = pci_lookup_dev(PCI_VENDOR_INTEL, PCI_DEVICE_INTEL_82559ER, poll)) != NULL)
	{
		/* if on bus 0 and not in PCI slot */
		if ((enet->bus->busnum == 0)) {

			/* Get base I/O port */
			port = pci_dev_read_config_word(enet, 0x14);
			port &= 0xfffe;     
            
			eepro100_read_mac(port, mac);
            
			/* check for valid cobalt ID */
			if ((mac[0] == 0xff) && (mac[1] == 0xff) && (mac[2] == 0xff)) {
			        int i;
				printf("Invalid Cobalt MAC on Intel 82559ER at port 0x%04x\n", port);
				ssn = get_ssn();
				if (ssn) {
				/* grab middle 6 bytes of dallas ID and use as temp MAC address */
				for (i = 0; i < 6; i++) {
					mac[5-i] = ssn[i+1];
				}
				printf("Writing temporary MAC for Intel 82559ER at port 0x%04x\n", port);
				eepro100_write_mac(port, mac);
				}
			}
		}
		poll=enet;
	}
}

static void config_macphyter(void)
{
	pci_dev *enet, *poll=pci_devices;
	unsigned short port;
	unsigned char *ssn;
	unsigned char mac[6];

	/* for the xtr/qube3, find and configure National Semi DP83815
	   boards on bus 0, and not in any PCI slots. for alpine,
	   the macphyter is on bus 1. */
	while ((enet = pci_lookup_dev(PCI_VENDOR_NS, PCI_DEVICE_NS_83815, poll)) != NULL)
	{
		/* if on bus 0 and not in PCI slot for xtr/qube3.
		   if on bus 1 and not in PCI slot for alpine. */
		if ((enet->bus->busnum == 0) ||
		    ((boardtype() == BOARDTYPE_ALPINE) && 
		     (enet->bus->busnum == 1) &&
		     (enet->devfn != PCI_DEVFN(0x8,0)))) {
		        dword i;

			/* Get base I/O port */
			port = pci_dev_read_config_word(enet, 0x10);
			port &= 0xfffe;	
	    
			macphyter_read_mac(port, mac);
	    
			/* check for invalic MAC */
			if ((mac[0] == 0xff) && (mac[1] == 0xff) && (mac[2] == 0xff)) {
				printf("Invalid MAC on National DP83815 at port 0x%04x\n", port);
				ssn = get_ssn();
				if (ssn) {
				/* grab middle 6 bytes of dallas ID and use as temp MAC address */
				for (i = 0; i < 6; i++) {
					mac[5-i] = ssn[i+1];
				}
				printf("Writing temporary MAC on National DP83815 at port 0x%04x\n", port);
				macphyter_write_mac(port, mac);
				}
			}
		}
		poll = enet;
	}
}

static 
int configure_eth(void)
{
	config_eepro100();
	config_macphyter();
	return 0;
}

int init_eth(void)
{
	pci_dev *eth_dev;
	pci_dev *eth_dev2;
	int n_eths = 0;

	printf("Initializing ethernet: ");

	switch (systype())
	{
 	case SYSTYPE_3K:
		eth_dev  = pci_lookup_dev_by_bus(0, PCI_DEVFN(0x10, 0));
		eth_dev2 = pci_lookup_dev_by_bus(0, PCI_DEVFN(0x12, 0));
		break;

	case SYSTYPE_5K:
		eth_dev  = pci_lookup_dev(PCI_VENDOR_NS, PCI_DEVICE_NS_83815, NULL);
		eth_dev2 = pci_lookup_dev(PCI_VENDOR_NS, PCI_DEVICE_NS_83815, eth_dev);
		break;

	default:
		printf("failed\n");
		return -1;
	}

	if (!eth_dev && !eth_dev2) {
		printf("failed\n");
		return -1;
	}

	/* ethernet 0 */
	if (eth_dev)
		n_eths++;

	/* ethernet 1 */
	if (eth_dev2)
		n_eths++;

	printf("%d controller(s) found\n", n_eths);

	if (debug)
		print_eth_mac();

	configure_eth();

	return n_eths;
}

/* See if we have any Intel EEPRO 100 boards */
static void print_eepro100(void)
{
	pci_dev *enet, *poll = pci_devices;
       	unsigned short port;
	int i;
	unsigned char mac[6];

	while ((enet = pci_lookup_dev(PCI_VENDOR_INTEL, PCI_DEVICE_INTEL_82559ER, poll)) != NULL)
	{
		/* Get base I/O port */
		port = pci_dev_read_config_word(enet, 0x14);
		port &= 0xfffe;
		printf("  Intel 82559ER Found at port 0x%04x, ", port);
        
		eepro100_read_mac(port, mac);
		printf("MAC: ");
		for (i=0; i<5; i++) { 
			printf("%02x:",mac[i]); 
		}
		printf("%02x",mac[5]);
		printf("\n");

		poll=enet;
	}
}


static void print_macphyter(void)
{
	pci_dev *enet, *poll = pci_devices;
       	unsigned short port;
	int i;
	unsigned char mac[6];

 	/* See if we have any National Semi Macphyter dp83815 boards */
	while ((enet = pci_lookup_dev(PCI_VENDOR_NS, PCI_DEVICE_NS_83815, poll)) != NULL) {
		/* Get base I/O port */
		port = pci_dev_read_config_word(enet, 0x10);
		port &= 0xfffe;
		printf("  National Semiconductor DP83815 Found at port 0x%04x, ", port);
        
		macphyter_read_mac(port, mac);
		printf("MAC: ");
		for (i=0; i<5; i++) { 
			printf("%02x:",mac[i]); 
		}
		printf("%02x",mac[5]);
		printf("\n");

		poll=enet;
	}
}


int print_eth_mac(void)
{
	print_eepro100();
	print_macphyter();
	return 0;
}

char * eth_get_first_mac(void)
{
	pci_dev *dev;
	uint8 mac[6];
	static char macaddr[18];

	dev = pci_lookup_dev(PCI_VENDOR_INTEL, PCI_DEVICE_INTEL_82559ER, NULL);
	if (dev)
		eepro100_read_mac(dev->bar[1].base, mac);
	else {
		dev = pci_lookup_dev(PCI_VENDOR_NS, PCI_DEVICE_NS_83815, NULL);
		if (!dev)
			return NULL;
		macphyter_read_mac(dev->bar[0].base, mac);
	}

	sprintf(macaddr, "%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	return macaddr;
}

int eth_wol_setup(void)
{
	pci_dev *enet;
	unsigned short port;

	switch (systype()) {
	case SYSTYPE_3K:
		break;
									        	case SYSTYPE_5K:
		switch (boardtype()) {
		case BOARDTYPE_MONTEREY:
		case BOARDTYPE_ALPINE:
			enet = pci_lookup_dev(PCI_VENDOR_NS, 
				PCI_DEVICE_NS_83815, pci_devices);
			while (enet) {
				/* Get base I/O port */
				port = pci_dev_read_config_word(enet, 0x10);
				port &= 0xfffe;

				/* disable receiver */
				outl(0x8, port);
				/* ACK any interrupts */
				inl(port+0x10);
				inw(port+0xc8);
				/* set RXDP to 0 */
				outl(0, port+0x30);
				/* read wol status to clear */
				inl(port+0x40);
				/* clear PME status */
				outl(inl(port+0x3c), port+0x3c);
				/* enable receiver */
				outl(0x4, port);
				/* next enet device */
				enet = pci_lookup_dev(PCI_VENDOR_NS,
					PCI_DEVICE_NS_83815, enet);
			}
			break;

		default:
		}
	default:
	}
	return 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
