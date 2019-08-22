/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/*
 * $Id: acpi.h,v 1.1.1.1 2003/06/10 22:42:00 iceblink Exp $
 *
 * ACPI support
 */

#ifndef COMMMON_ACPI_H
#define COMMMON_ACPI_H

/* serverworks power management */
#define ACPI_INDEX_REG    0x0cd6
#define ACPI_DATA_REG     0x0cd7

void acpi_init (void);
void acpi_osb4_init(pci_dev *dev);

#endif /* common/acpi.h */
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
