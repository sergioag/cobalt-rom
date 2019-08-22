/* $Id: smp.h,v 1.1.1.1 2003/06/10 22:42:22 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 */

#ifndef MONITOR_SMP_H
#define MONITOR_SMP_H

int lapic_set_func(int argc, char *argv[] __attribute__((unused)));
int lapic_get_func(int argc, char *argv[] __attribute__((unused)));
int lapic_on_func(int argc, char *argv[] __attribute__((unused)));
int lapic_off_func(int argc, char *argv[] __attribute__((unused)));
int lapic_send_ipi_func(int argc, char *argv[] __attribute__((unused)));
int lapic_cast_ipi_func(int argc, char *argv[] __attribute__((unused)));
int ioapic_get_func(int argc, char *argv[] __attribute__((unused)));
int ioapic_set_func(int argc, char *argv[] __attribute__((unused)));

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
