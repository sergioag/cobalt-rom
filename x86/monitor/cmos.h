/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: cmos.h,v 1.2 2003/10/06 15:15:03 iceblink Exp $
 */

#ifndef MONITOR_CMOS_H
#define MONITOR_CMOS_H

int read_cmos_func(int, char *[]);
int write_cmos_func(int, char *[]);
int dump_cmos_func( int argc, char *argv[] __attribute__((unused)) );
int check_cmos_cks_func( int argc, char *argv[] __attribute__((unused)) );
int update_cmos_cks_func( int argc, char *argv[] __attribute__((unused)) );
int cmos_reset_func(int argc, char *argv[] __attribute__((unused)) );
int cmos_ver_func(int argc, char *argv[] __attribute__((unused)) );
int bto_ip_func( int argc, char *argv[] );
int bto_code_func(int, char *[]);
int cmos_flags_func(int argc, char *argv[] __attribute__((unused)) );

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
