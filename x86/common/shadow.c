/* $Id: shadow.c,v 1.1.1.1 2003/06/10 22:41:54 iceblink Exp $
 * Copyright (c) 1997-2000 Cobalt Networks, Inc.
 */

#include <extra_types.h>
#include <io.h>

#include "common/rom.h"
#include "common/pci.h"
#include "common/systype.h"

void shadow_set_RW (void)
{
	byte val;

	if (systype_is_5k()) {
		/* enable read-write ROM shadowing for f0000-fffff
		 * for PIRQ/MP/ACPI tables */
		val = pci_read_config_byte(0, PCI_DEVFN(0,0), 0x73);
		pci_write_config_byte(0, PCI_DEVFN(0,0), 0x73, val | 0xf0);
	}
}

void shadow_set_RO (void)
{
	byte val;

	if (systype_is_5k()) {
		/* enable read-only ROM shadowing for f0000-fffff */
		val = pci_read_config_byte(0, PCI_DEVFN(0,0), 0x73);
		pci_write_config_byte(0, PCI_DEVFN(0,0), 0x73, val & ~0x50);
	}
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
