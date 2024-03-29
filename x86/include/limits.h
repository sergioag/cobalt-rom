/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: limits.h,v 1.1.1.1 2003/06/10 22:42:00 iceblink Exp $ */

#ifndef LIMITS_H
#define LIMITS_H

/* Number of bits in a `char'.	*/
#define CHAR_BIT	8

/* Minimum and maximum values a `signed char' can hold.  */
#define SCHAR_MIN	(-128)
#define SCHAR_MAX	127

/* Maximum value an `unsigned char' can hold.  (Minimum is 0.)  */
#define UCHAR_MAX	255

/* Minimum and maximum values a `char' can hold.  */
#define CHAR_MIN	SCHAR_MIN
#define CHAR_MAX	SCHAR_MAX

/* Minimum and maximum values a `signed short int' can hold.  */
#define SHRT_MIN	(-32768)
#define SHRT_MAX	32767

/* Maximum value an `unsigned short int' can hold.  (Minimum is 0.)  */
#define USHRT_MAX	65535

/* Minimum and maximum values a `signed int' can hold.  */
#define INT_MIN		(-INT_MAX - 1)
#define INT_MAX		2147483647

/* Maximum value an `unsigned int' can hold.  (Minimum is 0.)  */
#define UINT_MAX	4294967295U

/* Minimum and maximum values a `signed long int' can hold.  */
#define LONG_MAX	2147483647L
#define LONG_MIN	(-LONG_MAX - 1L)

/* Maximum value an `unsigned long int' can hold.  (Minimum is 0.)  */
#define ULONG_MAX	4294967295UL

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
