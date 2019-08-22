/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* 
 * $Id: i2c.h,v 1.1.1.1 2003/06/10 22:42:19 iceblink Exp $
 */

#ifndef MONITOR_I2C_H
#define MONITOR_I2C_H

int read_ics_func( int argc, char *argv[] );
int write_ics_func( int argc, char *argv[] );
int read_voltage_func( int argc, char *argv[] );
int read_temp_func( int argc, char *argv[] );
int read_temp_thresh_func( int argc, char *argv[] );
int write_temp_thresh_func( int argc, char *argv[] );
int read_i2c_func(int argc, char *argv[]);
int write_i2c_func(int argc, char *argv[]);
#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
