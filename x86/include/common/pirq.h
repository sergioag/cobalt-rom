/* $Id: pirq.h,v 1.1.1.1 2003/06/10 22:42:03 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 */

#ifndef COMMON_PIRQ_H
#define COMMON_PIRQ_H

#include <extra_types.h>
#include "common/pci.h"

#define IRQMAP_ISA		((1<<1)|(1<<3)|(1<<4)|(1<<8))
#define IRQMAP_PCI		((1<<5)|(1<<7)|(1<<11)|(1<<12))

#define IRQ_USB			9
#define IRQ_ACPI		10
#define IRQ_IDE0		14
#define IRQ_IDE1		15

#define PIRQ_BASE_ADDRESS	0xF8000	/* table location in memory */
#define PIRQ_VERSION		0x0100
#define PIRQ_ELCR_REG		0x04d0
#define PIRQ_SIGNATURE		(('$'<<0)+('P'<<8)+('I'<<16)+('R'<<24))
#define PIRQ_TABLE_SIZE(e)	((16*(e))+32)
#define PMAP_NSLOTS(map)	(((map)->size-32)/16)

/* ServerWorks IRQ router has index 0xc00, redir 0xc01 */
#define SERVERWORKS_IRQ_INDEX		0x0C00
#define SERVERWORKS_IRQ_REDIRECT	0x0C01

#define SERVERWORKS_IRQ_INDEX_ACPI	0
#define SERVERWORKS_IRQ_INDEX_USB	1
#define SERVERWORKS_IRQ_INDEX_IDE0	2
#define SERVERWORKS_IRQ_INDEX_SMBUS	3
#define SERVERWORKS_IRQ_INDEX_IDE1	4

int  pirq_init(void);
byte pirq_route_pci(pci_dev *, byte);
byte pciirq_5k(byte, word);
void pciirq_5k_set_level(byte);
byte pciirq_lookup_5k(byte);

#endif /* COMMON_PIRQ_H */
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
