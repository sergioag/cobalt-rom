/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: cache.h,v 1.1.1.1 2003/06/10 22:42:00 iceblink Exp $ */

#ifndef COMMON_CACHE_H
#define COMMON_CACHE_H

void cache_init(int in_monitor);
void mtrr_init(void);
void cache_set_caches(int l1, int l2);
void enable_caches(void);
void cache_l1_on(void);
void cache_l1_off(void);
void cache_l2_on(void);
int cache_l1_status(void);
int cache_l2_status(void);
int cache_l1_should_be(void);
int cache_l2_should_be(void);

/* 
 * p6 back door cache commands
 */

/*
 * Control Register Read
 *
 * cache_p6_cmd_cr   
 *   reg - register to read
 *
 * reads the reg from the TAGRam configuration registers
 */
unsigned char cache_p6_cmd_cr(int reg);

/*
 * Control Register Write
 *
 * cache_p6_cmd_cw
 *   reg - register to write to
 *   data - data to write to register
 *
 * writes data into reg on the TAGRam configuration registers
 */
void cache_p6_cmd_cw(int reg, unsigned char data);

/*
 * Tag Inquite
 *
 * cache_p6_cmd_ti
 *   addr - address to which the tag corresponds
 *   hit - to be filled with tag hit/miss data
 *   state - to be filled with tag state data
 *   way - to be filled with tag way data
 *
 * does a tag inquire on addr and fills hit, state and way 
 */
void cache_p6_cmd_ti(unsigned int addr, 
		     int * hit, int * state, int * way);

/*
 * Data Read with LRU Update
 *
 * cache_p6_cmd_rlu
 *   addr - address of data to read
 *   hit - to be filled with tag hit/miss data
 *   state - to be filled with tag state data
 *   way - to be filled with tag way data
 *   line - to be filled with he cache line
 *   ecc - to be filled with ecc data
 *
 * does a data read with lru update and fills hit, state, way
 * line, and ecc.
 */
void cahce_p6_cmd_rlu(unsigned int addr, int *hit, int *state, int *way,
		      unsigned char *line, unsigned char *ecc);

/*
 * Tag Read with Data Read
 *
 * cache_p6_cmd_trr
 *   tag_addr - address to which the tag corresponds
 *   way - way of the tag
 *   data_addr - to be filled with the victum address
 *   line - to be filled with he cache line
 *   ecc - to be filled with ecc data
 *
 * finds the tag at tag_addr and way and fills data_addr, line, and ecc.
 */
void cache_p6_cmd_trr(unsigned int tag_addr, int way, unsigned int *data_addr,
		      unsigned char *line, unsigned int *ecc);

/*
 * Tag Write with Data Read
 *
 * cache_p6_cmd_twr
 *   addr - address of tag
 *   hit - tag hit/miss data
 *   state - tag state data
 *   way - tag way data
 *   line - to be filled with he cache line
 *   ecc - to be filled with ecc data
 *
 * sets the tag at addr with hit, state, and way and reads the corresponding 
 * cache line into line and ecc.
 */
void cache_p6_cmd_twr(unsigned int addr, int way, int state,
		      unsigned char *line, unsigned int *ecc);

/*
 * Tag Write
 *
 * cache_p6_cmd_tw
 *   addr - address of tag
 *   hit - tag hit/miss data
 *   state - tag state data
 *   way - tag way data
 *
 * sets the tag at addr with hit, state, and way.
 */
void cache_p6_cmd_tw(unsigned int addr, int way, int state);

/*
 * Tag Write with Data Read
 *
 * cache_p6_cmd_twr
 *   addr - address of tag
 *   hit - tag hit/miss data
 *   state - tag state data
 *   way - tag way data
 *   line - cache line
 *   ecc - ecc data
 *
 * sets the tag at addr with hit, state, and way and the corresponding 
 * cache line into line and ecc.
 */
void cache_p6_cmd_tww(unsigned int addr, int way, int state,
		      unsigned char *line, unsigned int *ecc);
		       
#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
