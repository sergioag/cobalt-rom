/* $Id: microcode.h,v 1.1.1.1 2003/06/10 22:42:21 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 *
 * Duncan Laurie
 */

#ifndef MONITOR_MICROCODE_H
#define MONITOR_MICROCODE_H

int cpuid_func(int argc, char *argv[]);
int microcode_func(int argc, char *argv[] __attribute__ ((unused)));

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
