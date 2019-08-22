/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: eeprom.h,v 1.1.1.1 2003/06/10 22:42:19 iceblink Exp $
 */

#ifndef MONITOR_EEPROM_H
#define MONITOR_EEPROM_H


int display_romid_func(int argc, char *argv[]);
int set_bank_func(int argc, char *argv[]);
int erase_eeprom_func(int argc, char *argv[]);
int write_eeprom_func(int argc, char *argv[]);
int read_eeprom_func(int argc, char *argv[]);
int reinit_eeprom_func(int argc, char *argv[] __attribute__((unused)));

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
