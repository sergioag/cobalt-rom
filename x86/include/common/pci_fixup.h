/* $Id: pci_fixup.h,v 1.1.1.1 2003/06/10 22:42:03 iceblink Exp $
 *
 * Copyright 2000 Cobalt Networks, Inc.
 * Copyright 2001 Sun Microsystems, Inc.
 */
#ifndef PCI_FIXUP_H
#define PCI_FIXUP_H

#include "common/pci.h"

int pci_fixup_pre(pci_dev *dev);
int pci_fixup_post(pci_dev *dev);
int pci_irq_fixup_pre(pci_dev *dev);
int pci_irq_fixup_post(pci_dev *dev);
int pci_resource_fixup_pre(pci_dev *dev, pci_bar_t type);
int pci_resource_fixup_post(pci_dev *dev, pci_bar_t type);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
