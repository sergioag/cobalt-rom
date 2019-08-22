/*
 * 
 * Filename: string.c
 * 
 * Description: Functions for string support
 * 
 * Author(s): Timothy Stonis, Tim Hockin
 * 
 * Copyright 1997,1999 Cobalt Networks, Inc.
 * 
 */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common/rom.h"


/*
 * Function: stoli
 *
 * Description: ASCII string to long integer - 0x prefix indicates hex
 *					     - no prefix indicates decimal
 *
 * In: String value, address of result
 *
 * Out: Status 0 = success, 1 = bad string
 * 
 */
unsigned char stoli( unsigned char *val, unsigned long *addr )
{
  unsigned char ctr = 0;

  *addr = 0;
   
  /* Do we want a hex conversion ? */
  if (val[1] == 'x'  || val[1] == 'X') {
      ctr = 2;
      while (val[ctr]) {
          val[ctr] = toupper(val[ctr]);
          if (isxdigit(val[ctr])) {
              *addr = ((*addr)<<4) + hexatoi(val[ctr]); 
          } else {
              return 1;
          }
          ctr++;
      } 
      if ((ctr > 10) || (ctr == 2)) {
          return 1;
      }
  } else {
      /* int conversion */
      while (val[ctr]) {
          if (isdigit(val[ctr])) {
              *addr = ((*addr)*10) + hexatoi(val[ctr]);
          } else {
              return 1;
	  }
          ctr++;
      }
      if (ctr > 12) {
          return 1;
      }
  }

  return 0;
}


/*
 * strlen
 *
 * works just like the library function
 */
int strlen( char * str )
{
	int len=0;
	while( *str )
	{
		len++;
		str++;
	}
	return len;
}

/*
 * strcmp 
 *
 * works just like the library function 
 */

int strcmp( char * str1, char *str2 )
{
    while( (*str1) &&
	   (*str2) &&
	   (*str1 == *str2) )
    {
	str1++;
	str2++;
    }
    return *str1 - *str2;
}

/*
 * strcpy 
 *
 * works just like the library function 
 */

char *strcpy( char *str1, char *str2 )
{
    while( *str2 ) {
	*(str1++) =  *(str2++);
    }
    *str1='\0';
    return str1;
}

/*
 * strncpy
 *
 * works just like the library function
 */
char *strncpy( char *str1, char *str2, int len )
{
    while( len-- && *str2 )
    {
		*(str1++) =  *(str2++);
    }
    *str1='\0';
    return str1;
}

/*
 * strcat
 *
 * works like the library function
 */
char *strcat( char *str1, char *str2 )
{
    while( *str1++ )
	;

    str1--;

    while( *str2 )
    {
	*str1++ = *str2++;
    }
    *str1='\0';
    return str1;
}

/*
 * strncat
 *
 * works like the library function
 */
char *strncat( char *str1, char *str2, int len )
{
    while( *str1++ )
	;

    str1--;

    while( len-- && *str2 )
    {
	*str1++ = *str2++;
    }
    *str1='\0';
    return str1;
}

//
// Function: itohexs
//
// Description: Convert a byte, word, or long to hex string
//
// In: Target string, value to convert, size of value (val should be 
//     cast to long)
//
// Out: Converted string
//
  
void itohexs(unsigned char *str, unsigned long val, unsigned char type)
{
 int ctr;
 int x;
 
 str[0]='0';
 str[1]='x';
 
 for (ctr=0; ctr<(type); ctr++) {
     x = ((val >> (ctr*4)) & 0x000F);
     str[type+1-ctr] = itohexa( x );
 }
  
 str[ctr+2] = '\0';

}
 
//
// Function: catpstr
//
// Description: Concatenate printable 4 bytes str2 onto str1 ("." otherwise)  
//
// In: Two strings
//
// Out: Concatenated string
//
  
void catpstr(  unsigned char *str1, unsigned char *str2 )
{
 unsigned char *ptr, *ptr2;
 int ctr = 0;
 
 ptr = str1;
 ptr2 = str2;
 
 // Find the end of str1
 while ( (*ptr) != '\0' )
  ptr++;

 // 
 do { 
  if ( ( *ptr2 > 0x1F) && (*ptr2 < 0x7F) )
   *ptr = *ptr2;
  else
   *ptr = 0x2E;
   
  ptr++;
  ptr2++;  
  ctr++;
 } while ( ctr < 4 );

 *(ptr) = '\0';

}

//
// Function: strncmp
//
// Description: Compare n characters of str1 and str2 for equality
//
// In: Two strings
//
//
  
int strncmp( unsigned char *str1, unsigned char *str2, int n )
{

    while (n && (*str1) && (*str2) && (*str1 == *str2)) {
	str1++;
	str2++;
	n--;
    }

    if (n) {
    	return *str1 - *str2;
    } else {
	return 0;
    }
}

/* 
 * Function: strdup
 * 
 * Works just like the library func
 * 
 * In: a string
 * Out: the same string, malloc'ed
 *
 */
char *strdup(char *s)
{
	char *new;
	
	new = (char *)malloc(sizeof(char) * (strlen(s)+1));
	if (new) {
		/* safe because we have enough mem */
		strcpy(new, s);
	}

	return new;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
