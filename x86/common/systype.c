/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: systype.c,v 1.1.1.1 2003/06/10 22:41:54 iceblink Exp $
 *
 * allows one to distinguish between the different systems types
 *
 */

#include <string.h>
#include <stdlib.h>
#include <printf.h>

#include "common/systype.h"
#include "common/pci.h"

#include "common/rom.h"

static unsigned int cur_systype;

static int systype_check_3k(void);
static int systype_check_5k(void);
static int boardtype_detect_3k(void);
static int boardtype_detect_5k(void);

void 
systype_init(void)
{
	/* detect the current system and board */
	if (systype_check_3k()) {
		cur_systype = SYSTYPE_3K;
		cur_systype |= boardtype_detect_3k();
	} else if (systype_check_5k()) {
		cur_systype = SYSTYPE_5K;
		cur_systype |= boardtype_detect_5k();
	} else {
		cur_systype = SYSTYPE_UNKNOWN | BOARDTYPE_UNKNOWN;
	}
}

unsigned int 
systype(void)
{
	return (cur_systype & 0xffff0000);
}

unsigned int
boardtype(void)
{
	return (cur_systype & 0x0000ffff);
}

int 
systype_is_3k(void)
{
	return systype() == SYSTYPE_3K;
}

int 
systype_is_5k(void)
{
	return systype() == SYSTYPE_5K;
}

/*
 * these detect routines have to use the bus/dev pci access as 
 * pci_init has not been called yet
 */
static int 
systype_check_3k(void)
{
	return (pci_read_config_word(0,PCI_DEVFN(0,0),0) == PCI_VENDOR_AL) &&
	    (pci_read_config_word(0,PCI_DEVFN(0,0),2) == PCI_DEVICE_AL_M1541);
}

static int 
systype_check_5k(void)
{
	return (pci_read_config_word(0,PCI_DEVFN(0,0),0) == PCI_VENDOR_SERVERWORKS) &&
	       (pci_read_config_word(0,PCI_DEVFN(0,0),2) == PCI_DEVICE_SERVERWORKS_LE);
}

static int
boardtype_detect_3k(void)
{
	unsigned int confreg, i;

	/* pci structures are not up at this stage - but we know PMU is 0:3 */

	/* Momentarily change DOGB to input */
	confreg = pci_read_config(0, PCI_DEVFN(3,0), 0x7d, 1);
	pci_write_config(0, PCI_DEVFN(3,0), 0x7d, confreg & (~0x20), 1);

	/* read the GPIO register */
	i = pci_read_config(0, PCI_DEVFN(3,0), 0x7f, 1);

	/* change DOGB back to output */
	confreg = pci_read_config(0, PCI_DEVFN(3,0), 0x7d, 1);
	pci_write_config(0, PCI_DEVFN(3,0), 0x7d, confreg | 0x20, 1);

	/* RaQ3/4 boards have DOGB (0x20) pulled high, Qube3 has DOGB low */
	if (i & 0x20) {
		return BOARDTYPE_PACIFICA;
	} else {
		return BOARDTYPE_CARMEL;
	}
}

static int
boardtype_detect_5k(void)
{
    if((pci_readw( 0x0, PCI_DEVFN( 0xf, 0x0), 0x0 ) == PCI_VENDOR_SERVERWORKS ) &&
	(pci_readw( 0x0, PCI_DEVFN( 0xf, 0x0), 0x2 ) ==  PCI_DEVICE_SERVERWORKS_OSB4 ) )
	return BOARDTYPE_MONTEREY;
    else if((pci_readw( 0x0, PCI_DEVFN( 0xf, 0x0), 0x0 ) == PCI_VENDOR_SERVERWORKS ) &&
	    (pci_readw( 0x0, PCI_DEVFN( 0xf, 0x0), 0x2 ) ==  PCI_DEVICE_SERVERWORKS_CSB5 ) )
	return BOARDTYPE_ALPINE;
    else
	return BOARDTYPE_UNKNOWN;
}

char *
systype_str(void)
{
	if (systype_is_3k()) {
		return "3000 series system";
	} else if (systype_is_5k()) {
		return "5000 series system";
	} else {
		return "Unknown system";
	}
}

char *
boardtype_str(void)
{
	if (systype_is_3k()) {
		if (boardtype() == BOARDTYPE_PACIFICA) {
			return "Version 1 board";
		} else if (boardtype() == BOARDTYPE_CARMEL) {
			return "Version 2 board";
		}
	} else if (systype_is_5k()) {
		if (boardtype() == BOARDTYPE_MONTEREY) {
			return "Version 1 board";
		}
		if (boardtype() == BOARDTYPE_ALPINE) {
			return "Version 2 board";
		}
	} 

	return "Unknown board";
}

/* 12-byte Product Identifier used for MP table */
char *
systype_productid_str(void)
{
	if (systype_is_5k()) {
		if (boardtype() == BOARDTYPE_MONTEREY)
			return "5000 RaQ-XTR";
		else if (boardtype() == BOARDTYPE_ALPINE)
			return "5000 Alpine ";
	}
	return "OEM Unknown ";
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
