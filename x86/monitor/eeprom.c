/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: eeprom.c,v 1.1.1.1 2003/06/10 22:42:19 iceblink Exp $
 */

#include <printf.h>
#include <string.h>

#include "common/rom.h"
#include "common/boot.h"
#include "common/rammap.h"
#include "common/eeprom.h"
#include "common/serial.h"

#include "eeprom.h"

int display_romid_func(int argc, char *argv[] __attribute__ ((unused)))
{
	unsigned int id;
	
	if (argc != 1) {
		return EBADNUMARGS;
	}
	
	id = eeprom_get_romid( eeprom_get_bank() );
	
	printf("Vendor ID: 0x%2x\n", id >> 8);
	printf("Device ID: 0x%2x\n", id & 0xff);
	
	return 0;
}

int set_bank_func(int argc, char *argv[])
{
	unsigned long bank, page;
	
	if(argc != 3)
		return EBADNUMARGS;
		
	if((stoli(argv[1], &bank) != 0) || (stoli(argv[2], &page) != 0)) 
		return EBADNUM;

    	if (bank >= EEPROM_MAX_BANKS || !eeproms[bank])
		return EBADARG;

	eeprom_set_bank_page(bank, page);
	
	return 0;
}

int erase_eeprom_func(int argc, char *argv[])
{
	unsigned long bank;
	
	if (argc != 2)
		return EBADNUMARGS;
		
	if ((stoli(argv[1], &bank) != 0))
		return EBADNUM;

    	if (bank >= EEPROM_MAX_BANKS || !eeproms[bank])
		return EBADARG;

	eeprom_erase_chip(bank, 1);

	return 0;
}

int write_eeprom_func(int argc, char *argv[])
{
	unsigned long bank, len;
	unsigned char in;
	unsigned int  sz = 0;
	unsigned char *data = (unsigned char *)RAM_SERIAL_LOAD;
	
	if (argc >= 2 && (stoli(argv[1], &bank) != 0))
		return EBADNUM;
	else
		bank = 0;

	printf("Re-Initializing flash: ");
	if (eeprom_init() < 0) {
		printf("failed\n");
		return 0;
	}
	printf("done\n");

    	if (bank >= EEPROM_MAX_BANKS || !eeproms[bank])
		return EBADARG;

	if (argc == 3 && (stoli(argv[2], &len) != 0))
		return EBADNUM;
	else
		len = eeproms[bank]->size;

	if (!len)
		return 0;

	printf("EEPROM in bank %d is %dKB (%s)\n", (unsigned int) bank,
	       (unsigned int) (len>>10), eeproms[bank]->name);

	/* read rom image from serial port */
	printf("Loading ROM image from serial port...\n");
	serial_inb(&in);
	do {
		*(data+sz) = in;
		if ((++sz % 0x400) == 0)
			printf("\r%dk", sz>>10);
	} while (!serial_inb_timeout(&in, 8*BOGO_SEC) 
		 && (unsigned long)(data+sz) < RAM_SERIAL_LOAD_MAX);
	printf("\r%dk done\n", sz>>10);

	/* set surrounding mem to 0xff */
	if (len-sz)
		memset(data+sz, ~0, len-sz);

	printf("Press any key to abort...");
	for (sz=3; sz>0; sz--) {
		printf(" %d", sz);
		if (!serial_inb_timeout((char *)&in, 2*BOGO_SEC)) {
			printf(" - ABORTED\n");
			return 0;
		}
	}
	printf("\n");

	if (eeprom_erase_chip(bank, 1))
		return 0;

	eeprom_write_chip(bank, data, len, 1);

	return 0;
}

int reinit_eeprom_func(int argc, char *argv[] __attribute__((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;

	printf("Initializing flash: ");

	if (eeprom_init() < 0) {
		printf("failed\n");
		return 0;
	}

	printf("done\n");
	eeprom_list();

	return 0;
}

int read_eeprom_func(int argc, char *argv[])
{
	int bank, len;
	int page;
	int i;
	int oldbank = eeprom_get_bank(); 
	int oldpage = eeprom_get_page();
	unsigned char *data = (unsigned char *)RAM_SERIAL_LOAD;

	if (argc != 2 && argc != 3) {
		return EBADNUMARGS;
	}
		
	if (stoli(argv[1], (unsigned long *)&bank) != 0) {
		return EBADNUM;
	}

    	if (bank >= EEPROM_MAX_BANKS || !eeproms[bank]) {
		return EBADARG;
	}

	/* 3 args == they supplied a length */
	if (argc == 3 && (stoli(argv[2], (unsigned long *)&len) != 0)) {
		return EBADNUM;
	} else if (argc == 2) {
		/* 2 args == fill in the length */
		len = eeproms[bank]->size;
	}

	if (!eeproms[bank] || len > eeproms[eeprom_get_bank()]->size) {
		return 1;
	}
		
	printf("Reading eeprom in bank %d:   0x%08x:", bank, len);
	printf("0x00000000");

	for (page = 0; page < (len>>16); page++) {
		eeprom_set_bank_page(eeprom_get_bank(), page);

		for (i = 0; i < ROM_PAGESIZE; i++) {
			data[(page*ROM_PAGESIZE) + i] = eeprom_read_rom(i);

			if (!(i&0xfff)) {
				printf("\b\b\b\b\b\b\b\b");
				printf("%08x", (page*ROM_PAGESIZE) + i);
			}
		}
	}
	serial_load_size = len;

	printf("\b\b\b\b\b\b\b\b");
	printf("%08x", len);
	printf(" done\n");
	eeprom_set_bank_page(oldbank, oldpage);
	return 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
