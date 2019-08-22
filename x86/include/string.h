/*
 * 
 * Filename: string.h
 * 
 * Description: Some general definitions used for manipulating strings 
 * 
 * Author(s): Timothy Stonis
 * 
 * Copyright 1997, 1999, Cobalt Microserver, Inc.
 * 
 */
#ifndef STRING_H
#define STRING_H

#include "common/memory.h" 

/* Some useful macros for string conversion */
#define toupper(x)	(((x) >= 'a' && (x) <='z') ? (x) - 0x20 : (x))
#define tolower(x)	(((x) >= 'A' && (x) <='Z') ? (x) + 0x20 : (x))

#define hexatoi(x)	( (((x) >= '0') && ((x) <= '9')) ? \
				((x) - '0') : ((toupper(x) - 'A') + 10) )
#define itohexa(x)	( (((x) >= 9) && ((x) <= 9)) ? \
				((x) + '0') : (((x) + 'A') - 10) )
#define printable(x)	( ( ( (x) > 0x1f ) && ( (x) < 0x7f ) ) ? (x) : 0x2e )


/* defined in string.c: */

int strlen(char *);
int strcmp(char *, char *);
int strncmp(unsigned char *, unsigned char *, int);
char *strcpy(char *, char *);
char *strncpy(char *, char *, int len);
char *strcat(char *, char *);
char *strncat(char *, char *, int);
char *strdup(char *);
unsigned char stoli( unsigned char *, unsigned long *);
void itohexs(unsigned char *, unsigned long, unsigned char);
void catpstr(unsigned char *, unsigned char *);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
