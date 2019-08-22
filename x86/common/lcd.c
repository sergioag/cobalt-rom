/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * lcd.c
 */

#include <string.h>
#include <io.h>

#include "common/romconf.h"
#include "common/rom.h"
#include "common/systype.h"
#include "common/lcd.h"
#include "common/delay.h"
#include "common/pci.h"
#include "common/i2c.h"
#include "common/parallel.h"
#include "common/eeprom.h"
#include "common/cmos.h"

static int use_i2c = 0;

static void lcd_set_led_defaults(void);

#define LCD_MAX_POLL 100
static void lcd_poll_wait(void)
{
	int i = 0;
	while (i++ < LCD_MAX_POLL) {
		if (lcd_read_inst() & 0x80) 
			continue;
		break;
	}
}

/* these four functions from the lcd driver */
void lcd_write_inst( unsigned char inst )
{
    lcd_poll_wait();
    if (use_i2c) {
	    i2c_write_byte( 0x48, 0, inst);
    } else {
	    outb( 0x03, LCD_CONTROL_PORT );  /* RS=0, R/W=0, E=0 */
	    outb( inst, LCD_DATA_PORT );
	    outb( 0x02, LCD_CONTROL_PORT );  /* RS=0, R/W=0, E=1 */
	    outb( 0x03, LCD_CONTROL_PORT );  /* RS=0, R/W=0, E=0 */
	    outb( 0x01, LCD_CONTROL_PORT );  /* RS=0, R/W=1, E=0 */
    }

    udelay(100);

}

void lcd_write_data( unsigned char data )
{
    lcd_poll_wait();
    if (use_i2c) {
	    i2c_write_byte( 0x4a, 0, data);
    } else{
	    outb( 0x07, LCD_CONTROL_PORT );  /* RS=1, R/W=0, E=0 */
	    outb( data, LCD_DATA_PORT );
	    outb( 0x06, LCD_CONTROL_PORT );  /* RS=1, R/W=0, E=1 */
	    outb( 0x07, LCD_CONTROL_PORT );  /* RS=1, R/W=0, E=0 */
	    outb( 0x05, LCD_CONTROL_PORT );  /* RS=1, R/W=1, E=0 */
    }

    delay(1);
}

unsigned char lcd_read_inst( void )
{
    unsigned char inst;

    if (use_i2c) {
	    inst = i2c_read_byte( 0x49, 0 );
    } else {
	    outb( 0x21, LCD_CONTROL_PORT );  /* RS=0, R/W=1, E=0 */
	    outb( 0x20, LCD_CONTROL_PORT );  /* RS=0, R/W=1, E=1 */
	    inst = inb( LCD_DATA_PORT );
	    outb( 0x21, LCD_CONTROL_PORT );  /* RS=0, R/W=1, E=0 */
	    outb( 0x01, LCD_CONTROL_PORT );  /* RS=0, R/W=1, E=0 */
    }

    delay(1);
    return inst;
}

unsigned char lcd_read_data( void )
{
    unsigned char data;

    lcd_poll_wait();
    if (use_i2c) {
	    data = i2c_read_byte( 0x4b, 0 );
    } else {
	    outb( 0x25, LCD_CONTROL_PORT );  /* RS=1, R/W=1, E=0 */
	    outb( 0x24, LCD_CONTROL_PORT );  /* RS=1, R/W=1, E=1 */
	    data = inb( LCD_DATA_PORT );
	    outb( 0x25, LCD_CONTROL_PORT );  /* RS=1, R/W=1, E=0 */
	    outb( 0x01, LCD_CONTROL_PORT );  /* RS=1, R/W=1, E=0 */
    }

    delay(1);

    return data;
}

void lcd_init( void )
{
	int i2c_io_port = pci_read_config(0, PCI_DEVFN(0xf,0), 0x90, 4)&0xfff0;
	if (i2c_io_port && (i2c_read_byte(0x41, 0) != 0xff)) {
		use_i2c = 1;
	}

	if (!use_i2c)
		init_parallel_port();
	lcd_write_inst( LCD_CMD_INIT1 );
	lcd_write_inst( LCD_CMD_INIT1 );
	lcd_write_inst( LCD_CMD_INIT1 );
	lcd_write_inst( LCD_CMD_INIT2 );
	lcd_write_inst( LCD_CMD_INIT3 );
	
	lcd_on();
/*	lcd_clear(); */
	lcd_cursor_off();
	lcd_set_cur_pos( 0 );
	
	lcd_set_led_defaults();
}

static void 
lcd_set_led_defaults(void)
{
    if (systype() == SYSTYPE_3K) {
        unsigned char tmp;

        /* pci structs are not up, yet - but we know PMU is 0:3 */
        tmp = pci_read_config(0, PCI_DEVFN(3,0), 0x7e, 1);

        if (boardtype() == BOARDTYPE_PACIFICA) {
            /* RaQ3/4 have reverse polarity for shutdown light, not for logo */
            tmp &= ~0x40;
            tmp |= 0x80;
        } else if (boardtype() == BOARDTYPE_CARMEL) {
            /* Qube3 has direct polarity for both LEDs */
            tmp |= (0x40 | 0x80);
        }
        pci_write_config(0, PCI_DEVFN(3,0), 0x7e, tmp, 1);
    } else if (systype() == SYSTYPE_5K) {
        if (use_i2c) {
		/* FIXME: set up the default LEDs for a monterey */
        }
    }
}

void lcd_on(void)
{
    lcd_write_inst( LCD_CMD_ON );
}

void lcd_off( void )
{
    lcd_write_inst( LCD_CMD_OFF );
}

void lcd_reset( void )
{
    lcd_write_inst( LCD_CMD_RESET1 );
    lcd_write_inst( LCD_CMD_RESET1 );
    lcd_write_inst( LCD_CMD_RESET1 );
    lcd_write_inst( LCD_CMD_RESET1 );
    lcd_write_inst( LCD_CMD_RESET2 );
    lcd_write_inst( LCD_CMD_RESET3 );
}

void lcd_clear( void )
{
    lcd_write_inst( LCD_CMD_CLEAR );
}

void lcd_cursor_left( void )
{
    lcd_write_inst( LCD_CMD_CUR_LEFT );
}

void lcd_cursor_right( void )
{
    lcd_write_inst( LCD_CMD_CUR_RIGHT );
}

void lcd_cursor_off( void )
{
    lcd_write_inst( LCD_CMD_CUR_OFF );
}

void lcd_cursor_on( void )
{
    lcd_write_inst( LCD_CMD_CUR_ON );
}

void lcd_blink_off( void )
{
    lcd_write_inst( LCD_CMD_BLINK_OFF );
}

unsigned char lcd_get_cur_pos( void )
{
    return lcd_read_inst() & 0x7f;
}

void lcd_set_cur_pos( unsigned char pos )
{
    lcd_write_inst( (pos & 0x7f) | LCD_SET_POS_MASK );
}

void lcd_write_char( unsigned char c )
{
    lcd_write_data( c );
}

void lcd_write_str( char *str )
{
    while( *str )
	lcd_write_char( *str++ );
}

void lcd_write_str_at(unsigned char pos, char *str)
{
    lcd_set_cur_pos(pos);
    lcd_write_str(str);
}

void lcd_set_char( int c, unsigned char *data )
{
	/* save current CGRAM pos */
    unsigned char pos = lcd_get_cur_pos();
    int i;

	/* select char to set */
    lcd_write_inst( LCD_SET_CHAR_MASK | ((c<<3)&0x3f) );
	
    for( i=0 ; i<8 ; i++ )
    {
	lcd_write_data( data[i] );
    }

	/* retstore current CGRAM pos */
    lcd_set_cur_pos( pos );
} 

/*
 * buttons
 */

/* note: there is now way to tell the difference between all buttons pressed 
 * and no buttons pressed.  This is not too much of a problem as it took two
 * people at the button pannel to confirm this.
 */
unsigned char lcd_button_read( void )
{
	unsigned char inst;

	if (use_i2c) {
		inst = i2c_read_byte( 0x41, 0 );
		if (inst == 0x3e)
			return ~(BUTTON_SELECT)&0xfe;
		if (inst == 0x3d)
			return ~(BUTTON_ENTER)&0xfe;
		if (inst == 0x1f)
			return ~(BUTTON_LEFT)&0xfe;
		if (inst == 0x3b)
			return ~(BUTTON_RIGHT)&0xfe;
		if (inst == 0x2f)
			return ~(BUTTON_UP)&0xfe;
		if (inst == 0x37)
			return ~(BUTTON_DOWN)&0xfe;
		if (inst == 0x3f)
			return BUTTON_NONE;
		return inst;
	} else {
		outb( 0x29, LCD_CONTROL_PORT );
		return inb( LCD_DATA_PORT ) & BUTTON_MASK;
	}
}



/* the following functions deal with reading fonts and strings out of the rom */
struct logo {
	char title1[13];
	char title2[13];
	unsigned char font[0];
} __attribute__ ((packed));

void lcd_set_font( int font_num )
{
    int i;
    volatile struct logo *ldata;

    if( font_num == 0 &&
        cmos_read_flag(CMOS_NOLOGO_FLAG) )
    {
        unsigned char blank[8];
        memset( blank, 0x0, sizeof( blank ) );
        for( i=0 ; i<8 ; i++ )
        {
            lcd_set_char( i, blank );
        }
    }
    else
    {
        eeprom_set_bank_page(0,0);
        ldata = (struct logo *)(0xffff0000 + ROM_LOGO_OFFSET);
        
        for( i=0 ; i<8 ; i++ )
        {
            lcd_set_char( i, (unsigned char *) ldata->font + (64*font_num + i*8 ) );
        }
    }
}

void lcd_get_title( char title1[13], char title2[13] )
{
    volatile struct logo *ldata;
    
    eeprom_set_bank_page(0,0);
    ldata = (struct logo *)(0xffff0000 + ROM_LOGO_OFFSET);
    strcpy( title1, (char *) ldata->title1 );
    strcpy( title2, (char *) ldata->title2 );
}

void lcd_write_progress_bar( int percent )
{
    int i;
    int target = (percent * 12) / 100;
    
    lcd_set_cur_pos(0x44);
    for( i=0; i<target ; i++ )
	lcd_write_char( 0xff );

    for( ; i<12 ; i++ )
	lcd_write_char( ' ' );

}


#define COBALT_I2C_DEV_LED_I            0x40
#define COBALT_I2C_DEV_LED_II           0x42
#define MONTEREY_FPLED_ETH0_TXRX        0x8000
#define MONTEREY_FPLED_ETH0_LINK        0x1000
#define MONTEREY_FPLED_ETH1_TXRX        0x4000
#define MONTEREY_FPLED_ETH1_LINK        0x0800
#define MONTEREY_FPLED_DISK0            0x2000
#define MONTEREY_FPLED_DISK1            0x0200
#define MONTEREY_FPLED_DISK2            0x0080
#define MONTEREY_FPLED_DISK3            0x0040
#define MONTEREY_FPLED_DISK_FAULT       0x0010
#define MONTEREY_FPLED_WEB              0x0400
#define MONTEREY_FPLED_HEART            0x0100
#define MONTEREY_FPLED_SPARE            0x0020
#define FPLED_MASK   (MONTEREY_FPLED_ETH0_TXRX |\
                      MONTEREY_FPLED_ETH0_LINK |\
                      MONTEREY_FPLED_ETH1_TXRX |\
                      MONTEREY_FPLED_ETH1_LINK |\
                      MONTEREY_FPLED_DISK0     |\
                      MONTEREY_FPLED_DISK1     |\
                      MONTEREY_FPLED_DISK2     |\
                      MONTEREY_FPLED_DISK3     |\
                      MONTEREY_FPLED_DISK_FAULT|\
                      MONTEREY_FPLED_WEB       |\
                      MONTEREY_FPLED_HEART     |\
                      MONTEREY_FPLED_SPARE)
  

void lcd_set_boot_leds( int state )
{
    if( systype_is_5k() )
    {
	if( use_i2c )
	{
	    unsigned int val;
	    val = i2c_read_byte(COBALT_I2C_DEV_LED_II, 0);
	    val <<= 8;
	    val |= i2c_read_byte(COBALT_I2C_DEV_LED_I, 0);
	    val &= ~FPLED_MASK;
	    if( state )
		val |= FPLED_MASK;
	    i2c_write_byte(COBALT_I2C_DEV_LED_I, 0, val & 0xff);
	    i2c_write_byte(COBALT_I2C_DEV_LED_II, 0, val >> 8 );
	}
	else
	{
		/* must be alpine */
	    if( state )
	    {
		/* webled is reverse polarity, logo and sysfault are not */
		outb( 0x00, 0x600 + 0xe );
		outb( 0x07, 0x500 + 0xb );
                outb( 0x23, 0x2e );
                outb( 0x61, 0x2f );
	    }
	    else
	    {
		outb( 0x20, 0x600 + 0xe );
		outb( 0x00, 0x500 + 0xb );
                outb( 0x23, 0x2e );
                outb( 0x63, 0x2f );
	    }
	}
    }
}

void lcd_clean_leds( void )
{
    unsigned char v;
    
    if (systype_is_5k() && (!use_i2c)) {
	v = 0x00;
    	if (cmos_read_flag(CMOS_SYSFAULT_FLAG)) {
		v |= 0x7;
	}
	outb(v, 0x500 + 0xb);    
        outb( 0x23, 0x2e );
        outb( 0x61, 0x2f );
    }
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
