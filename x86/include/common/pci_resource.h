/* $Id: pci_resource.h,v 1.1.1.1 2003/06/10 22:42:03 iceblink Exp $
 *
 * Copyright 2000 Cobalt Networks, Inc.
 * Copyright 2001 Sun Microsystems, Inc.
 */
#ifndef PCI_RESOURCE_H
#define PCI_RESOURCE_H

#include "common/pci.h"

void pci_resource_init(pci_bus *);

uint32 pci_resource_bridge(pci_bus *, pci_bar_t);
uint32 pci_resource_cardbus(pci_bus *, pci_bar_t);
uint32 pci_resource_cnb30le(pci_bus *, pci_bar_t);
uint32 pci_resource_m1541(pci_bus *bus, pci_bar_t type );

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
