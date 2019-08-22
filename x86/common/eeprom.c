/* $Id: eeprom.c,v 1.1.1.1 2003/06/10 22:41:45 iceblink Exp $ */
/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

/*
 * eeprom.c
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

#define MAX_TIMEOUT	(int)0x07ffffff

int eeprom_csum_valid;
static int eeprom_ttl_size;
static int eeprom_fs_offset;

static int generic_verify_chip(char *, int, int);

/* AMD routines */
static int amd_reset(void);
static int amd_erase_block(int, int);
static int amd_erase_chip(int);
static int amd_write_block(int, char *, int);
static int amd_write_chip(char*, int, int);
static int amd_verify_chip(char *, int, int);

/* intel routines */
static int intel_reset(void);
static int intel_erase_block(int, int);
static int intel_erase_chip(int);
static int intel_write_block(int, char *, int);
static int intel_write_chip(char*, int, int);
static int intel_verify_chip(char *, int, int);

volatile unsigned char *rom = (unsigned char *)0xffff0000;
static volatile unsigned char *flat_rom_base;

#define FLAT_ROM_BASE_ADDR ((unsigned char *)0xffc00000)

int eeprom_bank = -1;
int eeprom_page = -1;

eeprom_type *eeproms[EEPROM_MAX_BANKS];
unsigned int eeprom_ids[EEPROM_MAX_BANKS];

/* a list of EEPROMs we can support */
eeprom_type eeprom_types[] = {
	{ 
	  0x01, 0xd5, 0x00100000, "AMD AM29F080B",
	  amd_reset, amd_erase_block, amd_erase_chip,
	  amd_write_block, amd_write_chip, amd_verify_chip
	},
	{ 
	  0x01, 0xad, 0x00200000, "AMD AM29F016B",
	  amd_reset, amd_erase_block, amd_erase_chip,
	  amd_write_block, amd_write_chip, amd_verify_chip
	},
	{ 
	  0x01, 0x3d, 0x00200000, "AMD AM29F017B - untested",
	  amd_reset, amd_erase_block, amd_erase_chip,
	  amd_write_block, amd_write_chip, amd_verify_chip
	},
	{ 
	  0x01, 0x41, 0x00400000, "AMD AM29F032B",
	  amd_reset, amd_erase_block, amd_erase_chip,
	  amd_write_block, amd_write_chip, amd_verify_chip
	},
	{ 
	  /* Sharp is identical to Intel, even vendor and device IDs */
	  0x89, 0xa6, 0x00100000, "Intel E28F008S5/Sharp LM28F008SCT",
	  intel_reset, intel_erase_block, intel_erase_chip,
	  intel_write_block, intel_write_chip, intel_verify_chip
	},
	{ 
	  0x89, 0xaa, 0x00200000, "Intel E28F016S5/Sharp LM28016SCT",
	  intel_reset, intel_erase_block, intel_erase_chip,
	  intel_write_block, intel_write_chip, intel_verify_chip
	},
	{
	  /* ST Micro programs just like AMD */
	  0x20, 0xf1, 0x00100000, "ST Microelectronics M29F080A",
	  amd_reset, amd_erase_block, amd_erase_chip,
	  amd_write_block, amd_write_chip, amd_verify_chip
	},
	{
	  /* Hyundai programs just like AMD */
	  0xad, 0xd5, 0x00100000, "Hyundai HY29F080",
	  amd_reset, amd_erase_block, amd_erase_chip,
	  amd_write_block, amd_write_chip, amd_verify_chip
	},
	{ 
	  0x00, 0x00, 0x00000000, "Unknown flash module",
	  NULL, NULL, NULL, NULL, NULL, NULL
	}
};

int eeprom_init(void)
{
	int i,j;
	unsigned int romid;

	switch (systype()) {
	    case SYSTYPE_5K:
                /* Setup for paging
                 * set OSB4, reg 0xF50, 0xF51, 0xF52
                 * reg 0xF50 = low address
                 * reg 0xF51 = hi address
                 * reg 0xF52 = range
                 */
		if( boardtype() == BOARDTYPE_MONTEREY )
		{
                    outb(0x00, 0xF50);
                    outb(0x09, 0xF51);
                    outb(0xfe, 0xF52);
		    eeprom_fs_offset = 0x10000;
		}
		else if( boardtype() == BOARDTYPE_ALPINE )
		{
		    eeprom_fs_offset = 0x0;
		    flat_rom_base = FLAT_ROM_BASE_ADDR; 
		    rom = flat_rom_base; /* set for 4M rom for now.  
					  * This will alias to the bottom of
					  * whatever rom we have in there.
					  */
		}
		else break;

               /* 
		 * This is a little dangerous, but we can fix it later 
                 * so that writes aren't allowed. 
                 * Enable ROM writes 
                 */
                outb(SW_EEPROM_CSE, SW_MISC_CONTROL);

		if (boardtype() == BOARDTYPE_MONTEREY)
			outb(SW_BLACKBOX_FWE, SW_BLACKBOX_CONTROL);

		break;

	    case SYSTYPE_3K:
		eeprom_fs_offset = 0x10000;
		break;

	    default:
		return -1;
	}


	for (i = 0; i < EEPROM_MAX_BANKS; i++) {
	    eeproms[i] = NULL;
	    
	    
	    if( eeprom_set_bank_page(i, 0) < 0) 
		continue;
	    
	    romid = eeprom_get_romid(i);
	    
		/* nothing there ? */
	    if (romid == 0xffff)
		continue;
	    
	    eeprom_ids[i] = romid;
	    
	    j=0;
	    while (eeprom_types[j].size) {
		    /* find it in the list */
		if ((eeprom_types[j].vendor_id == (0xff & (romid >> 8))) &&
		    (eeprom_types[j].device_id == (0xff & (romid)))) {
		    
		    eeproms[i] = &eeprom_types[j];
		    eeprom_ttl_size += eeproms[i]->size;
		    if (eeproms[i]->reset)
			eeproms[i]->reset();
		    break;
		}
		j++;
	    }
	    if (!eeproms[i])
		eeproms[i] = &eeprom_types[j];

	    if( boardtype() == BOARDTYPE_ALPINE )
	    {
		rom = flat_rom_base = (unsigned char *) ~(eeproms[i]->size - 1);
	    }
	}
	eeprom_set_bank_page(0, 0);

	return 0;
}

unsigned int eeprom_get_romid(int bank)
{
	unsigned int id;
	
	eeprom_set_bank_page(bank, 0);

	/* AMD requires the first two, Intel only the third */
	rom[0x555] = 0xaa;
	rom[0x2aa] = 0x55;
	rom[0x555] = 0x90;
	
	/* thankfully both AMD and Intel store their info the same way */
	id = (rom[0x00] << 8) + rom[0x01];
	
	if (eeproms[bank] && eeproms[bank]->reset)
		eeproms[bank]->reset();
	
	return id;
}

/* this should be OK - independent of the flash chip */
int eeprom_set_bank_page(int bank, int page)
{
	unsigned int flags;

	if (bank >= EEPROM_MAX_BANKS)
		return -1;

	if( (boardtype() == BOARDTYPE_ALPINE) && bank)
	    return -1; /* Apline has only one rom */

	if (eeprom_bank==bank && eeprom_page==page)
		return 0;
    
	switch (systype())
	{
	case SYSTYPE_3K:
		flags = pci_read_config_word(0, PCI_DEVFN(3,0), 0xb8);
		flags &= 0xff80;
		flags |= (page & 0x3f) | ((bank&0x1)<<6);
		pci_write_config_word(0, PCI_DEVFN(3,0), 0xb8, flags);
		break;

	case SYSTYPE_5K:
	    switch( boardtype() )
	    {
		case BOARDTYPE_MONTEREY:
		    pci_write_config(0, PCI_DEVFN(0xf,0), 0x72, 0x08, 1);
		    outb(((page & 0x3f) | ((bank&0x1)<<6)),0x900);
		    pci_write_config(0, PCI_DEVFN(0xf,0), 0x72, 0x0c, 1);
		    break;
		case BOARDTYPE_ALPINE:
		    rom = flat_rom_base + (page * 0x10000);
		    break;
	    }
	    break;
	}
	/* wait for 2 millisecs
	 * that should be enough to stabilize the page register
	 */
	delay(2);
	eeprom_bank = (char)bank;
	eeprom_page = (char)page;

	return 0;
}

/* AMD specific routines */
static int amd_reset(void)
{
	rom[0x00] = 0xf0;
	return 0;
}

static int amd_erase_chip(int output)
{
	int timeout = 0;
	int oldbank = eeprom_bank; 
	int oldpage = eeprom_page;
	int r = 0;
 
	if (output) {
		spin_reset();
		printf("-");
	}

	/* AMD chip erase signal */
	rom[0x555] = 0xaa;
	rom[0x2aa] = 0x55;
	rom[0x555] = 0x80;
	rom[0x555] = 0xaa;
	rom[0x2aa] = 0x55;
	rom[0x555] = 0x10;
	
	/* wait for the high bit of the first byte to signal done */
	while (!(rom[0x0]&0x80) && !r) {
		/* test 0x80 again because the end result is 0xff */
		if ((rom[0x0]&0x20) && !(rom[0x0]&0x80))
			r = -1;
		if (output && !(++timeout & 0x7fff))
			spin();
		if (timeout == MAX_TIMEOUT)
			r = -1;
	}

	/* cleanup output */
	if (output)
		printf("\b \b");

	amd_reset();
	eeprom_set_bank_page(oldbank, oldpage);

	return r;
}

static int amd_erase_block(int block, int output)
{
	int timeout = 0;
	int oldbank = eeprom_bank; 
	int oldpage = eeprom_page;
	int r = 0;

	if (!eeproms[eeprom_bank] || block >= eeproms[eeprom_bank]->size>>16)
		return -1;

	/* AMD block erase signal */
	rom[0x555] = 0xaa;
	rom[0x2aa] = 0x55;
	rom[0x555] = 0x80;
	rom[0x555] = 0xaa;
	rom[0x2aa] = 0x55;
	rom[block<<16] = 0x30;

	/* wait for the high bit of the first byte to signal done */
	while (!(rom[0x0]&0x80) && !r) {
		/* test 0x80 again because the end result is 0xff */
		if ((rom[0x0] & 0x20) && !(rom[0x0] & 0x80))
			r = -1;
		if (output && !(++timeout & 0x7fff))
			spin();
		if (timeout == MAX_TIMEOUT)
			r = -1;
	}

	eeprom_set_bank_page(oldbank, oldpage);
	return r;
}

static int amd_write_chip(char *data, int size, int output)
{
	int page;
	int oldbank = eeprom_bank; 
	int oldpage = eeprom_page;
	int r = 0;

	if (!eeproms[eeprom_bank] || size > eeproms[eeprom_bank]->size)
		return 1;
	if (output)
		printf("0x00000000");

	for (page = 0; page < (size>>16) && !r; page++) {
		eeprom_set_bank_page(eeprom_bank, page);
		r = amd_write_block(page, data+(page*ROM_PAGESIZE), output);

		if (output) {
			printf("\b\b\b\b\b\b\b\b");
			printf("%08x", (page+1)*ROM_PAGESIZE);
		}
	}

	if (output) {
		printf("\b\b\b\b\b\b\b\b\b\b");
		printf("          ");
		printf("\b\b\b\b\b\b\b\b\b\b");
	}

	amd_reset();
	eeprom_set_bank_page(oldbank, oldpage);

	return r;
}

static int amd_write_block(int block, char *data, int output)
{
	int timeout = 0;
	unsigned char c;
	int i;
	int oldbank = eeprom_bank; 
	int oldpage = eeprom_page;
	int r = 0;

	if (!eeproms[eeprom_bank] || block >= eeproms[eeprom_bank]->size>>16)
		return -1;

	eeprom_set_bank_page(eeprom_bank, block);

	for (i = 0; i < ROM_PAGESIZE && !r; i++) {
		timeout = 0;
		c = data[i] & 0x80;
	
		/* magic write sequence */
		rom[0x555] = 0xaa;
		rom[0x2aa] = 0x55;
		rom[0x555] = 0xa0;

		rom[i] = data[i];
			
		/* wait for the high bit to turn up correctly */
		while ((rom[i]&0x80) != c && !r) {

			/* check again - book says so */
			if ((rom[i] & 0x20) && ((rom[i]&0x80) != c))
				r = i+1;
			if (++timeout == MAX_TIMEOUT)
				r = -1;
		}

		/* counter update every 0x1000 (4k) */
		if (output && !(i&0xfff)) {
			printf("\b\b\b\b\b\b\b\b");
			printf("%08x", (block*ROM_PAGESIZE) + i);
		}
	}

	eeprom_set_bank_page(oldbank, oldpage);

	return r;
}

static int amd_verify_chip(char *data, int size, int output)
{
	amd_reset();
	return generic_verify_chip(data, size, output);
}

/* Intel routines */
static int intel_reset(void)
{
	rom[0x0] = 0xff;
	return 0;
}

static int intel_erase_chip(int output)
{
	int block;
	int oldbank = eeprom_bank; 
	int oldpage = eeprom_page;
	int r = 0;

	/* monterey doesn't have the needed Vpp pin connected */
	if (systype() == SYSTYPE_5K && boardtype() == BOARDTYPE_MONTEREY) {
		printf("not supported for this flash module or system - ");
		return -1;
	}

	if (output) {
		printf("block 00 -");
		spin_reset();
	}

	for (block = 0; block < (eeproms[eeprom_bank]->size>>16) && !r; block++) {
		r = intel_erase_block(block, output);
		if (output) {
			printf("\b\b\b\b%s%d  ", (block+1)<10?"0":"", block+1);
			spin();
		}
	}

	if (output) {
		printf("\b\b\b\b\b\b\b\b\b\b");
		printf("          ");
		printf("\b\b\b\b\b\b\b\b\b\b");
	}

	intel_reset();
	eeprom_set_bank_page(oldbank, oldpage);

	return r;
}

static int intel_erase_block(int block, int output)
{
	int r = 0, timeout = 0;
	int oldbank = eeprom_bank; 
	int oldpage = eeprom_page;

	if (!eeproms[eeprom_bank] || block >= eeproms[eeprom_bank]->size>>16)
		return -1;

	/* monterey doesn't have the needed Vpp pin connected */
	if (systype() == SYSTYPE_5K && boardtype() == BOARDTYPE_MONTEREY) {
		printf("not supported for this flash module or system - ");
		return -1;
	}

	eeprom_set_bank_page(eeprom_bank, block);

	/* intel block erase signal - write to any byte inside this block */
	rom[0x00] = 0x20;
	rom[0x00] = 0xd0;

	/* wait for the high bit of the status reg to signal done */
	while (!(rom[0x0]&0x80) && !r) {
		if (rom[0x0]&0x20) {
			/* clear status reg */
			rom[0x0] = 0x50;
			r = -1;
		}
		if (output && !(++timeout & 0x7fff))
			spin();
		if (timeout == MAX_TIMEOUT)
			r = -1;
	}

	eeprom_set_bank_page(oldbank, oldpage);

	return r;
}

static int intel_write_chip(char *data, int size, int output)
{
	int page, r = 0;
	int oldbank = eeprom_bank; 
	int oldpage = eeprom_page;

	if (!eeproms[eeprom_bank] || size > eeproms[eeprom_bank]->size)
		return 1;

	/* monterey doesn't have the needed Vpp pin connected */
	if (systype() == SYSTYPE_5K && boardtype() == BOARDTYPE_MONTEREY) {
		printf("not supported for this flash module or system - ");
		return 1;
	}

	if (output)
		printf("0x00000000");

	for (page = 0; page < (size>>16) && !r; page++) {
		eeprom_set_bank_page(eeprom_bank, page);
		r = intel_write_block(page, data+(page*ROM_PAGESIZE), output);

		if (output) {
			printf("\b\b\b\b\b\b\b\b");
			printf("%08x", (page+1)*ROM_PAGESIZE);
		}
	}

	if (output) {
		printf("\b\b\b\b\b\b\b\b\b\b");
		printf("          ");
		printf("\b\b\b\b\b\b\b\b\b\b");
	}

	intel_reset();
	eeprom_set_bank_page(oldbank, oldpage);

	return r;
}

static int intel_write_block(int block, char *data, int output)
{
	int i, r = 0, timeout = 0;
	int oldbank = eeprom_bank; 
	int oldpage = eeprom_page;

	if (!eeproms[eeprom_bank] || block >= eeproms[eeprom_bank]->size>>16)
		return -1;

	/* monterey doesn't have the needed Vpp pin connected */
	if (systype() == SYSTYPE_5K && boardtype() == BOARDTYPE_MONTEREY) {
		printf("not supported for this flash module or system - ");
		return 1;
	}

	oldpage = eeprom_page;
	eeprom_set_bank_page(eeprom_bank, block);

	for (i = 0; i < ROM_PAGESIZE; i++) {
		timeout = 0;
	
		/* intel write sequence */
		rom[i] = 0x40;
		rom[i] = data[i];
			
		/* wait for the high bit to turn up correctly */
		while (!(rom[0x0]&0x80) && !r) {
			if (rom[0x0]&0x10) {
				/* clear SR */
				rom[0x0] = 0x50;
				r = i+1;
			}
			if (++timeout == MAX_TIMEOUT)
				r = -1;
		}

		/* counter update every 0x1000 (4k) */
		if (output && !(i&0xfff)) {
			printf("\b\b\b\b\b\b\b\b");
			printf("%08x", (block*ROM_PAGESIZE) + i);
		}
	}

	eeprom_set_bank_page(oldbank, oldpage);

	return r;
}

static int intel_verify_chip(char *data, int size, int output)
{
	intel_reset();
	return generic_verify_chip(data, size, output);
}

/* generic function for just verifying ROM against data */
static int generic_verify_chip(char *data, int size, int output)
{
	int page, i;
	int oldbank = eeprom_bank; 
	int oldpage = eeprom_page;

	if (!eeproms[eeprom_bank] || size > eeproms[eeprom_bank]->size)
		return 1;

	if (output)
		printf("0x00000000");

	for (page = 0; page < (size>>16); page++) {
		eeprom_set_bank_page(eeprom_bank, page);
		for (i = 0; i < ROM_PAGESIZE; i++) {
			unsigned char d;
			if (output && !(i&0xfff)) {
				printf("\b\b\b\b\b\b\b\b");
				printf("%08x", (page*ROM_PAGESIZE) + i);
			}

			d = (unsigned char) data[(page*ROM_PAGESIZE) + i];
			if (rom[i] != d) {
				if (output) {
					printf("\b\b\b\b\b\b\b\b\b\b");
					printf("          ");
					printf("\b\b\b\b\b\b\b\b\b\b");
				}
				eeprom_set_bank_page(oldbank, oldpage);
				return ((page*ROM_PAGESIZE) + i + 1);
			}
		}
	}

	if (output) {
		printf("\b\b\b\b\b\b\b\b\b\b");
		printf("          ");
		printf("\b\b\b\b\b\b\b\b\b\b");
	}

	eeprom_set_bank_page(oldbank, oldpage);

	return 0;
}

int read_block(uint16 block_ndx, void *block)
{
	switch (systype())
	{
	case SYSTYPE_3K: {
		int rom_addr, bank;
	
		/* calc offset in rom address space */
		rom_addr = (block_ndx*CRFS_BLOCK_SIZE) + eeprom_fs_offset;
	
		/* first figure out the which bank to use */
		if (rom_addr < eeproms[0]->size) {
			bank = 0;
		}
		else if (eeproms[1] && (rom_addr < (eeproms[0]->size + eeproms[1]->size))) {
			rom_addr -= eeproms[0]->size;
			bank = 1;
		}
		else return -1;

		eeprom_set_bank_page(bank, rom_addr >> 16);
		memcpy(block, (void *) (rom+(rom_addr&0xffff)), CRFS_BLOCK_SIZE);
		break;
	}
	case SYSTYPE_5K: {
		int rom_addr, bank;
	
		/* calc offset in rom address space */
		rom_addr = (block_ndx*CRFS_BLOCK_SIZE) + eeprom_fs_offset;
	
		/* first figure out the which bank to use */
                // FIXME... Need double flash support - TJS
		bank = 0;
		
		eeprom_set_bank_page(bank, rom_addr >> 16);
		memcpy(block, (void *) (rom+(rom_addr&0xffff)), CRFS_BLOCK_SIZE);

		break;
	}
	default:
		printf("Invalid system type!\n");
		break;
	}

#ifdef FS_DEBUG
	printf("%d;0x%0x\n", (block_ndx >> 4) + 1,
	       (void *) (rom+((block_ndx&0xf)*CRFS_BLOCK_SIZE)));
#endif
	return 0;
}

int write_block(uint16 block_ndx, void *block)
{
	int rom_addr, bank;
	
	/* calc offset in rom address space */
	rom_addr = (block_ndx*CRFS_BLOCK_SIZE) + eeprom_fs_offset;
	
	/* first figure out the which bank to use */
	if (rom_addr < eeproms[0]->size)
		bank = 0;
	else if (eeproms[1] && (rom_addr < (eeproms[0]->size + eeproms[1]->size))) {
		rom_addr -= eeproms[0]->size;
		bank = 1;
	} else
		return -1; /* does not fit in the rom */
		
	eeprom_set_bank_page(bank, rom_addr >> 16);
		
	if (eeproms[bank]->erase_block && eeproms[bank]->write_block) {
		int r;

		r = eeproms[bank]->erase_block(rom_addr>>16, 0);
		if (eeproms[bank]->reset)
			eeproms[bank]->reset();
		if (r)
			return r;

		r = eeproms[bank]->write_block(rom_addr>>16, (char *)block, 0);
		if (eeproms[bank]->reset)
			eeproms[bank]->reset();

		return r;
	}

	return -1;
}

#if 0
unsigned int
eeprom_csum(void)
{
	int page, i;
	int oldbank = eeprom_bank;
	int oldpage = eeprom_page;
	unsigned int csum = 0;
	unsigned int stored_csum = 0;

	/* read the whole thing - every bank, every page */
	for (page = 0; page < (eeprom_ttl_size>>16); page++) {
		eeprom_set_bank_page(eeprom_bank, page);

		for (i = 0; i < ROM_PAGESIZE; i++) {
			/* byte additive checksum */
			csum += rom[i]; 
		}
	}

	/* check if it is valid */
	eeprom_set_bank_page(0, 0);
	stored_csum = *(unsigned int *)(rom + ROM_CSUM_OFFSET);
	eeprom_csum_valid = (csum == stored_csum);

	eeprom_set_bank_page(oldbank, oldpage);

	return csum;
}

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
