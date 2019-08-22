/* $Id: eeprom_ops.c,v 1.1.1.1 2003/06/10 22:41:45 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

/*
 * eeprom_ops.c
 * seperated to save space
 */
#include <string.h>
#include <io.h>

#include "common/rom.h"
#include "common/pci.h"
#include "common/rammap.h"
#include "common/eeprom.h"
#include "common/spin.h"
#include "common/delay.h"
#include "common/systype.h"
#include "crfs_block.h"

void eeprom_list(void)
{
	int i;

	for (i = 0; i < EEPROM_MAX_BANKS; i++) {
		printf("  Flash Bank %d: ", i);
		if (!eeproms[i])
			printf("not installed.\n");
		else {
			printf("%s %dKB (%02x:%02x)\n",
			       eeproms[i]->name,
			       (unsigned int)(eeproms[i]->size >> 10),
			       (eeprom_ids[i]>>8) & 0xff,
			       (eeprom_ids[i]) & 0xff);
		}
	}
}

/* hook for erasing a full EEPROM - regardless of which chip it is */
int eeprom_erase_chip(int bank, int output)
{
	int ret = 0;

	eeprom_set_bank_page(bank, 0);

	printf("Erasing eeprom in bank %d: ", bank);

	/* each chip type has it's own special method */
	if (eeproms[bank]->erase_chip)
		ret = eeproms[bank]->erase_chip(output);  /* update screen */

	if (eeproms[bank]->reset)
		eeproms[bank]->reset();

	printf("%s\n", (ret) ? "failed" : "done");

	return ret;
}

/* hook for writing a full EEPROM - regardless of which chip it is */
int eeprom_write_chip(int bank, unsigned char *data, unsigned int size, int output)
{
	int ret = 0;
	
	eeprom_set_bank_page(bank, 0);
	printf("Writing eeprom in bank %d:   0x%08x:", bank, size);

	/* each chip type has it's own special method */
	if (eeproms[bank]->write_chip)
		ret = eeproms[bank]->write_chip(data, size, output);

	if (eeproms[bank]->reset)
		eeproms[bank]->reset();

	if (ret) {
		printf("0x%08x failed\n", ret-1);
		return ret;
	}
	printf("0x%08x done\n", size);
	printf("Verifying eeprom in bank %d: 0x%08x:", bank, size);

	if (eeproms[bank]->verify_chip)
		ret = eeproms[bank]->verify_chip(data, size, output);

	if (eeproms[bank]->reset)
		eeproms[bank]->reset();

	if (!ret)
		printf("0x%08x done\n", size);
	else
		printf("0x%08x failed\n", ret-1);

	return ret;
}

int eeprom_get_bank(void)
{
	return eeprom_bank;
}

int eeprom_get_page(void)
{
	return eeprom_page;
}

void eeprom_set_bank(int bank)
{
	eeprom_set_bank_page(bank, eeprom_page);
}

void eeprom_set_page(int page)
{
	eeprom_set_bank_page(eeprom_bank, page);
}

unsigned char eeprom_read_rom(unsigned int pos)
{
	if (pos < ROM_PAGESIZE)
		return rom[pos];

	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
