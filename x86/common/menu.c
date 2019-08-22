/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
#include <string.h>
#include <printf.h>
#include <stdlib.h>
#include <ctype.h>

#include "common/rom.h"
#include "common/menu.h"
#include "common/readline.h"
#include "common/lcd.h"
#include "common/delay.h"

static void menu_do_help(menu_item *);
static int split_buffer(char * buffer, char **argv);
void credits(void);

void menu_handle_console( menu_item *the_menu )
{
    char *buffer;
    int argc;
    char *argv[MENU_MAX_ARGS];
    menu_item *m;
    int i;

    while (1) {
	/* prompt - this should be OEM'ified */
	printf("\nCobalt:%s> ", the_menu->title);

	/* read the line entered */
	buffer = readline();

	/* break it for parsing */
	argc = split_buffer(buffer, (char **)argv);

	/* blank lines get ignored */
	if (!argc) { 
	    continue;
	}

	/*
	 * Here are a number of builtin commands that are available from 
	 * all menus.  
	 */

	/* help */
	if ((!strcmp(argv[0], "?")) || (!strcmp(argv[0], "help"))) {
	    menu_do_help(the_menu);
	    continue;
	}

	/* go up a menu */
	if ((!strcmp(argv[0], "up")) || (!strcmp(argv[0], ".."))) {  
	    if (the_menu->parent) {
	    	the_menu = the_menu->parent;
	    }
	    continue;
	}

	/* show version */
	if ((!strcmp(argv[0], "ver")) || (!strcmp(argv[0], "version"))) {
		printf("Firmware version: %s\n", VERSION);
		printf("ROM Build Info: %s\n", BUILD_INFO);
		continue;
	}

	/* dump the command history */
	if (!strcmp(argv[0], "history")) {
	 	readline_history();
		continue;
	}
	
	/* show credits */
	if (!strcmp(argv[0], "credits")) {
		extern void credits(void);
		credits();
		continue;
	}

	/* it must be a menu cmd - find it */
	i = 0;
	m = the_menu->action.items[i];
	while (m) {
	    if(!strcmp(argv[0], m->cmd)) {
		/* found it */
		if (m->type == MENU_TYPE_CMD) {
			int r;
			r = m->action.func(argc, (char **)argv);
			if (r) {
			    printf("Syntax error: ");
			    if (r == EBADNUMARGS) {
			    	printf("wrong number of arguments");
			    } else if (r == EBADNUM) {
			    	printf("bad number format");
			    } else if (r == EBADARG) {
			    	printf("invalid argument");
			    } else {
			    	printf("unknown error");
			    }
			    printf(" (use ? or help for help info)\n");
			}
			break;
		} else if (m->type == MENU_TYPE_MENU) {
			the_menu = m;
			break;
		}
	    }
	    m = the_menu->action.items[i];
	    i++;
	}

	/* didn't find it */
	if (!m) {
		printf("Invalid command\n");
	}
    } 
}

void menu_handle_lcd( menu_item * the_menu )
{
    int item=0;
    unsigned int buttons = 0;
    char func_name[0x100];
    char *tmp;
    
    lcd_set_cur_pos(0x00);
    lcd_write_str("                    ");
    lcd_set_cur_pos(0x00);
    lcd_write_str(the_menu->title);
    lcd_set_cur_pos(0x40);
    lcd_write_str("                    ");
    lcd_set_cur_pos(0x40);

    /* wait for select to be released */
    while (lcd_buttons_in_set(lcd_button_read(), BUTTON_SELECT))
	;

    while (1) {
	lcd_set_cur_pos(0x00);
        lcd_write_str("                    ");
        lcd_set_cur_pos(0x00);
	lcd_write_str(the_menu->title);
	lcd_set_cur_pos(0x40);
	lcd_write_str("                    ");
	lcd_set_cur_pos(0x40);
	lcd_write_str(the_menu->action.items[item]->cmd);
	
	/* snarf up any repeats - must let up on the button */
	while (lcd_button_read() == buttons)
		delay(10);

	/* they may have held enter too long (for submenus) */
	while(lcd_buttons_in_set(lcd_button_read(), BUTTON_ENTER))
		delay(10);

	/* wait */
	while((buttons = lcd_button_read()) == BUTTON_NONE)
		;

	/* button(s) pressed - process... */

	/* scroll down */
	if ((lcd_button_eq(buttons, BUTTON_SELECT)) 
	 ||  (lcd_button_eq(buttons, BUTTON_RIGHT)) 
	 ||  (lcd_button_eq(buttons, BUTTON_DOWN))) {
	    item++;
	    if(!the_menu->action.items[item]) {
		item = 0;
	    }
	    continue;
	} 

	/* scroll up */
	if ((lcd_button_eq(buttons, BUTTON_UP)) 
	 ||  (lcd_button_eq(buttons, BUTTON_LEFT))) {
	    item--;
	    
	    /* this sorta sucks.  O(n) and all */
	    if(item < 0) {
		while(the_menu->action.items[item+1])
		    item++;
	    }
	    continue;
	}

	/* hit enter */
	if (lcd_button_eq(buttons, BUTTON_ENTER)) {
	    switch(the_menu->action.items[item]->type) {
		case MENU_TYPE_CMD:
		    strcpy(func_name, the_menu->action.items[item]->cmd);
		    tmp = func_name;
		    the_menu->action.items[item]->action.func(1, &tmp);
		    /* if we get here - we want to go up a menu */
		    if (the_menu->parent) {
		    	the_menu = the_menu->parent;
		    	item = 0;
		    }
		    break;

		case MENU_TYPE_MENU:
			the_menu = the_menu->action.items[item];
			item = 0;
			break;

	        case MENU_TYPE_NULL:
			break;
	    }
	}
    }
}

#define HELP_START	38
static void 
menu_do_help(menu_item *the_menu)
{
	int i=0;
	int len;

	printf("\n%s\n", the_menu->title);
	printf("------------------------------------------------------\n");

	while (the_menu->action.items[i]) {
		char *p;
		/* print the cmd */
		len = printf("%s", the_menu->action.items[i]->cmd);
		if (the_menu->action.items[i]->type == MENU_TYPE_CMD
		 && the_menu->action.items[i]->title) {
			/* print any parameters */
			len += printf(" %s  ", 
				the_menu->action.items[i]->title);
		}
		/* pad to the magic place */
		while (len++ <= HELP_START) printf(" ");
		/* print the help */
		p = the_menu->action.items[i]->help;
		while (p && *p) {
			printf("%c", *p);
			if (*p == '\n') {
				len = 0;
				while (len++ <= HELP_START) printf(" ");
			}
			p++;
		}
		printf("\n");
		i++;
	}
	printf("\n");
}

typedef enum {
	ST_BLANK = 1,
	ST_QSTR,
	ST_TOKEN,
} buf_state_t;
/*
 * split_buffer
 *   buffer - buffer to split
 *   argv   - space to put split args in
 *
 * splits the buffer on white space and sticks the results into
 * argv and returns the number of valid elements in argv
 */
static int 
split_buffer(char *buffer, char **argv)
{
	buf_state_t state = ST_BLANK;
	int argc = 0;
	char qchar = 0;
    
	while (*buffer) {
		switch (state) {
		  case ST_BLANK:
			/* skip leading blank space */
			if (!isblank(*buffer)) {
				if (*buffer == '"' || *buffer == '\'') {
					qchar = *buffer;
					argv[argc] = buffer+1;
					state = ST_QSTR;
				} else {
					argv[argc] = buffer;
					state = ST_TOKEN;
				}
				argc++;
			}
			break;
		
		  case ST_TOKEN:
			/* read a token */
			if (isblank(*buffer)) {
				*buffer = '\0';
				state = ST_BLANK;
			} else if (*buffer == '"' || *buffer == '\'') {
				qchar = *buffer;
				state = ST_QSTR;
			}
			break;

		  case ST_QSTR:
			/* read until we can close the quoted string */
			if (*buffer == qchar) {
				qchar = 0;
				/* FIXME: should support "foo"bar ->foobar */
				if (isblank(*(buffer+1))) {
					state = ST_BLANK;
				} else {
					state = ST_TOKEN;
				}
				*buffer = '\0';
			}
			break;
		}
		buffer++;
	}

	return argc;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
