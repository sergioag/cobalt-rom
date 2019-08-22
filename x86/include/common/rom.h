/*
 *
 * Filename: rom.h
 *
 * Description: Some general definitions used by the rom 
 *
 * Author(s): Timothy Stonis, Tim Hockin
 *
 * Copyright 1999, Cobalt Networks, Inc.
 *
 */
#ifndef COMMON_ROM_H
#define COMMON_ROM_H

#include "limits.h"
#include "extra_types.h"
#include "printf.h"
#include "common/x86.h"

#ifndef NULL
# define NULL	(void *)0
#endif

#ifndef offsetof
# define offsetof(TYPE, MEMBER) ((size_t)&((TYPE *)0)->MEMBER)
#endif

#define BOGO_SEC        0x0003F940

/* functions with no better home */
void do_first_boot(void);
void shadow_set_RW(void);
void shadow_set_RO(void);

/* these are output in the startup sequence - VERSION is in the Makefile */
#define TITLE_STRING		\
 	"         Sun Cobalt - Smaller, Bluer, Better, and Free"
#define VERSION_STRING  	\
	"               Firmware version " VERSION

/* for debugging */
#define DPRINTF(fmt, args...)	if (debug) { printf(fmt, ##args); }
#define PCI_POST(code)		outb((code), 0x80)

/* common return values */
#define EBADNUMARGS	-11
#define EBADNUM		-12
#define EBADARG		-13

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
