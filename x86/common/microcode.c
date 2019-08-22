/* $Id: microcode.c,v 1.1.1.1 2003/06/10 22:41:49 iceblink Exp $
 * Copyright (c) 2000 Cobalt Networks, Inc.
 *
 * updates intel cpu microcode
 *
 * Duncan Laurie
 */

#include <printf.h>
#include <string.h>
#include <stdlib.h>
#include <extra_types.h>

#include "common/microcode.h"
#include "common/rammap.h"
#include "common/msr.h"
#include "common/x86.h"
#include "common/systype.h"
#include "common/fs.h"

/* microcode update images are 2048bytes (512dwords)
 *   header is 48b
 *   data is 2000b
 * the checksum value is correct if sum of 512dwords is zero
 */

struct microcode {
	dword version;		/* version of this header data structure */
	dword revision;		/* revision of microcode in data portion */
	dword date;		/* date of update creation		 */
	dword processor;	/* family/model/stepping req'd for update */
	dword checksum;		/* checksum of data+header */
	dword loader;		/* version of loader used to install */
	dword flags;		/* processor flags (from MSR 0x17) */
	dword reserved[5];	/* for future expansion */
	dword data[500];	/* updata data image */
};

void update_cpu_microcode(void)
{
	unsigned int a,b,c,d;
	unsigned int procid, flags, x;
	unsigned long long msr, rev;
	struct crfs_stat stb;
	struct microcode *updates;

	if (systype_is_3k())
	    return;

	/* first clear microcode MSR */
	msr_set(MSR_MICROCODE_SIG, 0);

	/* call cpuid which sets MSR_MICROCODE_SIG if microcode is loaded */
	cpuid(1, &a, &b, &c, &d);
	procid = a;

	/* then save current microcode signature */
	rev = msr_get(MSR_MICROCODE_SIG);

	/* read processor flags from MSR */
	msr = msr_get(MSR_BBL_CR_OVRD);

	/* stored as BCD in bits 52:50,
	 * update header expects corresponding bit # to be set
	 */
	flags = (dword) (1 << ((msr >> 50) & 0x7));

	if (crfs_stat(MICROCODE_FILE, &stb) < 0) {
		printf("IA32 Microcode Update: Unable to stat romfs file '%s'\n",
		       MICROCODE_FILE);
		return;
	}

	if (load_from_romfs(MICROCODE_FILE) < 0) {
		printf("IA32 Microcode Update: Unable to load '%s' from romfs\n",
		       MICROCODE_FILE);
		return;
	}

	updates = (void *)RAM_SERIAL_LOAD;

	/* now go through update list and see if we have a match */
	for (x = 0; x < stb.st_size/sizeof(struct microcode); x++)
	{
		if (updates[x].processor == procid && updates[x].flags == flags) {

			/* we found a match -- first see if it is newer */
			if (updates[x].revision <= rev) {
				printf("IA32 Microcode Update: CPU already updated!\n");
				return;
			}

			/* put it in MSR to trigger update */
			__asm__ __volatile__("wrmsr" : :
					     "c" (MSR_MICROCODE_TRIG),
					     "a" ((unsigned int)&updates[x].data),
					     "d" (0));

			/* call cpuid again to update microcode signature MSR */
			cpuid(1, &a, &b, &c, &d);

			msr = msr_get(MSR_MICROCODE_SIG);
			printf("IA32 Microcode Update: %s\n", (rev==msr) ? "Failed" : "OK");

			return;
		}
	}

	printf("IA32 Microcode Update: not found [ID:0x%08x REV:0x%08x FLAG:0x%02x]\n",
	       procid, (unsigned int)rev, flags);
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
