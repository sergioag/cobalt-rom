/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: menu.h,v 1.1.1.1 2003/06/10 22:42:02 iceblink Exp $
 */

#ifndef COMMON_MENU_H
#define COMMON_MENU_H

/***************************************************************
 * the important structures
 **************************************************************/
typedef enum {
	MENU_TYPE_NULL = 0,
	MENU_TYPE_CMD  = 1,
	MENU_TYPE_MENU = 2
} menu_item_type;

struct menu_item_t {
	char *cmd;				/* how to invoke this cmd */
	char *title;/* for MENU_TYPE_MENU, params for MENU_TYPE_CMD */
	char *help;				/* help text */
	menu_item_type type;			/* menu or cmd ? */
	union {
        	struct menu_item_t **items;	/* array of cmds */
        	int (*func)(int, char *[]);	/* function for cmd */
	} action;
	struct menu_item_t *parent;		/* parent menu of a menu */
}; 
typedef struct menu_item_t menu_item;
/**************************************************************/

void menu_handle_console(menu_item *the_menu);
void menu_handle_lcd(menu_item *the_menu);


#define MENU_BUFFER_LEN	0xA0
#define MENU_MAX_ARGS	0x10

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
