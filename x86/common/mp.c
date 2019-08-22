/* $Id: mp.c,v 1.2 2003/12/11 00:57:09 thockin Exp $
 * Copyright (c) 1999-2000 Cobalt Networks, Inc.
 * Copyright (c) 2001 Sun Microsystems, Inc.
 *
 *   Erik Gilling <erik@cobalt.com>
 *   Duncan Laurie <smp@sun.com>
 */

#include <printf.h>
#include <stdlib.h>

#include "common/mp.h"
#include "common/systype.h"
#include "common/apic.h"
#include "common/cmos.h"
#include "common/rom.h"
#include "common/cache.h"
#include "common/pci.h"
#include "common/delay.h"
#include "common/mptable.h"
#include "common/ioapic.h"

static void mp_setup_trampoline(unsigned int);
static int  mp_do_trampoline(int, uint32);
static int  mp_detect_p6(uint16);

extern void *mp_boot;
extern void *mp_boot_end;

static const int mp_cpus[] = { 0, 3 };
static int mp_n_cpus;
static int mp_map_cpus;

/*
 * send INIT/IPI sequence to start APs
 * return 1 for success, 0 for fail
 */
static int mp_do_trampoline (int id, uint32 vector)
{
	volatile uint8 *p = (uint8 *) 0x9800;
	int k;

	/* clear AP status byte */
	*p = 0;
	DPRINTF("SMP: attempting to start CPU %d\n", id);

	for (k=0; k<3; k++)
	{
		/* send startup ipi */
		lapic_send_ipi_startup(id, vector>>12);

		udelay(200);
		if (lapic_wait_for_idle()) {
			break;
		}

		udelay(200);
		if (lapic_status()) {
			break;
		}

		/* wait for AP to boot */
		udelay(0xa0000);
		if (*p != 0) {
			DPRINTF("SMP: found CPU %d\n", id);
			return 1;
		}
	}

	DPRINTF("SMP: CPU %d not found!\n", id);
	return 0;
}

/*
 * relocate AP boot code block
 * setup GDT
 */
static void mp_setup_trampoline (unsigned int base)
{
	unsigned int *scratch, *boot_ap;
	uint32 length;
	uint16 limit, j;
	struct {
		unsigned short size __attribute__ ((packed));
		unsigned long base __attribute__ ((packed));
	} gdtr;


	scratch = (unsigned int *)TRAMPOLINE_MEM;
	boot_ap = (unsigned int *)&mp_boot;
	length = (uint32)(&mp_boot_end)-(uint32)(&mp_boot);
    
	/* tell trampoline where the gdt is */
	__asm__ __volatile__("sgdt %0": :"m" (gdtr));
	limit = gdtr.size + 1;
	scratch[0] = base;
	scratch[1] = limit;
	scratch[2] = (gdtr.base >> 16) & 0xffff;
	scratch[3] = (gdtr.base & 0xffff);
	scratch = (unsigned int *)gdtr.base;

	/* relocate trampoline code */
	DPRINTF("SMP: relocate AP boot code from 0x%08x to 0x%08x "
		"[size=%s]\n", (uint32)(&mp_boot), base,
		print_bytes(length));

	scratch = (unsigned int *)base;
	for (j=0; j<length; j++) {
		scratch[j] = boot_ap[j];
	}
}

static int mp_detect_p6 (uint16 base)
{
	uint8 i;

	mp_map_cpus = 0;
	mp_n_cpus = 1;

	if (!base || !lapic_get_bsp()) {
		return -1;
	}

	for (i=0; i<(sizeof(mp_cpus)/sizeof(int)); i++)
	{
		if (lapic_get_id() != mp_cpus[i]) {
			mp_setup_trampoline(base);
			if (!mp_do_trampoline(mp_cpus[i], base)) {
				continue;
			}
			mp_n_cpus++;
			systype_init_ap_ID(mp_cpus[i]);
		}
		mp_map_cpus |= (1 << mp_cpus[i]);
	}
	return mp_n_cpus;
}

int mp_init (void)
{
	switch(systype()) {
	case SYSTYPE_3K:
		mp_n_cpus = 1;
		break;

	case SYSTYPE_5K: {
		pci_dev *dev;

		mtrr_init();
		lapic_init();
		ioapic_init();
		mptable_init();

		if (mp_detect_p6(TRAMPOLINE_BASE) <= 0) {
			break;
		}

		/**** ServerWorks CNB30LE v1.8 spec, 4.7.39
		 *
		 * 0xCE: bit 0 applies to CPU-0 and bit 3 applies
		 *       to CPU-3.  when this bit is set it implies
		 *       that CPU is present.
		 */
		dev = pci_lookup(PCI_VENDOR_SERVERWORKS,
				 PCI_DEVICE_SERVERWORKS_LE);
		if (dev) {
			pci_dev_writeb(dev, 0xCE, mp_map_cpus);
		}
			
		/**** ServerWorks CSB5 v2.0 spec, 5.2.17
		 *
		 * 0x44: this is a count for the number of
		 *       CPU's present (00 = 1 CPU present)
		 */
		if (boardtype() == BOARDTYPE_ALPINE) {
			dev = pci_lookup(PCI_VENDOR_SERVERWORKS,
					 PCI_DEVICE_SERVERWORKS_CSB5);
			if (dev) {
				pci_dev_writeb(dev, 0x44, mp_n_cpus-1);
			}
		}

		break;
	}

	default:
		return 0;
	}

	mp_list();

	return mp_n_cpus;
}

/* FIXME: this doesn't handle per-CPU speeds */
void mp_list(void)
{
	struct cpu_info *cpu;

	printf("CPU: %d processor(s) detected\n", mp_n_cpus);

	for (cpu=systype_get_info(0); cpu; cpu=cpu->NEXT) {
		uint8 rt = cpu->clock_ratio;
		printf("  CPU %d: %s %dMHz (%d%s x %dMHz host bus)%s\n",
		       cpu->apic_id, cpu->vendor_string, cpu->cpu_clock,
		       rt >> 1, (rt & 1) ? ".5" : "",
		       cpu->host_bus_clock, cpu->bsp ? " [BSP]" : "");
	}
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
