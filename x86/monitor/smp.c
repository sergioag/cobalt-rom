/* $Id: smp.c,v 1.1.1.1 2003/06/10 22:42:22 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 */
#include <string.h>
#include <printf.h>
#include <extra_types.h>

#include "common/rom.h"
#include "common/systype.h"
#include "common/pci.h"
#include "common/apic.h"
#include "common/ioapic.h"

#include "smp.h"

int lapic_set_func(int argc, char *argv[] __attribute__((unused)))
{
	unsigned long reg, val;

	if (argc != 3)
		return EBADNUMARGS;

	if ((stoli(argv[1], &reg) != 0)	||
	    (stoli(argv[2], &val) != 0))
		return EBADNUM;

	lapic_set_reg((dword) reg, (dword) val);

	return 0;
}

int lapic_get_func(int argc, char *argv[] __attribute__((unused)))
{
	unsigned long reg;

	if (argc != 2)
		return EBADNUMARGS;

	if (stoli(argv[1], &reg) != 0)
		return EBADNUM;

	printf("Local APIC register 0x%x = 0x%08x\n",
	       (dword) reg, lapic_get_reg((dword) reg));

	return 0;
}

int lapic_on_func(int argc, char *argv[] __attribute__((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;

	lapic_enable();

	return 0;
}

int lapic_off_func(int argc, char *argv[] __attribute__((unused)))
{
	if (argc != 1)
		return EBADNUMARGS;

	lapic_disable();

	return 0;
}

int lapic_send_ipi_func(int argc, char *argv[] __attribute__((unused)))
{
	unsigned long dest, mode, vector, trigger, level;

	if (argc != 6)
		return EBADNUMARGS;

	if ((stoli(argv[1], &dest)    != 0) ||
	    (stoli(argv[2], &mode)    != 0) ||
	    (stoli(argv[3], &vector)  != 0) ||
	    (stoli(argv[4], &trigger) != 0) ||
	    (stoli(argv[5], &level)   != 0))
		return EBADNUM;

	lapic_send_ipi(dest, mode, vector, trigger, level);

	return 0;
}

int lapic_cast_ipi_func(int argc, char *argv[] __attribute__((unused)))
{
	unsigned long dest, mode, vector, trigger, level;

	if (argc != 6)
		return EBADNUMARGS;

	if ((stoli(argv[1], &dest)    != 0) ||
	    (stoli(argv[2], &mode)    != 0) ||
	    (stoli(argv[3], &vector)  != 0) ||
	    (stoli(argv[4], &trigger) != 0) ||
	    (stoli(argv[5], &level)   != 0))
		return EBADNUM;

	lapic_broadcast_ipi(dest, mode, vector, trigger, level);

	return 0;
}

int ioapic_get_func(int argc, char *argv[] __attribute__((unused)))
{
	unsigned long id, reg;

	if (argc != 3)
		return EBADNUMARGS;

	if ((stoli(argv[1], &id)  != 0) ||
	    (stoli(argv[2], &reg) != 0))
		return EBADNUM;

	printf("I/O APIC #%d register 0x%x = 0x%08x\n",
	       (dword) id, (dword) reg, ioapic_get_reg((int) id, (dword) reg));

	return 0;
}

int ioapic_set_func(int argc, char *argv[] __attribute__((unused)))
{
	unsigned long id, reg, val;

	if (argc != 4)
		return EBADNUMARGS;

	if ((stoli(argv[1], &id)  != 0) ||
	    (stoli(argv[2], &reg) != 0) ||
	    (stoli(argv[3], &val) != 0))
		return EBADNUM;

	ioapic_set_reg((int) id, (dword) reg, (dword) val);

	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
