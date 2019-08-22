/* $Id: main.c,v 1.1.1.1 2003/06/10 22:42:20 iceblink Exp $
 *
 * Copyright (c) 1999,2000 Cobalt Networks, Inc.
 * Copyright (c) 2001 Sun Microsystems, Inc.
 */

#include <io.h>
#include <stdlib.h>

#include "common/rom.h"
#include "common/cmos.h"
#include "common/rtc.h"
#include "common/cache.h"
#include "common/serial.h"
#include "common/boot.h"
#include "common/lcd.h"
#include "common/systype.h"
#include "common/memory.h"
#include "common/delay.h"
#include "common/eeprom.h"
#include "common/fs.h"
#include "common/pci.h"
#include "common/acpi.h"
#include "common/eth.h"
#include "common/ide.h"
#include "common/i2c.h"
#include "common/dallas.h"
#include "common/mp.h"
#include "common/mptable.h"
#include "common/microcode.h"

#include "menu.h"

#ifdef REMOTE_GDB
#include "common/exceptions.h"
#include "common/gdb.h"
#endif

unsigned int mem_size;

static void clear_bss(void);
static void show_menu_lcdmsg(void);
/*static void scrub_memory(void); */
static void set_memory_timing(struct cpu_info *);

char *title1, *title2;

/*
 * entry point from ramcode
 */
volatile void _start(unsigned int memsize, unsigned int monitor_size)
{
	unsigned char val;

	/* make sure the .bss space is nice for us */
	clear_bss();
	mem_size = memsize;

	/* do subsystem initializations - order matters! */
	systype_init();
	cmos_init();
	serial_init(console);
	
	cache_init(1);
	delay_init();
	systype_init_bsp();
	rtc_init();
	
	/* do delay timings - ifdef'ed in delay.c */
	delay_time_trials();

	/* init i2c before LCD is used */
	i2c_init();
	if (i2c_reset() < 0) {
		printf("I2C bus reset failed\n");
	}

	/* print the first banner the user sees on the console */
	printf("\n\n%s\n%s\n\n", TITLE_STRING, VERSION_STRING);

	/* print out ROM info */
	printf("Current date: %s\n", rtc_get_datestr());
	printf("ROM build info: %s\n", BUILD_INFO);
	printf("System serial number: %s\n", serial_num);
	printf("System type: %s, %s\n", systype_str(), boardtype_str());
	{
		const unsigned char *ssn_string = get_ssn_hex();
		printf("Silicon serial number: %s\n",
		       ssn_string ? (char *)ssn_string : "failed");
	}
	printf("Monitor: %d bytes\n", monitor_size);
	printf("Memory: %d MB\n", (mem_size >> 20)==4080?4096:(mem_size >> 20));
	if (!clean_boot) {
		printf("Warning: Improper shutdown detected\n\n");
	}

	/* backdoor... useful if the rom hangs
	 * during MP boot or PCI init, among other things */
	if (!serial_inb_timeout((char *)&val, BOGO_SEC) && val=='x') {
		eeprom_init();
		fs_init();
		menu_do_console(); 
	}

	/* set shadow regions read-write
	 * for ACPI / PIRQ / MP tables */
	shadow_set_RW();

	/* look for other processors,
	 * create MP tables in shadow memory */
	if (mp_init() > 1) {
		set_kernel_image(BOOT_KERNEL_LINUX_SMP);
	} else {
		set_kernel_image(BOOT_KERNEL_LINUX);
	}


	/* need flash for logo data & microcode updates */
	printf("Initializing flash: ");
	if (eeprom_init() < 0) {
		printf("failed\n");
	} else {
		printf("done\n");
		eeprom_list();
	}

	/* show the logo */
	lcd_init();

	title1 =  malloc(13);
	title2 =  malloc(13);
	if (show_logo) {
		/* save the strings, JIC */
		lcd_get_title( title1, title2 );
	} else {
		strncpy(title1, "            ", 12);
		strncpy(title2, "            ", 12);
	}

#if 0
	/* checksum ourself */
	{
		unsigned int csum;
		printf("ROM checksum: ");
		csum = eeprom_csum();
		printf("0x%08x (%svalid)\n", csum, 
			eeprom_csum_valid ? "" : "in");
	}
#endif
	/* setup dram refresh and memory timings */
	set_memory_timing(systype_get_info(0));

	/* get to CRfs */
	printf("Mounting ROM fs: ");
	fs_init();
	printf("done\n");
	
#ifdef REMOTE_GDB
	printf("Initializing Remote Debugging: ");
	init_exceptions();
	init_debug();
	printf("done\n");
#endif

	printf("Initializing PCI: ");
	pci_init();
	pci_set_hole();
	printf("done\n");
	pci_list();



	/* build MP tables--
	 * run pci_init() first to find PCI IRQ routing info */
	mptable_build();

	/* configure ACPI, build ACPI tables */
	acpi_init();

	/* configure builtin Ethernet */
	init_eth();

#if 0
	/* check for microcode updates */
	update_cpu_microcode();
#endif

	/* setup ide, this takes a LONG time */
	lcd_set_font( 2 );
	lcd_set_cur_pos(0x0);
	lcd_write_char(0); lcd_write_char(1);
	lcd_write_char(2); lcd_write_char(3);
	lcd_set_cur_pos(0x40);
	lcd_write_char(4); lcd_write_char(5);
	lcd_write_char(6); lcd_write_char(7);
	lcd_logo_clear();
	init_ide();
	lcd_set_font( 0 );
	lcd_logo_clear();
	lcd_logo_write(title1, title2);

	/* set shadow region back to read-only */
	shadow_set_RO();

	/* scrub the rest of memory */
	/* this needs to be done after PCI so we dont step on PCI space */
	{
	    int pbar;
	    pbar = delay_clear_amount() >= (128 * 1024 * 1024);
	    
	    if( pbar )
	    {
		lcd_set_font( 1 );
		lcd_logo_clear(); 
	    }

	    printf("Checking Memory: ");
	    delay_clear_rest( pbar );
	    printf("done\n");
	    
	    if( pbar )
	    {
		lcd_set_font( 0 );
		lcd_logo_clear();
		lcd_logo_write(title1, title2);
	    }
	}

	/* initialize boot type options */
	if (boot_type)
		set_boot_opt(BOOT_OPT_MASK, boot_type);

	printf("\nPress spacebar to enter ROM mode\n");
	
	if (lcd_buttons_in_set(lcd_button_read(), BUTTON_SELECT)) {
		/* to enable ROM menu via LCD:
		 * hold "SELECT" button */
		printf("System is in LCD menu mode\n");
		menu_do_lcd(); 
	}
	else if (lcd_buttons_in_set(lcd_button_read(), BUTTON_ENTER)) {
		/* to reset CMOS values to default:
		 * hold "ENTER" button */
		printf("Restoring CMOS defaults\n");
		cmos_set_defaults();
		lcd_logo_clear();
		lcd_logo_write("  CMOS", "  restored");
		while (lcd_buttons_in_set(lcd_button_read(), BUTTON_ENTER))
			;
		boot_default();
	}
	else if ((auto_prompt && console) ||
		 (!serial_inb_timeout((char *)&val, 3*BOGO_SEC) && val==' ')) {
		/* if CMOS aut-prompt flag is set, serial console is enabled,
		 * and char 0x20 (space) is received before 3 second timeout:
		 *     then enter ROM menu mode */
		show_menu_lcdmsg();
		menu_do_console(); 
	}
	else boot_default();

	/* should never get here */
	asm("cli");
	asm("hlt");
}

extern char __bss_start;
extern char _end;
static void clear_bss(void)
{
	unsigned long bss_start, bss_end;
	char *ptr;

	bss_start = (unsigned long )&__bss_start;
	bss_end = (unsigned long )&_end;

	/* kill it all */	ptr = (char *)bss_start;

	memset(ptr, 0, bss_end - bss_start);
}

/* 
 * hook for doing setup the first time a system is powered up 
 */
void do_first_boot(void)
{
	cmos_set_defaults();
	/* this must go after set_defaults so that we boot from net */
	cmos_first_boot_defaults_only( 1 ); 
	rtc_set_defaults();
}

static void show_menu_lcdmsg(void)
{
	lcd_logo_clear();
	lcd_logo_write("  Sys in ROM", "  menu mode");
}

static char *authors[] = {
	"Patrick", "Bose",
	"Moshen", "Chan",
	"Erik", "Gilling",
	"Tuong", "Hoang",
	"Tim", "Hockin",
	"Duncan", "Laurie",
	"Timothy", "Stonis",
	"Adrian", "Sun",
	NULL, NULL
};

void credits(void) 
{
	int i = 0;
	char *f;
	char *l;

	printf("This firmware brought to you by:\n");

	lcd_logo_clear();
	lcd_write_str_at(0x04, "Firmware by:");

	delay(500);

	while (authors[i]) {
		f = authors[i];
		l = authors[i+1];
		printf("\t%s %s\n", f, l);

		lcd_logo_clear();
		lcd_logo_write(f, l);

		delay(500);
		i += 2;
	}

	show_menu_lcdmsg();
}

static void set_memory_timing (struct cpu_info *cpu)
{
	switch (boardtype()) {
	case BOARDTYPE_MONTEREY:
	{
         /* NOTE: we don't want to do anything here except change the
	  *       refresh rate. doing anything else really doesn't
	  *       work.  besides, we want to be ultra-conservative
	  *       with memory settings for the serverworks LE */

		unsigned char val;

		/* this is only valid for front-side bus speed of 100 MHz
		 * (test if FSB > 110 MHz) */
		if (cpu->host_bus_clock > 110) 
			return;

		/* Read the refresh counter register at 79h and
		 * modify it according to the following table:
		 *
		 *     133MHz   100MHz  refresh interval (us)
		 *    ---------------------------------------
		 *      0x1e     0x16   15.4        [default] 
		 *      0x0f     0x0b    7.7
		 *      0x07     0x05    3.85
		 */
		val = pci_readb(0x0, PCI_DEVFN(0,0), 0x79);
		if (val == 0x07)
			val = 0x05;
		else if (val == 0x0F)
			val = 0x0B;
		else /* everything else is slower than 15.4us refresh */
			val = 0x16;
		pci_writeb(0x0, PCI_DEVFN(0,0), 0x79, val);
	}
		break;

	default:
		break;
	}
	return;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
