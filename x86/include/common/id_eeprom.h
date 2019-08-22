/*
 * $Id: id_eeprom.h,v 1.1.1.1 2003/06/10 22:42:02 iceblink Exp $
 *
 * Copyright (C) 2001 Sun Microsystem, Inc.
 * All Rights Reserved
 *
 * Serial eeprom ID chip
 */

#ifndef __common_id_eeprom_h__
#define __common_id_eeprom_h__

#include "common/i2c.h"

#define ID_EEPROM_DEV          I2C_DEV_AT24C02

#define ID_EEPROM_BOARDTYPE    0
#define ID_EEPROM_BOARDREV     4
#define ID_EEPROM_MANUF_DATE   8
#define ID_EEPROM_UID          12
/* 8 bytes */

#define ID_EEPROM_SCRATCH      127

unsigned int id_eeprom_read( int index );
int id_eeprom_write( int index, unsigned int val );
void id_eeprom_read_serial_num( char *sbuffer );


#endif /* __common_id_eeprom_h__ */
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
