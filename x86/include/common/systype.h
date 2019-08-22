/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: systype.h,v 1.1.1.1 2003/06/10 22:42:04 iceblink Exp $
 *
 * allows one to distinguish between the different systems types
 *
 */

#ifndef COMMON_SYSTYPE_H
#define COMMON_SYSTYPE_H

#include <extra_types.h>

typedef enum {
	SYSTYPE_UNKNOWN	= 0,
	SYSTYPE_3000 = 0x10000,
	SYSTYPE_5000 = 0x20000
} systype_t;

#define SYSTYPE_3K		SYSTYPE_3000
#define SYSTYPE_5K		SYSTYPE_5000

typedef enum {
	BOARDTYPE_UNKNOWN = 0,
	BOARDTYPE_PACIFICA = 0x1,
	BOARDTYPE_CARMEL = 0x2,
	BOARDTYPE_MONTEREY = 0x3,
	BOARDTYPE_ALPINE = 0x4
} boardtype_t;

extern __u8 global_cpu_map;

struct cpu_info {
	struct cpu_info *NEXT;
	char vendor_string[13];
	byte family, model, stepping, type;
	dword feature_mask, l2_cache;
	byte clock_ratio;
	dword cpu_clock, host_bus_clock;
	byte bsp, apic_id, apic_version;
};

void systype_init(void);

unsigned int systype(void);
char *systype_str(void);
unsigned int boardtype(void);
char *boardtype_str(void);
char *systype_productid_str(void);
int systype_is_3k(void);
int systype_is_5k(void);

struct cpu_info *systype_get_info(int);
struct cpu_info *systype_get_info_ID(int);
const char *systype_cpu_vendor(int);
int systype_cpu_model(int);
int systype_cpu_family(int);
int systype_cpu_type(int);
int systype_cpu_stepping(int);
byte systype_clock_ratio(int);
unsigned int systype_cpu_clock(int);
unsigned int systype_host_bus_clock(int);
int systype_l2_cache_size(int);

void systype_init_bsp(void); 
void systype_init_ap(void);
void systype_init_ap_ID(int);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
