/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: memory.h,v 1.1.1.1 2003/06/10 22:42:20 iceblink Exp $
 */

#ifndef MONITOR_MEMORY_H
#define MONITOR_MEMORY_H

int read_mem_func(int argc, char *argv[]);
int write_mem_func(int argc, char *argv[]);
int inbwl_func(int argc, char *argv[]);
int outbwl_func(int argc, char *argv[]);
int disp_mem_func(int argc, char *argv[]);
int copy_mem_func(int argc, char *argv[]);
int move_mem_func(int argc, char *argv[]);
int mem_test_func(int argc, char *argv[]);
int uber_mem_test_func(int argc, char *argv[]);
int cache_test_func(int argc, char *argv[]);
int l1_func(int argc, char *argv[]);
int l2_func(int argc, char *argv[]);
int read_test_func(int argc, char *argv[]);
int write_test_func(int argc, char *argv[]);
int disp_mem_eeprom_func(int argc, char *argv[]);
int read_msr_func(int argc, char *argv[]);
int write_msr_func(int argc, char *argv[]);
int nmi_enable_func(int argc, char *argv[] __attribute__ ((unused)));
int nmi_disable_func(int argc, char *argv[] __attribute__ ((unused)));

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
