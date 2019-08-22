/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: eth.h,v 1.2 2003/10/06 15:15:03 iceblink Exp $ */

#ifndef COMMON_ETH_H
#define COMMON_ETH_H

int init_eth( void );
int print_eth_mac( void );
int eth_wol_setup(void);
char * eth_get_first_mac(void);
#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
