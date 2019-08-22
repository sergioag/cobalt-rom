/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <printf.h>


#include "crfs_func.h"
#include "crfs_block.h"

#include "common/rom.h"
#include "common/i18n.h"

typedef struct {
    unsigned int id;
    char *str;
} i18n_string;

#define I18N_MAX_STRINGS 	2048
#define I18N_MAX_STRLEN 	2048
#define I18N_MAX_KEYLEN 	32

static i18n_string i18n_strings[I18N_MAX_STRINGS];
static int i18n_num_strings;

int init_i18n( char *lang )
{
    CO_FILE *fp;
    char fn[32];
    int error;

    strcpy( fn, "/strings." );
    strcat( fn, lang );

    if( (fp = crfs_open( fn )) == NULL ) {
	return -1;
    }

    if( (error = i18n_parse_file( fp )) < 0 ) {
	printf("error parsing strings file %s at line %d - ", fn, -error);
	return error;
    }

    return 0;
}

char *i18n_lookup_str( unsigned int id )
{
    int i;
    for( i=0 ; i<i18n_num_strings; i++ )
    {
	if( i18n_strings[ i ].id == id )
	    return i18n_strings[i].str;
    }

    return "String not found";
}

enum {
	STATE_BOL = 0, 		/* beginning of a line - suck up space */
	STATE_FOUND_0, 		/* found start of hex id - look for 'x' */
	STATE_FOUND_X,		/* found 'x' of hex id - need 1 hex digit */
	STATE_READ_HEX,		/* found 1 hex, eat any more */ 
	STATE_POST_HEX_BLANK,	/* eat space after hex id */
	STATE_FOUND_EQ,		/* found '=' - eat space until '"' */
	STATE_READ_STRING,	/* found '"' - read the string value */
	STATE_END_STRING,	/* found a closing '"' - prep to end line */
	STATE_ESCAPE,		/* found a '\' char - process an escape */
	STATE_ESCAPED_OCTAL,	/* found what seems to be an escaped octal */
	STATE_COMMENT		/* found a comment - eat until end of line */
};

/* macros to save values to the buffers safely */
#define SAVE_VAL_CHAR(c) 	{\
				  if (tmp_n < (I18N_MAX_STRLEN - 1)) {\
					tmp[tmp_n++] = (c);\
				  }\
			 	}
#define SAVE_KEY_CHAR(c) 	{\
				  if (key_n < (I18N_MAX_KEYLEN - 1)) {\
					key[key_n++] = (c);\
				  }\
			 	}

/*  This is a simple finite state machine 
 *  there are abundant comments inside
 *
 *  FIXME: Will this work for non-english languages?
 */
int i18n_parse_file( CO_FILE *fp )
{
    char buff[CRFS_BLOCK_SIZE];
    char *p;
    char tmp[I18N_MAX_STRLEN];
    char key[I18N_MAX_KEYLEN];
    int tmp_n=0, key_n=0;
    char c;
    int state = STATE_BOL;
    int read_bytes;
    unsigned int tmp_val;
    int chars = 0;
    int lineno = 0;

    while( (read_bytes = 
	    crfs_read( fp, (void *) buff, CRFS_BLOCK_SIZE )) >0)
    {
	p=buff;
	while( read_bytes-- ) {
	    c = *p++;

	    /*printf("%c",c);*/

	    switch(state) {
		/* start state - beginning of line */
		case STATE_BOL:
		    lineno++;
		    if (lineno >= I18N_MAX_STRINGS) {
			return -lineno;
		    }
		    if (isspace(c)) {
			/* ignore leading space */
			state = state;
		    } else if (c == '#') {
			/* we found a comment line */
			state = STATE_COMMENT;
		    } else if (c == '0') {
			/* start of hex number */
			SAVE_KEY_CHAR(c);
			state = STATE_FOUND_0;
		    } else {
			return -lineno;
		    }
		    break;

		/* found a '0' - look for the 'x' in the hex num */
		case STATE_FOUND_0:
		    if (c == 'x' || c == 'X') {
			/* we have an x - go for a hex digit now */
			SAVE_KEY_CHAR('x');
			state = STATE_FOUND_X;
		    } else {
			return -lineno;
		    }
		    break;

		/* found the 'x' - look for the first hex digit */
		case STATE_FOUND_X: 
		    if (isxdigit(c)) {
			/* found the first hex digit - more are optional */
			SAVE_KEY_CHAR(c);
			state = STATE_READ_HEX;
		    } else {
			return -lineno;
		    }
		    break;

		/* reading the hex number - look for the end */ 
		case STATE_READ_HEX: 
		    if (isxdigit(c)) {
			/* more hex digits */
			SAVE_KEY_CHAR(c);
			state = state;
		    } else if (isblank(c)) {
			/* hex is done */
			SAVE_KEY_CHAR('\0');
			state = STATE_POST_HEX_BLANK;
		    } else if (c == '=') {
			/* hex is done */
			SAVE_KEY_CHAR('\0');
			state = STATE_FOUND_EQ;
		    } else {
			return -lineno;
		    }
		    break;

		/* we've gotten thru with the key - find an '=' */
		case STATE_POST_HEX_BLANK:
		    if (isblank(c)) {
			/* keep going */
			state = state;
		    } else if (c == '=') {
			/* got the fulcrum */
			state = STATE_FOUND_EQ;
		    } else {
			return -lineno;
		    }
		    break;

		/* found '=' - look for '"' */
		case STATE_FOUND_EQ:
		    if (isblank(c)) {
			/* throw spaces away */
			state = state;
		    } else if (c == '"') {
			/* found the start of the string */
			state = STATE_READ_STRING;
		    } else {
			return -lineno;
		    }
		    break;

		/* found '"' - read the string */
		case STATE_READ_STRING:
		    if (c == '"') {
			/* end of string */
			state = STATE_END_STRING;
		    } else if (c == '\\') {
			/* found an escape */
			state = STATE_ESCAPE;
		    } else if (c == '\n') {
			/* premature EOL */
			return -lineno;
		    } else {
			/* save the char we read */
			SAVE_VAL_CHAR(c);
			state = state;
		    }
		    break;

		/* found a closing '"' - deal with it */
		case STATE_END_STRING:
		    if (isblank(c)) {
			/* eat space */
			state = state;
		    } else if (c == '\n') {
			/* got the end of line */
			SAVE_VAL_CHAR('\0');
			if (stoli(key, (unsigned long *)&tmp_val) != 0) {
			    return -lineno;
			    break;
			}
			i18n_strings[i18n_num_strings].id = tmp_val;
			i18n_strings[i18n_num_strings].str = strdup(tmp);
			i18n_num_strings++;
			key_n = tmp_n = 0;

			state = STATE_BOL;
		    } else if (c == '"') {
			/* second portion of the string - multiline string ? */
			state = STATE_READ_STRING;
		    } else {
			return -lineno;
		    }
		    break;

		/* found a '\' escape char */
		case STATE_ESCAPE:
		    if ((c >= '0') && (c <= '7')) {
			/* looks like an escaped octal */
			tmp_val = (c - '0');
			chars = 1;
			state = STATE_ESCAPED_OCTAL;
		    } else if (isalpha(c)) {
			/* is it a regular escape char? */
			switch (c) {
			    case 't':
				SAVE_VAL_CHAR('\t');
				break;
			    case 'n':
				SAVE_VAL_CHAR('\n');
				break;
			    case 'b':
				SAVE_VAL_CHAR('\b');
				break;
			    case 'r':
				SAVE_VAL_CHAR('\r');
				break;
			    case '\\':
				SAVE_VAL_CHAR('\\');
				break;
			    default:
				/* not a known escape - just print it */
				SAVE_VAL_CHAR(c);
				break;
			}
			state = STATE_READ_STRING;
		    } else {
			/* something else - just print the literal */
			SAVE_VAL_CHAR(c);
			state = STATE_READ_STRING;
		    }
		    break;

		/* found an escaped octal char */
		case STATE_ESCAPED_OCTAL:
		    if ((chars < 3) && ((c >= '0') && (c <= '7'))) {
			/* it looks like it is still part of an octal */
			tmp_val <<= 3;
			tmp_val += (c - '0');
			chars++; 
			state = state;
		    } else {
			/* save the octal value and the current val */
			SAVE_VAL_CHAR((char)(tmp_val & 0xFF));
			SAVE_VAL_CHAR(c);
			state = STATE_READ_STRING;
		    }
		    break;

		/* found a comment - clear to EOL */ 
		case STATE_COMMENT:
		    if (c == '\n') {
			/* done */
			state = STATE_BOL;
		    } else {
			/* keep going */
			state = state;
		    }
		    break;
	    }
	}
    }

    return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
