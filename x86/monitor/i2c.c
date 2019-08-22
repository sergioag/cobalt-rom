/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: i2c.c,v 1.1.1.1 2003/06/10 22:42:19 iceblink Exp $
 */
#include <string.h>
#include <printf.h>
#include <io.h>

#include "common/rom.h"
#include "common/i2c.h"
#include "common/systype.h"

#include "i2c.h"


static int degrees(int); 

int read_ics_func( int argc, char *argv[] __attribute__ ((unused)) )
{
	unsigned char data[6];
	int i;
	
	if( argc != 1 )
		return EBADNUMARGS;
		
	i2c_read_block( 0xd2, 0x0, data, 6 );
	
	for( i=0 ; i<6 ; i++ )
	{
		printf( "0x%2x ", data[i] );
	}
	
	printf("\n");

	return 0;
}
int write_ics_func( int argc, char *argv[] )
{
	unsigned char data[6];
	int i;
	unsigned long num;
	
	if( argc != 7 )
		return EBADNUMARGS;
	
	for( i=0 ; i<6 ; i++ )
	{
		if( stoli( argv[i+1], &num ) )
			return EBADNUM;
		data[i] = num&0xff;
	}
		
	i2c_write_block( 0xd2, 0x0, data, 6 );
	
	return 0;
}

static int degrees(int val) 
{
	int deg=0;

	if (systype_is_3k() || boardtype() == BOARDTYPE_MONTEREY) {
		/* shift 4 to drop off the .5 degrees bit */
		deg = val >> 4;
		/* then add it back, but rounded */
		if (val & 0x0008) {
			if (val < 0) {
				deg--;
			} else {
				deg++;
			}
		}
	}
	else if (boardtype() == BOARDTYPE_ALPINE) {
		if ((val&0x80) == 0x80) {
			deg = val - 0xff - 1;
		} else {
			deg = val;
		}
	}

	return deg;
}

int read_voltage_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1) {
		return EBADNUMARGS;
	}

	if (boardtype() == BOARDTYPE_ALPINE) {
		int j;
		/* set up the Vbat select line */
		outb(inb(0x608) | 1, 0x608);
		for (j=0; j<2; j++) {
			int val;
			int low;
			int high;

			val = i2c_read_byte(I2C_DEV_ADM1029,
						ADM1029_AIN_VALUE(j));
			low = i2c_read_byte(I2C_DEV_ADM1029,
						ADM1029_AIN_LOW(j));
			high = i2c_read_byte(I2C_DEV_ADM1029,
						 ADM1029_AIN_HIGH(j));
			printf("Voltage Sensor %d :  %d.%d (%d.%d-%d.%d)\n",
			       j, val/100, val%100, low/100,
			       low%100, high/100, high%100);
		}
		/* unset the Vbat select line */
		outb(inb(0x608) & ~1, 0x608);
	}

	return 0;
}

int read_temp_func(int argc, char *argv[] __attribute__ ((unused)))
{
	int tmp = 0;
	int val = 0;
	int sensorID = 0;

	if (argc != 1) {
		return EBADNUMARGS;
	}

	if (systype_is_3k() || boardtype() == BOARDTYPE_MONTEREY) {
		for (sensorID = 0; sensorID <4; sensorID++) {
			/* address = base + device + 1 (for read) */
			tmp = i2c_read_word(I2C_DEV_LM77 + (sensorID<<1) + 1, 
					    LM77_TEMP);
			val = (tmp<<8 & 0xff00) + (tmp>>8 & 0x00ff);
			printf("Sensor %d = %d degrees C  (0x%04x)\n",
			       sensorID, degrees(val), val);
		}
	} else if (boardtype() == BOARDTYPE_ALPINE) {
		tmp = i2c_read_byte(I2C_DEV_ADM1029, ADM1029_TEMP_VALUE(0));
		printf("ADM1029 Sensor  :  %d degrees C\n", degrees(tmp));

		tmp = i2c_read_byte(I2C_DEV_ADM1029, ADM1029_TEMP_VALUE(1));	
		printf("Tualatin Sensor :  %d degrees C\n", degrees(tmp));
	}

	return 0;
}

/* LM77 stores temperature values are this way:
 * "ssss|tttt|tttt|tfff"
 * s = sign bit
 * t = temp bit
 * f = flag bit
 */
int read_temp_thresh_func(int argc, char *argv[] __attribute__ ((unused)))
{
	int tmp = 0;
	int val = 0;

	if (argc != 1) {
		return EBADNUMARGS;
	}

	if (systype_is_3k() || boardtype() == BOARDTYPE_MONTEREY) {
		tmp = i2c_read_word(I2C_DEV_LM77, LM77_LOW);
		val = (tmp<<8 & 0xff00) + (tmp>>8 & 0x00ff);
		printf("low: 0x%04x (%d degrees C)\n", val, degrees(val));

		tmp = i2c_read_word(I2C_DEV_LM77, LM77_HIGH);
		val = (tmp<<8 & 0xff00) + (tmp>>8 & 0x00ff);
		printf("high: 0x%04x (%d degrees C)\n", val, degrees(val));

		tmp = i2c_read_word(I2C_DEV_LM77, LM77_CRIT);
		val = (tmp<<8 & 0xff00) + (tmp>>8 & 0x00ff);
		printf("critical: 0x%04x (%d degrees C)\n", val, degrees(val));

		tmp = i2c_read_word(I2C_DEV_LM77, LM77_HYST);
		val = (tmp<<8 & 0xff00) + (tmp>>8 & 0x00ff);
		printf("hysteresis: 0x%04x (%d degrees C)\n", val, degrees(val));
	}
	else if (boardtype() == BOARDTYPE_ALPINE) {		
		tmp = i2c_read_byte(I2C_DEV_ADM1029, ADM1029_TEMP_LOW(0));
		printf("ADM1029 Temp low         : %d degrees C\n", degrees(tmp));
		tmp = i2c_read_byte(I2C_DEV_ADM1029, ADM1029_TEMP_HIGH(0));
		printf("ADM1029 Temp high        : %d degrees C\n", degrees(tmp));
		tmp = i2c_read_byte(I2C_DEV_ADM1029, ADM1029_TEMP_HYST(0));
		printf("ADM1029 Temp hysteresis  : %d degrees C\n", (tmp>>4)|0xf);
		printf("\n");
		tmp = i2c_read_byte(I2C_DEV_ADM1029, ADM1029_TEMP_LOW(1));
		printf("Taulatin Temp low        : %d degrees C\n", degrees(tmp));
		tmp = i2c_read_byte(I2C_DEV_ADM1029, ADM1029_TEMP_HIGH(1));
		printf("Tualatin Temp high       : %d degrees C\n", degrees(tmp));
		tmp = i2c_read_byte(I2C_DEV_ADM1029, ADM1029_TEMP_HYST(1));
		printf("Tualatin Temp hysteresis : %d degrees C\n", (tmp>>4)|0xf);
	}

	return 0;
}

int write_temp_thresh_func(int argc, char *argv[])
{
	int newval = 0;
	unsigned long val;

	if (argc != 3) {
		return EBADNUMARGS;
	}

	if (stoli(argv[2], &val)) {
		return EBADNUM;
	}

	if (systype_is_3k() || boardtype() == BOARDTYPE_MONTEREY) {

		val <<= 4;
		newval = (val<<8 & 0xff00) | (val>>8 & 0x00ff);

		if (!strcmp(argv[1], "hyst"))
			i2c_write_word(I2C_DEV_LM77, LM77_HYST, newval);
		else if (!strcmp(argv[1], "low"))
			i2c_write_word(I2C_DEV_LM77, LM77_LOW, newval);
		else if (!strcmp(argv[1], "high"))
			i2c_write_word(I2C_DEV_LM77, LM77_HIGH, newval);
		else if (!strcmp(argv[1], "crit"))
			i2c_write_word(I2C_DEV_LM77, LM77_CRIT, newval);
		else return EBADARG;
	}
	else if (boardtype() == BOARDTYPE_ALPINE) {

		if (!strcmp(argv[1], "hyst")) {	
			newval = i2c_read_byte(I2C_DEV_ADM1029, ADM1029_TEMP_HYST(1));
			newval &= ~0xf0;
			newval |= (val << 4) & 0xf0;
			i2c_write_byte(I2C_DEV_ADM1029, ADM1029_TEMP_HYST(1), newval);
		}
		else if (!strcmp(argv[1], "low"))
			i2c_write_byte(I2C_DEV_ADM1029, ADM1029_TEMP_LOW(1), val);
		else if (!strcmp(argv[1], "high"))
			i2c_write_byte(I2C_DEV_ADM1029, ADM1029_TEMP_HIGH(1), val);
		else return EBADARG;
	}

	return 0;
}

int write_i2c_func(int argc, char *argv[])
{
	unsigned long dev;
	unsigned long idx;
	unsigned long len;
	unsigned long value;

	if (argc != 5) {
		return EBADNUMARGS;
	}
	if ((stoli(argv[1], &dev) != 0)
		|| (stoli(argv[2], &idx) != 0)
		|| (stoli(argv[3], &len) != 0)) {
		return EBADNUM;
	}
	stoli(argv[4], &value);

	if (len == 1) {
		i2c_write_byte( (int)dev, (int)idx, value);
	}
	else if (len == 2) {
		i2c_write_word( (int)dev, (int)idx, value);
	}
	else {
		return EBADNUM;
	}
	return 0;
}

int read_i2c_func(int argc, char *argv[])
{
	int val;
	unsigned long dev;
	unsigned long idx;
	unsigned long len;
	
	if (argc != 4) {
		return EBADNUMARGS;
	}
	if ((stoli(argv[1], &dev) != 0) 
	    || (stoli(argv[2], &idx) != 0)
	    || (stoli(argv[3], &len) != 0)) {
		return EBADNUM;
	}

	if (len == 1) {
		val = i2c_read_byte((int)dev, (int)idx);
		printf("0x%02x\n", val);
	} else if (len == 2) {
		val = i2c_read_word((int)dev, (int)idx);
		printf("0x%04x\n", val);
	} else {
		return EBADARG;
	}

	return 0;
}
		
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
