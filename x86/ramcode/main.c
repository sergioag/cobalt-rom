/* $Id: main.c,v 1.1.1.1 2003/06/10 22:42:22 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 */
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <printf.h>

#include "common/romconf.h"
#include "common/rom.h"
#include "common/rammap.h"
#include "common/elf.h"
#include "common/systype.h"
#include "common/cmos.h"
#include "common/delay.h"
#include "common/serial.h"
#include "common/eeprom.h"
#include "common/pci.h"
#include "common/fs.h"
#include "common/lcd.h"
#include "common/i2c.h"
#include "common/bzlib.h"
#include "common/cache.h"
#include "common/msr.h"

static void clear_bss(void);
static void pic_init(void);
static void hang(void);
static void mini_monitor(void);
static void main_lcd_magic(void); 
static void toggle_console(void);

static char *title1, *title2;

/* function pointer for monitor */
typedef void (*monitor_fn)(unsigned int memsize, unsigned int monsize);

unsigned int mem_size;
void do_first_boot( void ){}

/*
 * This is the starting point for ramcode
 * 
 * Ramcode is responsible for loading monitor from the ROMFS, and not much
 * else.
 */
volatile void 
ram_main(unsigned int memsize)
{
	monitor_fn monitor_start;
	int monsize;
	unsigned char in;

	/* Set up interrupt controllers */
	pic_init();
	
	/* make sure the .bss space is nice for us */
	clear_bss();

	/* save this */
	mem_size = memsize;

	/* we need systype to do anything */
	systype_init();

	/* we need delays for various things */
	delay_init();

	/* do i2c for possible i2c LCDs */
	i2c_init();
	
	/* lcd init needed for console toggle and magic (below) */
	lcd_init();

	/* boot-time hack to enable/disable console:
	 *   5000:  hold "LEFT" button
	 *   3000:  hold "RESET" button
	 */
	if (systype_is_3k()) {
      		if (lcd_buttons_in_set(lcd_button_read(), BUTTON_RESET))
			toggle_console();
	} else if (systype_is_5k()) {
		if (lcd_buttons_in_set(lcd_button_read(), BUTTON_LEFT))
			toggle_console();
	}

	/* need serial for output */
	serial_init(cmos_read_flag(CMOS_CONSOLE_FLAG));
	printf("\n");

	/* we need eeproms to load from the ROM fs */
	if (eeprom_init() < 0) {
		printf("EEPROM initialization failed\n");
		hang();
	}

	main_lcd_magic(); 

	/* init the ROM fs */
	fs_init();

	if( !cmos_check_checksum() || /* if this is the fisrt boot, force cache on */
            !cmos_read_flag(CMOS_DELAY_CACHE_FLAG) )
	{
	    cache_init(0);
	    mtrr_init();
	    cache_l1_on();
	    if( systype_is_5k() )
		msr_set_on(MSR_BBL_CR_CTL3, (1 << 8));
	}

	if( (!serial_inb_timeout(&in, BOGO_SEC>>2 )) && (in==' ') )
	    mini_monitor();

	/* load the monitor */
	if ((monsize = load_from_romfs("/monitor")) < 0) {
		printf("Load monitor failed\n");
		hang();
	} else {
		int r;
		
		if (!elf_valid((void *)RAM_SERIAL_LOAD)) {
		    printf("Load monitor failed\n");
			hang();
		}
		r = elf_relocate((void *)RAM_SERIAL_LOAD, 
			(void *)RAM_MONITOR_CODE, 
			RAM_MONITOR_MAX - RAM_MONITOR_CODE);
		if (r) {
			printf("Load monitor failed - not ELF?\n");
			hang();
		}
		monitor_start =	(monitor_fn)elf_entry((void *)RAM_SERIAL_LOAD);
		monitor_start(memsize, monsize);
	}

	/* shouldn't get here */
	printf("ACK!! I shouldn't be here!\n");
	hang();
}

static void
pic_init(void)
{
	/* ICW1 */
	outb_p(0x11, 0x20); /* initialization sequence */
	outb_p(0x11, 0xa0); /* initialization sequence */
    
	/* ICW2 */
	outb_p(0x20, 0x21); /* start of hardware int's (0x20) */
	outb_p(0x28, 0xa1); /* start of hardware int's (0x20) */
    
	/* ICW3 */
	outb_p(0x04, 0x21); /* 8259-1 is master */
	outb_p(0x02, 0xa1); /* 8259-2 is slave  */

	/* ICW4 */
	outb_p(0x01, 0x21); /* 8086 mode for both */
	outb_p(0x01, 0xa1); /* 8086 mode for both */
    
	/* OCW1 */
#if 0 /* FIXME: what is going on here? */
	outb_p(0xff, 0xa1); /* mask off all interrupts for now */
	outb_p(0xfb, 0x21); /* mask all irq's but irq2 which is cascaded */
#else
	outb_p(0x00, 0x21);
	outb_p(0x00, 0xa1);
#endif
}

extern char _bss;
extern char _ebss;
static void 
clear_bss(void)
{
	unsigned long bss_start, bss_end;
	char *ptr;

	bss_start = (unsigned long)&_bss;
	bss_end = (unsigned long)&_ebss;

	/* kill it all */
	ptr = (char *)bss_start;

	memset(ptr, 0, bss_end - bss_start);
}

static void
hang(void)
{
    mini_monitor();
    printf("Halting system....");
    asm("cli; hlt");
}

static void mini_monitor_help( void )
{
    printf( "h - display help\n" );   
    printf( "m - download and run new monitor\n" );
    printf( "b - download and run a bzip2ed monitor\n" );
/*    printf( "t <?|test> - run a system test\n" ); */
    printf( "q - quit MiniMonitor (proceed to Monitor)\n" );
    printf( "\n" );
}

static void mini_monitor_dl( int bz )
{
    unsigned char *data = (unsigned char *)RAM_SERIAL_LOAD;
    unsigned char in;
    unsigned int  sz = 0;
    int r;
    monitor_fn monitor_start;
    

    	/* read monitor from serial port */
    printf("Loading monitor from serial port...\n");
    serial_inb(&in);
    do {
	*(data+sz) = in;
	if ((++sz % 0x400) == 0)
	    printf("\r%dk", sz>>10);
    } while (!serial_inb_timeout(&in, 8*BOGO_SEC) 
	     && (unsigned long)(data+sz) < RAM_SERIAL_LOAD_MAX);
    printf("\r%dk done\n", sz>>10);


    
	/* decompress if necessary */
    if( bz )
    {
	unsigned int destLen = RAM_DECOMP_AREA_MAX-RAM_DECOMP_AREA;
	int err;
	
	if( (err = BZ2_bzBuffToBuffDecompress( (char *)RAM_DECOMP_AREA, &destLen,
					data, sz, 1, 4 )) != BZ_OK )
	{
	    printf( "Error bunziping monitor:\n" );
	    return;
	}
	
 	memcpy( data, (char *)RAM_DECOMP_AREA, destLen );
    }
	/* relocate the image */
    
    if (!elf_valid((void *)RAM_SERIAL_LOAD)) {
	printf("Loading monitor failed - not ELF?\n");
	return;
    }

    r = elf_relocate((void *)RAM_SERIAL_LOAD, 
		     (void *)RAM_MONITOR_CODE, 
		     RAM_MONITOR_MAX - RAM_MONITOR_CODE);
    if (r) {
	printf("Loading monitor failed\n");
	return;
    }

	/* run the monitor */
    monitor_start = (monitor_fn)elf_entry((void *)RAM_SERIAL_LOAD);
    monitor_start(mem_size, sz);
}

static void mini_monitor( void )
{
    unsigned char command;

    printf("\n");
    printf("Sun Cobalt MiniMonitor\n"); 
    printf("-------------------------------------\n");

    mini_monitor_help();

    while(1) {
	printf( ">" );
	do {
	    serial_inb(&command);
	} while( command == ' ' );
	printf( "%c\n", command );
	
	if( command == 'h' )
	    mini_monitor_help();
	else if( command == 'm' )
	    mini_monitor_dl(0);
	else if( command == 'b' )
	    mini_monitor_dl(1);
	else if( command == 'q' )
	    return;
	else
	    printf( "Unknown command '%c'.  h for help\n", command );
    }  

}

/* 
 * let's try some LCD magic 
 */

static void main_lcd_magic(void)
{
	int i;
	
	title1 = malloc( 13 );
	title2 = malloc( 13 );
	lcd_set_font( 0 );
	if (!cmos_read_flag(CMOS_NOLOGO_FLAG)) {
		/* save the strings, JIC */
		lcd_get_title( title1, title2 );
	} else {
		strncpy(title1, "            ", 12);
		strncpy(title2, "            ", 12);
	}

	/* clear the LCD */
	lcd_clear();

	/* starting at the right - slide the logo, reveal the strings */
	/* this assumes that we can write indexes off the LCD screen */
	for (i = 0; i < 16; i++) {
		if (!cmos_read_flag(CMOS_NOLOGO_FLAG)) {
			/* row one */
			lcd_set_cur_pos(0x0 + (15-i));
			lcd_write_char(0);
			lcd_write_char(1);
			lcd_write_char(2);
			lcd_write_char(3);
			if (i >= 4) {
				lcd_write_char(title1[11-i + 4]);
			}

			/* row 2 */
			lcd_set_cur_pos(0x40 + (15-i));
			lcd_write_char(4);
			lcd_write_char(5);
			lcd_write_char(6);
			lcd_write_char(7);
			if (i >= 4) {
				lcd_write_char(title2[11-i + 4]);
			}
		} else {
			lcd_set_cur_pos(0x0 + i);
			lcd_write_char('*');
		}

		delay(100);
		if( i%10 == 0 )
		    lcd_set_boot_leds( 1 );
		else if(  i%10 == 5 )
		    lcd_set_boot_leds( 0 );
	}
	for( ; i<30 ; i++ )
	{
	    delay(100);
	    if( i%10 == 0 )
		lcd_set_boot_leds( 1 );
	    else if(  i%10 == 5 )
		lcd_set_boot_leds( 0 );
	}

	lcd_clean_leds();
}

static void toggle_console(void)
{
	lcd_logo_clear();

	lcd_set_cur_pos(0x04);
	if (cmos_read_flag(CMOS_CONSOLE_FLAG)) {
		lcd_write_str(" console OFF");
		cmos_write_flag(CMOS_CONSOLE_FLAG, 0);
	} else {
		lcd_write_str(" console ON");
		cmos_write_flag(CMOS_CONSOLE_FLAG, 1);
	}

	while (lcd_buttons_subset(lcd_button_read(),
				  (BUTTON_RESET|BUTTON_LEFT)))
		;

	/* restore the splash LCD state */
	lcd_logo_write(title1, title2);
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
