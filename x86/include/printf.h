/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */

#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>
#include <string.h>

int printf(const char *fmt, ...) 
	__attribute__((format(printf, 1, 2)));
int sprintf(char *str, const char *fmt, ...) 
	__attribute__((format(printf, 2, 3)));
int snprintf(char *str, int len, const char *fmt, ...) 
	__attribute__((format(printf, 3, 4)));

int getc(void);
int putc(char c);
int puts(char *str);

const char *print_bytes(uint32);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
