/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: eepro100_eeprom.h,v 1.1.1.1 2003/06/10 22:42:01 iceblink Exp $ */

/* Header for Intel EEpro100 EEPROM ruitines
 * Moshen Chan <mchan@cobalt.com>
 * June 02, 2000
 */

#ifndef EEPRO100_EEPROM_H
#define EEPRO100_EEPROM_H

int eepro100_read_mac(unsigned short ioaddr, unsigned char *mac_data);
int eepro100_read_byte(unsigned short ioaddr, unsigned char *data, int offset);
int eepro100_write_byte(unsigned short ioaddr, unsigned char data, int offset);
int eepro100_read_word(unsigned short ioaddr, unsigned short *data, int offset);
int eepro100_write_word(unsigned short ioaddr, unsigned short data, int offset);
int eepro100_write_eeprom(unsigned short ioaddr, unsigned short *eeprom_data);
int eepro100_write_mac(unsigned short ioaddr, unsigned char *mac);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
