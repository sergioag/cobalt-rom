/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * Moshen Chan <mchan@cobalt.com>
 * June 02, 2000
 */
#include <string.h>
#include <printf.h>

#include "common/rom.h"
#include "common/pci.h"
#include "common/eth.h"
#include "common/systype.h"
#include "common/eepro100_eeprom.h"
#include "common/macphyter_eeprom.h"

#include "eth.h"

int read_mac_func (int argc, char *argv[] __attribute__ ((unused)) )
{

    if (argc != 1) {
	return EBADNUMARGS;
    }
    
    /* See if we have any Intel EEPRO 100 boards */
    print_eth_mac();
    return 0;
}

int write_mac_func (int argc, char *argv[] __attribute__ ((unused)))
{
	unsigned char newmac[6], oldmac[6];
	unsigned long macbyte, port;
	int i;

	if (argc != 8)
		return EBADNUMARGS;

	if (stoli(argv[1], &port) != 0)
		return EBADNUM;

	for (i=2; i<8; i++) {
		if (stoli(argv[i], &macbyte) != 0)
			return EBADNUM;
		newmac[i-2] = (unsigned char) macbyte;
	}

	switch (systype())
	{
	case SYSTYPE_3K:
		return 0;
		break;

	case SYSTYPE_5K:
		macphyter_read_mac((unsigned short)port, oldmac);
		printf("  old MAC: %02x", oldmac[0]);
		for (i=1; i<6; i++)
			printf(":%02x", oldmac[i]);
		printf("\n");

		printf("  new MAC: %02x", newmac[0]);
		for (i=1; i<6; i++)
			printf(":%02x", newmac[i]);
		printf("\n");

		printf("Writing to National DP83815 at port %04x... ", (unsigned short)port);
		i = macphyter_write_mac((unsigned short)port, newmac);
		if (i < 0)
			printf("failed\n");
		else
			printf("done\n");

		macphyter_read_mac((unsigned short)port, oldmac);
		printf("  new MAC: %02x", oldmac[0]);
		for (i=1; i<6; i++)
			printf(":%02x", oldmac[i]);
		printf("\n");

		break;
	}

	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
