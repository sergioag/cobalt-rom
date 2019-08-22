/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * This is the temperature chip, the ID chip, and clock
 * i2c.c
 */

#include <printf.h>
#include <string.h>
#include <io.h>

#include "common/rom.h"
#include "common/pci.h"
#include "common/i2c.h"
#include "common/systype.h"

#define I2C_3K_STATUS                   0x00
#define I2C_3K_CMD                      0x01
#define I2C_3K_START                    0x02
#define I2C_3K_ADDR                     0x03
#define I2C_3K_LOW_DATA                 0x04
#define I2C_3K_HIGH_DATA                0x05
#define I2C_3K_BLOCK_DATA               0x06
#define I2C_3K_INDEX                    0x07

#define I2C_3K_STATUS_IDLE              0x04

#define I2C_3K_CMD_RW_BYTE              0x20
#define I2C_3K_CMD_RW_WORD              0x30
#define I2C_3K_CMD_RW_BLOCK             0xC0
#define I2C_3K_CMD_RESET_PTR            0x80

#define I2C_5K_HOST_STATUS              0x00
#define I2C_5K_SLAVE_STATUS             0x01
#define I2C_5K_HOST_CONTROL             0x02
#define I2C_5K_HOST_COMMAND             0x03
#define I2C_5K_HOST_ADDR                0x04
#define I2C_5K_DATA_0                   0x05
#define I2C_5K_DATA_1                   0x06
#define I2C_5K_BLOCK_DATA               0x07
#define I2C_5K_SLAVE_CONTROL            0x08
#define I2C_5K_SHADOW_COMMAND           0x09
#define I2C_5K_SLAVE_EVENT              0x0a
#define I2C_5K_SLAVE_DATA               0x0c

#define I2C_5K_HOST_STATUS_BUSY         0x01

#define I2C_5K_HOST_CMD_START           0x40
#define I2C_5K_HOST_CMD_QUICK_RW        (0x00 << 2)
#define I2C_5K_HOST_CMD_BYTE_RW         (0x01 << 2)
#define I2C_5K_HOST_CMD_BYTE_DATA_RW    (0x02 << 2)
#define I2C_5K_HOST_CMD_WORD_DATA_RW    (0x03 << 2)
#define I2C_5K_HOST_CMD_BLOCK_DATA_RW   (0x05 << 2)

static unsigned short i2c_io_port;
static int i2c_wait_for_smi(void);
static void i2c_set_temp_defaults(void);
static void init_ics(void);

static unsigned char ics_init_block[6] = { 0xf6,0xff,0xff,0xff,0xff,0xff };

int i2c_init(void)
{
	/* PCI has not been initialized yet */
	switch (systype()) {
	case SYSTYPE_3K:
		i2c_io_port = pci_read_config(0, PCI_DEVFN(3,0), 0x14, 2) & 0xfff0;
		break;
	case SYSTYPE_5K:
		i2c_io_port = pci_read_config(0, PCI_DEVFN(0xf,0), 0x90, 4) & 0xfff0;
		break;
	}

	if (systype_is_3k()) {
		init_ics();
		i2c_set_temp_defaults();
	}

	return 0;
}

static void init_ics(void)
{
	/* set initial ics values - as per Glenn's request */
	i2c_write_block( 0xd2, 0x0, ics_init_block, 6 );
}	

int i2c_reset(void)
{
	switch (systype()) {
	case SYSTYPE_3K:
		outb(0xff, i2c_io_port + I2C_3K_STATUS);  /* clear status */
		outb(0x08, i2c_io_port + I2C_3K_CMD);     /* reset SMB devices */
		outb(0xff, i2c_io_port + I2C_3K_START);   /* start command */

		return i2c_wait_for_smi();
	case SYSTYPE_5K:
		/* not quite sure of the analogous sequence on the 5000 hardware */
		outb(0xff, i2c_io_port + I2C_5K_HOST_STATUS);  /* clear status */
		return 0;
	}
	return -1;
}

/* wait for the bus to go idle */
static int i2c_wait_for_smi(void)
{
	int timeout=0xffff;
	unsigned char status;
	
	while (timeout) {
		outb(0xff, 0x80);  /* wait */

		switch (systype()) {
		case SYSTYPE_3K:
		    status = inb(i2c_io_port + I2C_3K_STATUS);  /* get status */
		    if (status & I2C_3K_STATUS_IDLE)
			    return 0;
		    break;

		case SYSTYPE_5K:
		    status = inb(i2c_io_port + I2C_5K_HOST_STATUS);
		    if (! (status & I2C_5K_HOST_STATUS_BUSY))
			    return 0;
		    break;
		}
		timeout--;
	}
	
	/* FIXME: do this for both systypes */
	/* still busy - punch the abort bit */
	//outb(0x4, i2c_io_port + I2C_CMD);
	return -1;
}

int i2c_read_byte( int dev, int index )
{
	int ret_val = -1;
	
	if( systype_is_3k() )
	{
	    outb( 0xff, i2c_io_port + I2C_3K_STATUS ); /* clear status */
	    outb( (dev|0x1) &0xff, i2c_io_port + I2C_3K_ADDR ); /* device address */
	    outb( I2C_3K_CMD_RW_BYTE, i2c_io_port + I2C_3K_CMD ); /* Read Write byte */
	    outb( index&0xff, i2c_io_port + I2C_3K_INDEX ); /* I2C index */
	    
		/* start command */
	    outb( 0xff, i2c_io_port + I2C_3K_START );
	}
	else if( systype_is_5k() )
	{
	    outb( 0xff, i2c_io_port + I2C_5K_HOST_STATUS );  /* clear status */
	    outb( (dev|0x1) & 0xff, i2c_io_port + I2C_5K_HOST_ADDR ); /* device address */
	    outb( index & 0xff, i2c_io_port + I2C_5K_HOST_COMMAND ); /* byte index */
	    outb( I2C_5K_HOST_CMD_START | I2C_5K_HOST_CMD_BYTE_DATA_RW,
		  i2c_io_port + I2C_5K_HOST_CONTROL );
	}

	if( i2c_wait_for_smi() < 0 )
		return -1;
	
	if( systype_is_3k() )
	{
	    ret_val = inb( i2c_io_port + I2C_3K_LOW_DATA );
	}
	else if( systype_is_5k() )
	{
	    ret_val = inb( i2c_io_port + I2C_5K_DATA_0 );
	}

	return ret_val;
}

int i2c_read_word( int dev, int index )
{
	int ret_val = -1;
	
	if( systype_is_3k() )
	{
	    outb( 0xff, i2c_io_port + I2C_3K_STATUS ); /* clear status */
	    outb( (dev|0x1) &0xff, i2c_io_port + I2C_3K_ADDR ); /* device address */
	    outb( I2C_3K_CMD_RW_WORD, i2c_io_port + I2C_3K_CMD ); /* Read Write word */
	    outb( index&0xff, i2c_io_port + I2C_3K_INDEX ); /* I2C index */
	    
		/* start command */
	    outb( 0xff, i2c_io_port + I2C_3K_START );
	}
	else if( systype_is_5k() )
	{
	    outb( 0xff, i2c_io_port + I2C_5K_HOST_STATUS );  /* clear status */
	    outb( (dev|0x1) & 0xff, i2c_io_port + I2C_5K_HOST_ADDR ); /* device address */
	    outb( index & 0xff, i2c_io_port + I2C_5K_HOST_COMMAND ); /* byte index */
	    outb( I2C_5K_HOST_CMD_START | I2C_5K_HOST_CMD_WORD_DATA_RW,
		  i2c_io_port + I2C_5K_HOST_CONTROL );
	}

	if( i2c_wait_for_smi() < 0 )
		return -1;
	
	if( systype_is_3k() )
	{
	    ret_val = inb( i2c_io_port + I2C_3K_LOW_DATA ) +
		(inb( i2c_io_port + I2C_3K_HIGH_DATA) << 8);
	}
	else if( systype_is_5k() )
	{
	    ret_val = inb( i2c_io_port + I2C_5K_DATA_0 ) +
		(inb( i2c_io_port + I2C_5K_DATA_1) << 8);

	}

	return ret_val;
}

int i2c_write_byte( int dev, int index, int val )
{
	if( systype_is_3k() )
	{
	    outb( 0xff, i2c_io_port + I2C_3K_STATUS ); /* clear status */
	    outb( dev&0xff, i2c_io_port + I2C_3K_ADDR ); /* device address */
	    outb( I2C_3K_CMD_RW_BYTE, i2c_io_port + I2C_3K_CMD ); /* Read Write byte */
	    
	    outb( val&0xff, i2c_io_port + I2C_3K_LOW_DATA ); /* Low byte of data */
	    
	    outb( index&0xff, i2c_io_port + I2C_3K_INDEX ); /* I2C index */
	    
		/* start command */
	    outb( 0xff, i2c_io_port + I2C_3K_START );
	}
	else if( systype_is_5k() )
	{
	    outb( 0xff, i2c_io_port + I2C_5K_HOST_STATUS );  /* clear status */
	    outb( dev & 0xff, i2c_io_port + I2C_5K_HOST_ADDR ); /* device address */
	    outb( index & 0xff, i2c_io_port + I2C_5K_HOST_COMMAND ); /* byte index */
	    outb( val & 0xff, i2c_io_port + I2C_5K_DATA_0 );
	    
	    outb( I2C_5K_HOST_CMD_START | I2C_5K_HOST_CMD_BYTE_DATA_RW,
		  i2c_io_port + I2C_5K_HOST_CONTROL );
	}

	if( i2c_wait_for_smi() < 0 )
		return -1;

	return 0;	
}

int i2c_write_word( int dev, int index, int val )
{
	if( systype_is_3k() )
	{
	    outb( 0xff, i2c_io_port + I2C_3K_STATUS ); /* clear status */
	    outb( dev&0xff, i2c_io_port + I2C_3K_ADDR ); /* device address */
	    outb( I2C_3K_CMD_RW_WORD, i2c_io_port + I2C_3K_CMD ); /* Read Write WORD */
	    
	    outb( val&0xff, i2c_io_port + I2C_3K_LOW_DATA ); /* Low byte of data */
	    outb( (val>>8)&0xff, i2c_io_port + I2C_3K_HIGH_DATA ); /* Hi byte */
	    
	    outb( index&0xff, i2c_io_port + I2C_3K_INDEX ); /* I2C index */
	    
		/* start command */
	    outb( 0xff, i2c_io_port + I2C_3K_START );
	}
	else if( systype_is_5k() )
	{
	    outb( 0xff, i2c_io_port + I2C_5K_HOST_STATUS );  /* clear status */
	    outb( dev & 0xff, i2c_io_port + I2C_5K_HOST_ADDR ); /* device address */
	    outb( index & 0xff, i2c_io_port + I2C_5K_HOST_COMMAND ); /* byte index */
	    outb( val & 0xff, i2c_io_port + I2C_5K_DATA_0 );
	    outb( (val>>8) & 0xff, i2c_io_port + I2C_5K_DATA_1 );
	    
	    outb( I2C_5K_HOST_CMD_START | I2C_5K_HOST_CMD_WORD_DATA_RW,
		  i2c_io_port + I2C_5K_HOST_CONTROL );
	}

	if( i2c_wait_for_smi() < 0 )
		return -1;

	return 0;	
}

int i2c_read_block( int dev, int index, unsigned char *data, int count )
{
	if( systype_is_3k() )
	{
	    outb( 0xff, i2c_io_port + I2C_3K_STATUS ); /* clear status */
	    outb( (dev|0x1) &0xff, i2c_io_port + I2C_3K_ADDR ); /* device address */
	    outb( I2C_3K_CMD_RW_BLOCK, i2c_io_port + I2C_3K_CMD ); /* Read Write block */
	    outb( count&0xff, i2c_io_port + I2C_3K_LOW_DATA ); /* byte count */
	    outb( index&0xff, i2c_io_port + I2C_3K_INDEX ); /* I2C index */
	    
		/* start command */
	    outb( 0xff, i2c_io_port + I2C_3K_START );
	}
	else if( systype_is_5k() )
	{
	    outb( 0xff, i2c_io_port + I2C_5K_HOST_STATUS );  /* clear status */
	    outb( (dev|0x1) & 0xff, i2c_io_port + I2C_5K_HOST_ADDR ); /* device address */
	    outb( index & 0xff, i2c_io_port + I2C_5K_HOST_COMMAND ); /* byte index */
	    
		/* the CNB docs are not clear which one of these is the block 
		 * count register so I write to both 
		 */
	    outb( count & 0xff, i2c_io_port + I2C_5K_DATA_0 );
	    outb( count & 0xff, i2c_io_port + I2C_5K_DATA_1 );

	    outb( I2C_5K_HOST_CMD_START | I2C_5K_HOST_CMD_BLOCK_DATA_RW,
		  i2c_io_port + I2C_5K_HOST_CONTROL );
	}

	if( i2c_wait_for_smi() < 0 )
		return -1;

	while( count )
	{
		/* write a byte of block data */
	    if( systype_is_3k() )
	    {
		*data = inb( i2c_io_port + I2C_3K_BLOCK_DATA ); 
	    }
	    else if( systype_is_5k() )
	    {
		*data = inb( i2c_io_port + I2C_5K_BLOCK_DATA ); 
	    }
	    data++;
	    count--;
	}

	return 0;	
}

int i2c_write_block( int dev, int index, unsigned char *data, int count )
{
	if( systype_is_3k() )
	{
	    outb( 0xff, i2c_io_port + I2C_3K_STATUS ); /* clear status */
	    outb( dev&0xff, i2c_io_port + I2C_3K_ADDR ); /* device address */
	    outb( I2C_3K_CMD_RW_BLOCK, i2c_io_port + I2C_3K_CMD ); /* Read Write block */
	    
	    outb( count&0xff, i2c_io_port + I2C_3K_LOW_DATA ); /* byte count */
	}
	else if( systype_is_5k() )
	{
	    outb( 0xff, i2c_io_port + I2C_5K_HOST_STATUS );  /* clear status */
	    outb( dev&0xff & 0xff, i2c_io_port + I2C_5K_HOST_ADDR ); /* device address */
	    outb( index & 0xff, i2c_io_port + I2C_5K_HOST_COMMAND ); /* byte index */
	    
		/* the CNB docs are not clear which one of these is the block 
		 * count register so I write to both 
		 */
	    outb( count & 0xff, i2c_io_port + I2C_5K_DATA_0 );
	    outb( count & 0xff, i2c_io_port + I2C_5K_DATA_1 );

	}

	while( count )
	{
		/* write a byte of block data */
	    if( systype_is_3k() )
	    {
		outb( *data, i2c_io_port + I2C_3K_BLOCK_DATA ); 
	    }
	    else if( systype_is_5k() )
	    {
		outb( *data, i2c_io_port + I2C_5K_BLOCK_DATA ); 
	    }
	    data++;
	    count--;
	}
	
	if( systype_is_3k() )
	{
	    outb( index&0xff, i2c_io_port + I2C_3K_INDEX ); /* I2C index */
	    
		/* start command */
	    outb( 0xff, i2c_io_port + I2C_3K_START );
	}
	else if( systype_is_5k() )
	{
	    outb( I2C_5K_HOST_CMD_START | I2C_5K_HOST_CMD_BLOCK_DATA_RW,
		  i2c_io_port + I2C_5K_HOST_CONTROL );

	}
	if( i2c_wait_for_smi() < 0 )
		return -1;

	return 0;	
}

#define ADJUST		(-8)
#define TEMP_LOW	(55  + ADJUST)
#define TEMP_HIGH	(135 + ADJUST)
#define TEMP_CRIT	(150 + ADJUST)
#define TEMP_HYST	(8)
static void i2c_set_temp_defaults(void)
{
	int tmp = 0;
	int val = 0;

	/* we set low to where we really wanted high. 
         * the hookup is reversed on the raq3 board, so we compensate for 
         * the upside-down signal here.  Hopefully this will be fixed 
	 * eventually :) - then we can remove the "-8"'s
	 */
	tmp = TEMP_LOW<<4;
	val = (tmp<<8 & 0xff00) + (tmp>>8 & 0x00ff);
	i2c_write_word(I2C_DEV_LM77, LM77_LOW, val);

	tmp = TEMP_HIGH<<4;
	val = (tmp<<8 & 0xff00) + (tmp>>8 & 0x00ff);
	i2c_write_word(I2C_DEV_LM77, LM77_HIGH, val);

	tmp = TEMP_CRIT<<4;
	val = (tmp<<8 & 0xff00) + (tmp>>8 & 0x00ff);
	i2c_write_word(I2C_DEV_LM77, LM77_CRIT, val);

	tmp = TEMP_HYST<<4;
	val = (tmp<<8 & 0xff00) + (tmp>>8 & 0x00ff);
	i2c_write_word(I2C_DEV_LM77, LM77_HYST, val);
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
