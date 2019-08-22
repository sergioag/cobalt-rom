/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: pci.h,v 1.1.1.1 2003/06/10 22:42:21 iceblink Exp $
 */

#ifndef MONITOR_PCI_H
#define MONITOR_PCI_H

int lspci_func( int argc, char *argv[] );
int read_pci_cfg_func( int argc, char *argv[] );
int write_pci_cfg_func( int argc, char *argv[] );
int dump_pci_func( int argc, char * argv[] );
int print_pci_header_func( int argc, char *argv[] );
int dump_all_pci_func(int argc, char * argv[] __attribute__ ((unused)));

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
