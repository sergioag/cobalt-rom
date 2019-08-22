/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: eeprom.h,v 1.1.1.1 2003/06/10 22:42:01 iceblink Exp $ */

/*
 * eeprom.h
 */

#ifndef COMMON_EEPROM_H
#define COMMON_EEPROM_H

/* this is the heart of supporting multiple chip types */
typedef struct {
	unsigned char vendor_id; 
	unsigned char device_id;
	int size;		/* in bytes - >>16 for # blocks */
	char *name;
	int (*reset)(void);	
	int (*erase_block)(int block, int output);
	int (*erase_chip)(int output);
	int (*write_block)(int block, char *data, int output); 
	int (*write_chip)(char *data, int size, int output);
	int (*verify_chip)(char *data, int size, int output);
} eeprom_type;

extern int eeprom_csum_valid;
extern eeprom_type *eeproms[];
extern eeprom_type eeprom_types[];
extern unsigned int eeprom_ids[]; /* exported for eeprom_ops.c */
extern volatile unsigned char *rom; /* exported for eeprom_ops.c */
extern int eeprom_bank; /* exported for eeprom_ops.c */
extern int eeprom_page; /* exported for eeprom_ops.c */

int  eeprom_init(void);
void eeprom_list(void);

int  eeprom_erase_chip(int bank, int output);
int  eeprom_write_chip(int bank, unsigned char * data, unsigned int size, int output);

int  eeprom_set_bank_page(int bank, int page);
int  eeprom_get_bank(void);
void eeprom_set_bank(int bank);
int  eeprom_get_page(void);
void eeprom_set_page(int page);

unsigned int  eeprom_get_romid(int bank);
unsigned char eeprom_read_rom(unsigned int pos);
unsigned int  eeprom_csum(void);

#define EEPROM_MAX_BANKS	2
#define ROM_PAGESIZE		0x10000  /* 64K */

/* ServerWorks EEPROM ports */
#define SW_MISC_CONTROL	        0xC6F
#define SW_EEPROM_CSE		0x40

#define SW_BLACKBOX_CONTROL	0xC06
#define SW_BLACKBOX_FWE		0x02

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
