/*
 * $Id: id_eeprom.c,v 1.1.1.1 2003/06/10 22:41:46 iceblink Exp $
 *
 * Copyright (C) 2001 Sun Microsystem, Inc.
 * All Rights Reserved
 *
 * Serial eeprom ID chip
 */

#include "common/i2c.h"
#include "common/id_eeprom.h"

static int get_field_size( int index )
{
    if( (index == ID_EEPROM_SCRATCH) )
	return 1;

    if( (index == ID_EEPROM_BOARDTYPE) ||
	(index == ID_EEPROM_BOARDREV) ||
	(index == ID_EEPROM_MANUF_DATE) )
	return 4;
	
    return -1;
}


void id_eeprom_read_serial_num( char *sbuffer )
{
    int i;

    for( i=0 ; i<8 ; i++ )
    {
	sbuffer[i] = i2c_read_byte(ID_EEPROM_DEV, ID_EEPROM_UID + i);
    }
}

unsigned int id_eeprom_read( int index )
{
    int size;
    
    if( (size = get_field_size( index )) < 0 )
	return 0;

    if( size == 1 )
	return i2c_read_byte( ID_EEPROM_DEV, index );

    if( size == 2 )
	return i2c_read_word( ID_EEPROM_DEV, index );
    
    if( size == 4 )
    {
	unsigned int val = 0;
	val |= i2c_read_byte( ID_EEPROM_DEV, index ) << 24;
	val |= i2c_read_byte( ID_EEPROM_DEV, index + 1) << 16;
	val |= i2c_read_byte( ID_EEPROM_DEV, index + 2) << 8;
	val |= i2c_read_byte( ID_EEPROM_DEV, index + 3);
	return val;
    }
    
    return 0;
}

int id_eeprom_write( int index, unsigned int val )
{
    int size;
    
    if( (size = get_field_size( index )) < 0 )
	return -1;

    if( size == 1 )
	return i2c_write_byte( ID_EEPROM_DEV, index, (unsigned char) val );
    if( size == 2 )
	return i2c_write_word( ID_EEPROM_DEV, index, (unsigned short int) val );
    if( size == 4 )
    {
	int err;
	err = i2c_write_byte( ID_EEPROM_DEV, index, (val >> 24) & 0xff );
	err += i2c_write_byte( ID_EEPROM_DEV, index, (val >> 16) & 0xff );
	err += i2c_write_byte( ID_EEPROM_DEV, index, (val >> 8) & 0xff );
	err +=i2c_write_byte( ID_EEPROM_DEV, index, (val) & 0xff );
	return err;
    }

    return -1;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
