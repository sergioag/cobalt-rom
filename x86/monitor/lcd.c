/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: lcd.c,v 1.1.1.1 2003/06/10 22:42:19 iceblink Exp $
 */

#include <string.h>

#include "common/rom.h"
#include "common/lcd.h"

#include "lcd.h"

int lcd_func(int argc, char *argv[])
{
    if (argc != 2)
	return EBADNUMARGS;

    if (!strcmp(argv[1], "on")) {
	lcd_on();
    } else if (!strcmp(argv[1], "off")) {
	lcd_off();
    } else if (!strcmp(argv[1], "init")) {
	lcd_init();
    } else if (!strcmp(argv[1], "reset")) {
	lcd_reset();
    } else if (!strcmp(argv[1], "clear")) {
	lcd_clear();
    } else {
	return EBADARG;
    }

    return 0;
}

int lcd_sysinfo_func(int argc, char *argv[] __attribute__ ((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;

	lcd_set_cur_pos(0x00);
	lcd_write_str("                    ");
	lcd_set_cur_pos(0x00);
	lcd_write_str("Firmware version");
	lcd_set_cur_pos(0x40);
	lcd_write_str("                    ");
	lcd_set_cur_pos(0x40);
	lcd_write_str( VERSION );

	/* wait */
	while(lcd_button_read() == BUTTON_NONE)
		;

	return 0;
}

// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
