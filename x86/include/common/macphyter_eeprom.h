/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: macphyter_eeprom.h,v 1.1.1.1 2003/06/10 22:42:02 iceblink Exp $ */

/* Header for National Semiconductor dp83815 (MacPhyter) eeprom ruitines
 * Moshen Chan <mchan@cobalt.com>
 * June 02, 2000
 */

#ifndef MACPHYTER_EEPROM_H
#define MACPHYTER_EEPROM_H

int macphyter_read_mac(unsigned short ioaddr, unsigned char *mac_data);
int macphyter_read_byte(unsigned short ioaddr, unsigned char *data, int offset);
int macphyter_write_byte(unsigned short ioaddr, unsigned char data, int offset);
int macphyter_read_word(unsigned short ioaddr, unsigned short *data, int offset);
int macphyter_write_word(unsigned short ioaddr, unsigned short data, int offset);
int macphyter_write_eeprom(unsigned short ioaddr, unsigned short *eeprom_data);
int macphyter_write_mac(unsigned short ioaddr, unsigned char *mac);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
