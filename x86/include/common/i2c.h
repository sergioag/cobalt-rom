/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: i2c.h,v 1.1.1.1 2003/06/10 22:42:02 iceblink Exp $
 */

#ifndef COMMON_I2C_H
#define COMMON_I2C_H

int i2c_init( void );
int i2c_reset( void );
int i2c_read_byte( int dev, int index );
int i2c_read_word( int dev, int index );
int i2c_write_byte( int dev, int index, int val );
int i2c_write_word( int dev, int index, int val );
int i2c_read_block( int dev, int index, unsigned char *data, int count );
int i2c_write_block( int dev, int index, unsigned char *data, int count );

#define I2C_DEV_LM77			0x90
#define  LM77_TEMP			0x0
#define  LM77_STATUS			0x1
#define  LM77_HYST			0x2
#define  LM77_CRIT			0x3
#define  LM77_LOW			0x4
#define  LM77_HIGH			0x5

#define I2C_DEV_DRIVE_SWITCH            0x45
#define I2C_DEV_RULER			0x46
#define I2C_DEV_RULER_PORT(port)	((0x100 >> (port)) & 0xf0)

#define I2C_DEV_SPD			0xA0
#define I2C_DEV_SPD_PORT(slot)		(((slot) << 1) & I2C_DEV_SPD)
#define  SPD_USED_BYTES			0 /* # bytes used by module manufacturer */
#define  SPD_SIZE			1 /* size of serial memory used by SPD */
#define  SPD_TYPE			2 /* memory type */
#define   SPD_TYPE_EDO			0x02
#define   SPD_TYPE_SDRAM		0x04
#define  SPD_ROW_ADDR			3 /* # of row addrs */
#define  SPD_COL_ADDR			4 /* # of column addrs */
#define  SPD_MOD_ROWS			5 /* # of module rows */
#define  SPD_DATA_WIDTH_1		6 /* data width (bit 0 is LSB) */
#define  SPD_DATA_WIDTH_2		7 /* data width (bit 7 is MSB) */
#define  SPD_VOLTAGE			8 /* voltage interface standard */
#define  SPD_SDRAM_CYCLE_TIME		9 /* SDRAM cycle time at CL X (highest CAS lat.) */
#define  SPD_SDRAM_ACCESS_CLK		10 /* SDRAM access from clock (highest CAS lat.) */
#define  SPD_CONF			11 /* module config type */
#define   SPD_CONF_NONE			0x00
#define   SPD_CONF_PARITY		0x01
#define   SPD_CONF_ECC			0x02
#define  SPD_REFRESH			12 /* refresh rate/type */
#define  SPD_SDRAM_PRI_WIDTH		13 /* primary SDRAM width */
#define  SPD_SDRAM_ERR_WIDTH		14 /* error checking SDRAM width */
#define  SPD_MIN_CLK_DELAY		15 /* min. clock delay, back-to-back random col addr */
#define  SPD_BURST_LEN			16 /* burst lengths suported */
#define  SPD_BANKS			17 /* # of banks on each SDRAM device */
#define  SPD_CAS_LAT			18 /* CAS# latencies supported */
#define  SPD_CS_LAT			19 /* CS# latency */
#define  SPD_WRITE_LAT			20 /* write latency */
#define  SPD_SDRAM_MODULE_ATTR		21 /* SDRAM module attributes */
#define   SPD_REGISTERED_DIMM		0x16
#define  SPD_SDRAM_DEVICE_ATTR		22 /* SDRAM device attributes (general) */
#define  SPD_SDRAM_CYCLE_TIME_2		23 /* SDRAM cycle time at CL X-1 (2nd CAS# lat) */
#define  SPD_SDRAM_ACCESS_CLK_2		24 /* SDRAM access Clk at CL X-1 (2nd CAS# lat) */
#define  SPD_SDRAM_CYCLE_TIME_3		25 /* SDRAM cycle time at CL X-2 (3rd CAS# lat) */
#define  SPD_SDRAM_ACCESS_CLK_3		26 /* SDRAM access Clk at CL X-2 (3nd CAS# lat) */
#define  SPD_tRP			27 /* min row precharge time */
#define  SPD_tRRD			28 /* min row active to row active */
#define  SPD_tRCD			29 /* min RAS to CAS delay */
#define  SPD_tRAS			30 /* min RAS pulse width */
#define  SPD_DATA_VER			62 /* SPD data revision code */
#define  SPD_CHECKSUM			63 /* checksum for bytes 0-62 */

#define I2C_DEV_AT24C02			0xae /* alpine serial eeprom */

#define I2C_DEV_ADM1029			0x5e /* alpine system monitor */
#define  ADM1029_STATUS			(0x00)     /* Main Status Register */
#define  ADM1029_CONFIG			(0x01)     /* Config Register */
#define  ADM1029_RESET			(0x0b)     /* write causes Software Reset */
#define  ADM1029_MFG_ID			(0x0d)     /* Manufacturer ID [41h] */
#define  ADM1029_REV_ID			(0x0e)     /* Major/Minor Revision */
#define  ADM1029_GPIO_PRESENT		(0x05)     /* GPIO Present / AIN */
#define  ADM1029_GPIO_CONTROL(x)	(0x28 + x) /* GPIO Control */
#define  ADM1029_GPIO_FAULT(x)		(0x38 + x) /* set alarm speed if GPIO asserted */
#define  ADM1029_FAN_SET_ALRM		(0x07)     /* set Fan X Alarm Speed */
#define  ADM1029_FAN_SET_PLUG		(0x08)     /* set Fan X HotPlug Speed */
#define  ADM1029_FAN_SET_FULL		(0x09)     /* set Fan X Full Speed */
#define  ADM1029_FAN_STATUS(x)		(0x10 + x) /* Fan Status */
#define  ADM1029_FAN_CONTROL(x)		(0x18 + x) /* Fan Control */
#define  ADM1029_FAN_FAULT(x)		(0x20 + x) /* set alarm speed if Fan Fault */
#define  ADM1029_FAN_SPEED_FULL(x)	(0x60 + x) /* Normal Fan speed values */
#define  ADM1029_FAN_SPEED_PLUG(x)	(0x68 + x) /* HotPlug Fan speed values */
#define  ADM1029_FAN_TACH_VALUE(x)	(0x70 + x) /* Measured Fan Tachometer */
#define  ADM1029_FAN_TACH_HIGH(x)	(0x78 + x) /* High Limit for Fan Tachometer */
#define  ADM1029_TEMP_PRESENT		(0x06)     /* Temp Devices Installed */
#define  ADM1029_TEMP_OFFSET(x)		(0x30 + x) /* Temp Systemic Offset Effect */
#define  ADM1029_TEMP_CONTROL(x)	(0x40 + x) /* Temp Fault Control */
#define  ADM1029_TEMP_FAULT(x)		(0x48 + x) /* set alarm speed if Temp Fault */
#define  ADM1029_TEMP_MIN(x)		(0x80 + x) /* Min Temp for fan speed ctl */
#define  ADM1029_TEMP_HYST(x)		(0x88 + x) /* Temp range/step for fan speed ctl */
#define  ADM1029_TEMP_HIGH(x)		(0x90 + x) /* High Limit Temp measurement */
#define  ADM1029_TEMP_LOW(x)		(0x98 + x) /* Low Limit Temp measurement */
#define  ADM1029_TEMP_VALUE(x)		(0xa0 + x) /* Measured Temp Value */
#define  ADM1029_AIN_CONTROL(x)		(0x50 + x) /* Analog Channel Control */
#define  ADM1029_AIN_FAULT(x)		(0x58 + x) /* set alarm speed of Analog Fault */
#define  ADM1029_AIN_HIGH(x)		(0xa8 + x) /* High Limit Analog measurement */
#define  ADM1029_AIN_LOW(x)		(0xb0 + x) /* Low Limit Analog measurement */
#define  ADM1029_AIN_VALUE(x)		(0xb8 + x) /* Measure Analog Channel Value */

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
